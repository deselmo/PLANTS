#include "ApplicationLayer.h"

ApplicationLayer::ApplicationLayer(MicroBit* uBit, NetworkLayer* nl, SerialCom* serial, bool sink_mode){
    this->uBit = uBit;
    this->nl = nl;
    this->serial = serial;
    this->sink_mode = sink_mode;
    this->connected = sink_mode ? true : false;
}


void sensing_loop(void *par){
    Sensor *sensor = (Sensor *)par;
    while(true)
    {
        if(sensor->sensing_rate == 0)
        {
            sensor->active_loop = false;
            return;
        }
        if(!sensor->app->connected)
        {
            sensor->active_loop = false;
            return;
        }
        sensor->loop(sensor);
        sensor->app->uBit->sleep(sensor->sensing_rate);
    }
}
void humidity_loop(Sensor *par){
    
}



void thermomether_loop(Sensor *par){

}

void ApplicationLayer::init(){
    this->uBit->messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, this, &ApplicationLayer::recv_from_network);
    this->uBit->messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE, this, &ApplicationLayer::update_connection_status);
    this->nl->init();
    if(this->serial)
    {
        this->serial->addListener(APPLICATION_ID, SENSING_REQ, this, &ApplicationLayer::recv_sensing_req);
    }
    else
    {
        Sensor * tmp = new Sensor;
        tmp->name = "humidity";
        tmp->app = this;
        tmp->active_loop = false;
        tmp->min_value = false;
        tmp->max_value = false;
        tmp->sensing_rate = 0;
        tmp->loop = humidity_loop;
        sensors[tmp->name] = tmp;

        tmp = new Sensor;
        tmp->name = "thermometer";
        tmp->app = this;
        tmp->active_loop = false;
        tmp->min_value = false;
        tmp->max_value = false;
        tmp->sensing_rate = 0;
        tmp->loop = thermomether_loop;
        sensors[tmp->name] = tmp;
    }
}

void ApplicationLayer::sleep(uint32_t time){
    uBit->sleep(time);
}

void ApplicationLayer::update_connection_status(MicroBitEvent e){
    if(sink_mode)
        return update_connection_status_sink();
    DDConnection info = nl->recv_connection();
    if(info.status)
    {
        connected = true;
        std::map<ManagedString, Sensor *>::iterator it;
        for(it = sensors.begin(); it != sensors.end(); ++it)
        {
            Sensor *sensor = it->second;
            if(!sensor->active_loop)
            {
                sensor->active_loop = true;
                create_fiber(sensing_loop, (void *)sensor);
            }
        }
    }
    else
        connected = false;
}

void ApplicationLayer::update_connection_status_sink(){
    DDConnection info = nl->recv_connection();
    if(!info.status)
    {
        ManagedBuffer b(sizeof(uint32_t));
        uint8_t *buf = b.getBytes();
        memcpy(buf, &(info.address), sizeof(uint32_t));
        serial->send(APPLICATION_ID, DISCONNECTED_PLANT, b);
        if(waiting_ack && info.address == dest)
        {
            waiting_ack = false;
            ManagedBuffer b(1);
            b[0] = 2;
            serial->send(APPLICATION_ID, SENSING_RESP, b);
        }
    }
}

void send_sensing_req(void *par){
    AppAndData *tmp = (AppAndData *)par;
    ApplicationLayer *app = tmp->app;
    ManagedBuffer buf = tmp->msg;
    uint8_t *msg = buf.getBytes();
    uint32_t microbit_id;
    uint32_t sensor_len;
    uint8_t *sensor_name;
    uint32_t len = 0;
    uint8_t sample;
    uint32_t sample_rate;
    uint8_t min;
    uint32_t min_value;
    uint8_t max;
    uint32_t max_value;
    
    memcpy(&microbit_id, (void*)msg, sizeof(uint32_t));
    len += sizeof(uint32_t);
    memcpy(&sensor_len, msg + len, sizeof(uint32_t));
    len += sizeof(uint32_t);
    sensor_name = new uint8_t [sensor_len];
    memcpy(sensor_name, msg + len, sensor_len);
    len += sensor_len;
    memcpy(&sample, msg + len, 1);
    len += 1;
    memcpy(&min, msg + len, 1);
    len += 1;
    memcpy(&max, msg + len, 1);
    if(sample == 1)
    {
        memcpy(&sample_rate, msg + len, sizeof(uint32_t));
        len += sizeof(uint32_t);
    }
    if(min == 1)
    {
        memcpy(&min_value, msg + len, sizeof(uint32_t));
        len += sizeof(uint32_t);
    }
    if(max == 1)
    {
        memcpy(&max_value, msg + len, sizeof(uint32_t));
        len += sizeof(uint32_t);
    }
    Message toSend;
    toSend.type = SET_GRADIENT;
    toSend.len = sizeof(uint32_t);
    toSend.len += sensor_len;
    toSend.len += sizeof(uint8_t)*3;
    toSend.len += sample == 1? sizeof(uint32_t) : 0;
    toSend.len += min == 1? sizeof(uint32_t) : 0;
    toSend.len += max == 1? sizeof(uint32_t) : 0;
    toSend.payload = new uint8_t [toSend.len]; 
    uint8_t *payload = toSend.payload;
    len = 0;
    memcpy(payload,&sensor_len, sizeof(uint32_t));
    len += sizeof(uint32_t);
    memcpy(payload + len, sensor_name, sensor_len);
    len += sensor_len;
    memcpy(payload + len, &sample, 1);
    len += 1;
    memcpy(payload + len, &min, 1);
    len += 1;
    memcpy(payload + len, &max, 1);
    if(sample == 1)
    {
        memcpy(payload + len, &sample_rate, sizeof(uint32_t));
        len += sizeof(uint32_t);
    }
    if(min == 1)
    {
        memcpy(payload + len, &min_value, sizeof(uint32_t));
        len += sizeof(uint32_t);
    }
    if(max == 1)
    {
        memcpy(payload + len, &max_value, sizeof(uint32_t));
        len += sizeof(uint32_t);
    }
    app->waiting_ack = true;
    app->dest = microbit_id;
    if(!app->sendMessage(toSend))
    {
        app->waiting_ack = false;
        ManagedBuffer b(1);
        b[0] = 1;
        app->serial->send(APPLICATION_ID, SENSING_RESP, b);
        return;
    }
    uint32_t start = system_timer_current_time();
    while(app->waiting_ack && start - system_timer_current_time() < ACK_WAITING_TIME)
        app->uBit->sleep(50);
    if(app->waiting_ack)
    {
        ManagedBuffer b(1);
        b[0] = 2;
        app->waiting_ack = false;
        app->serial->send(APPLICATION_ID, SENSING_RESP, b);
    }
}

void ApplicationLayer::recv_sensing_req(ManagedBuffer buf){
    if(waiting_ack)
    {
        ManagedBuffer b(1);
        b[0] = 3;
        serial->send(APPLICATION_ID, SENSING_RESP, b);
        return;
    }
    AppAndData *tmp = new AppAndData;
    tmp->app = this;
    tmp->msg = buf;
    create_fiber(&send_sensing_req, (void *)tmp);
}


bool ApplicationLayer::sendMessage(Message msg){
    ManagedBuffer b(msg.len+1);
    b[0] = msg.type;
    uint8_t *buf = b.getBytes();
    memcpy(buf+1,msg.payload,msg.len);
    return nl->send(b, dest);
}