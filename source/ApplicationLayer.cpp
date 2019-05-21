#include "ApplicationLayer.h"

ApplicationLayer::ApplicationLayer(MicroBit* uBit, NetworkLayer* nl, SerialCom* serial, bool sink_mode){
    this->uBit = uBit;
    this->nl = nl;
    this->serial = serial;
    this->sink_mode = sink_mode;
    this->connected = sink_mode ? true : false;
}

Message Message::from(DDMessage msg){
    Message ret;
    ManagedBuffer b = msg.payload;
    ret.type = b[0];
    ret.len = b.length() - 1;
    ret.payload = new uint8_t[b.length() -1];
    memcpy(ret.payload,b.getBytes(), b.length() - 1);
    return ret;
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
        bool send = true;
        float sensed = sensor->loop(sensor);
        int tmp_sensed = (int)sensed;
        if(sensor->min_value && tmp_sensed < sensor->min_value_threshold)
            send = false;
        if(sensor->max_value && tmp_sensed > sensor->max_value_threshold)
            send = false;
        if(send)
            sensor->app->send_app_data(sensor->name, sensed);
        sensor->app->uBit->sleep(sensor->sensing_rate);
    }
}

float humidity_loop(Sensor *par){
    
}



float thermomether_loop(Sensor *par){

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
        sensors.push_back(tmp);

        tmp = new Sensor;
        tmp->name = "thermometer";
        tmp->app = this;
        tmp->active_loop = false;
        tmp->min_value = false;
        tmp->max_value = false;
        tmp->sensing_rate = 0;
        tmp->loop = thermomether_loop;
        sensors.push_back(tmp);
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
        send_microbit_info();
        connected = true;
        std::vector<Sensor *>::iterator it;
        for(it = sensors.begin(); it != sensors.end(); ++it)
        {
            Sensor *sensor = *it;
            if(!sensor->active_loop)
            {
                sensor->active_loop = true;
                create_fiber(&sensing_loop, (void *)sensor);
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
    delete tmp;
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
    delete sensor_name;
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
        delete toSend.payload;
        app->waiting_ack = false;
        ManagedBuffer b(1);
        b[0] = 1;
        app->serial->send(APPLICATION_ID, SENSING_RESP, b);
        return;
    }
    delete toSend.payload;
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

void ApplicationLayer::send_microbit_info(){
    uint32_t len = sizeof(uint8_t) + sizeof(uint32_t)+sensors.size()*sizeof(uint32_t);
    std::vector<Sensor *>::iterator it;
    for(it = sensors.begin(); it != sensors.end(); ++it)
        len += (*it)->name.length()+1;
    ManagedBuffer b(len);
    uint8_t *buf = b.getBytes();
    uint32_t size = sensors.size();
    len = 1;
    buf[0] = MICRO_INFO;
    memcpy(buf + len, &size, sizeof(uint32_t));
    len += sizeof(uint32_t);
    for(it = sensors.begin(); it != sensors.end(); ++it)
    {
        size = (*it)->name.length() + 1;
        memcpy(buf + len,&size,sizeof(uint32_t));
        len += sizeof(uint32_t);
        memcpy(buf + len, (*it)->name.toCharArray(), size);
        len += size;
    }
    nl->send(b);
}

void ApplicationLayer::send_app_data(ManagedString sensor, float value){
    uint32_t len = sizeof(uint8_t) + sizeof(uint32_t);
    len += sensor.length() + 1;
    len += sizeof(float);
    ManagedBuffer b(len);
    uint8_t *buf = b.getBytes();
    buf[0] = APP_DATA;
    len = 1;
    uint32_t size = sensor.length() + 1;
    memcpy(buf + len, &size, sizeof(uint32_t));
    len += sizeof(uint32_t);
    memcpy(buf + len, sensor.toCharArray(), size);
    len += size;
    memcpy(buf + len, &value, sizeof(float));
    nl->send(b);
}

void send_sensing_ack(void *par){
    AppAndData *tmp = (AppAndData *)par;
    ApplicationLayer *app = tmp->app;
    ManagedBuffer msg = tmp->msg;
    ManagedBuffer resp(msg.length() + 1 + sizeof(uint32_t));
    resp[0] = SET_GRADIENT_ACK;
    uint32_t size = msg.length();
    uint8_t *buf = resp.getBytes();
    memcpy(buf + 1,&size,sizeof(uint32_t));
    memcpy(buf + 1+ sizeof(uint32_t), msg.getBytes(), size);
    app->nl->send(resp);
    delete tmp;
}

void send_new_plant(void *par){
    AppAndData *tmp = (AppAndData *)par;
    ApplicationLayer *app = tmp->app;
    ManagedBuffer msg = tmp->msg;
    uint32_t microbit_id = tmp->source;
    ManagedBuffer resp(sizeof(uint32_t) + msg.length());
    uint8_t *buf = resp.getBytes();
    memcpy(buf, &microbit_id, sizeof(uint32_t));
    memcpy(buf + sizeof(uint32_t), msg.getBytes(), msg.length());
    app->serial->send(APPLICATION_ID, NEW_PLANT, resp);
    delete tmp;
}

void send_new_sample(void *par){
    AppAndData *tmp = (AppAndData *)par;
    ApplicationLayer *app = tmp->app;
    ManagedBuffer msg = tmp->msg;
    uint32_t microbit_id = tmp->source;
    ManagedBuffer resp(sizeof(uint32_t) + msg.length());
    uint8_t *buf = resp.getBytes();
    memcpy(buf, &microbit_id, sizeof(uint32_t));
    memcpy(buf + sizeof(uint32_t), msg.getBytes(), msg.length());
    app->serial->send(APPLICATION_ID, NEW_SAMPLE, resp);
    delete tmp;
}

void ApplicationLayer::recv_from_network(MicroBitEvent e){
    DDMessage msg = nl->recv();
    if(msg == DDMessage::Empty)
        return;
    Message m = Message::from(msg);
    //if I'm a node this is the only message I should receive :)
    if(m.type == SET_GRADIENT)
    {
        uint32_t size;
        uint32_t len;
        uint8_t *payload = m.payload;
        memcpy(&size,payload,sizeof(uint32_t));
        len = sizeof(uint32_t);
        ManagedString s((int)size);
        uint8_t *str = (uint8_t *) s.toCharArray();
        memcpy(str, payload + len, size);
        len += size;
        uint8_t sample;
        uint8_t min;
        uint8_t max;
        memcpy(&sample, payload + len, 1);
        len += 1;
        memcpy(&min, payload + len, 1);
        len += 1;
        memcpy(&max, payload + len, 1);
        len += 1;
        uint32_t sample_rate;
        uint32_t min_val;
        uint32_t max_val;
        if(sample == 1)
        {
            memcpy(&sample_rate, payload + len, sizeof(uint32_t));
            len += sizeof(uint32_t);
        }
        if(min == 1)
        {
            memcpy(&min_val, payload + len, sizeof(uint32_t));
            len += sizeof(uint32_t);
        }
        if(max == 1)
        {
            memcpy(&max_val, payload + len, sizeof(uint32_t));
            len += sizeof(uint32_t);
        }
        std::vector<Sensor *>::iterator it;
        for(it = sensors.begin(); it != sensors.end(); ++it)
        {
            Sensor *sensor = *it;
            if(sensor->name == s)
            {
                if(min == 1)
                {
                    sensor->min_value = true;
                    sensor->min_value_threshold = min_val;
                }
                else if(min == 2)
                    sensor->min_value = false;
                if(max == 1)
                {
                    sensor->max_value = true;
                    sensor->max_value_threshold = max_val;
                }
                if(sample == 1)
                {
                    sensor->sensing_rate = sample_rate;
                    if(!sensor->active_loop && sample_rate != 0)
                    {
                        sensor->active_loop = true;
                        create_fiber(&sensing_loop, (void *)sensor);
                    }
                }
            }
        }
        ManagedBuffer b = s;
        AppAndData *tmp = new AppAndData;
        tmp->app = this;
        tmp->msg = b;
        create_fiber(&send_sensing_ack, (void *)tmp);   
    }
    //From here the message are received from the sink
    else if(m.type == SET_GRADIENT_ACK)
    {
        if(dest == msg.source)
        {
            waiting_ack = false;
            ManagedBuffer b(1);
            b[0] = 1;
            serial->send(APPLICATION_ID, SENSING_RESP, b);
        }
    }
    else if(m.type == MICRO_INFO)
    {
        AppAndData *tmp = new AppAndData;
        tmp->app = this;
        tmp->source = msg.source;
        ManagedBuffer b(m.payload, m.len);
        tmp->msg = b;
        create_fiber(&send_new_plant, (void *)tmp);
    }
    else if(m.type == APP_DATA)
    {
        AppAndData *tmp = new AppAndData;
        tmp->app = this;
        tmp->source = msg.source;
        ManagedBuffer b(m.payload, m.len);
        tmp->msg = b;
        create_fiber(&send_new_sample, (void *)tmp);
    }
    delete m.payload;
}