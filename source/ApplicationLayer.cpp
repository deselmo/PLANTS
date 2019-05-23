#include "ApplicationLayer.h"

ApplicationLayer::ApplicationLayer(MicroBit* uBit, NetworkLayer* nl){
    this->uBit = uBit;
    this->nl = nl;
}

Message Message::from(DDMessage msg){
    Message ret;
    ManagedBuffer b = msg.payload;
    ret.type = b[0];
    ret.len = b.length() - 1;
    ret.payload = new uint8_t[b.length() -1];
    memcpy(ret.payload,b.getBytes()+1, b.length() - 1);
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
        uint32_t now = system_timer_current_time();
        while(system_timer_current_time() - now < sensor->sensing_rate)
            sensor->app->uBit->sleep(20);
    }
}

float humidity_loop(Sensor *par){
    
}



float thermomether_loop(Sensor *par){

}

void ApplicationLayer::init(SerialCom *serial, bool sink){
    this->uBit->messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, this, &ApplicationLayer::recv_from_network);
    this->uBit->messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE, this, &ApplicationLayer::update_connection_status);
    this->serial = serial;
    this->sink_mode = sink;
    this->connected = sink_mode ? true : false;
    if(this->sink_mode)
    {
        this->nl->init(serial, sink);
        this->serial->addListener(APPLICATION_ID, SENSING_REQ, this, &ApplicationLayer::recv_sensing_req);
    }
    else
    {
        this->nl->init(serial, sink);
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
        uBit->display.print("C");
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
    {
        uBit->display.print("N");
        connected = false;
    }
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
    uint8_t sample;
    uint32_t sample_rate;
    uint8_t min;
    uint32_t min_value;
    uint8_t max;
    uint32_t max_value;

    uint32_t len = 0;
    
    memcpy(&microbit_id, msg, sizeof(uint32_t));
    len += sizeof(uint32_t);

    memcpy(&sensor_len, msg + len, sizeof(uint32_t));
    len += sizeof(uint32_t);

    uint8_t sensor_name[sensor_len];
    memcpy(sensor_name, msg + len, sensor_len);
    len += sensor_len;

    memcpy(&sample, msg + len, 1);
    len += 1;

    memcpy(&min, msg + len, 1);
    len += 1;

    memcpy(&max, msg + len, 1);
    len += 1;

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
    toSend.len = len - sizeof(uint32_t);
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
    len += 1;

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
        delete []toSend.payload;
        app->waiting_ack = false;
        ManagedBuffer b(1);
        b[0] = 1;
        app->serial->send(APPLICATION_ID, SENSING_RESP, b);
        return;
    }

    
    delete []toSend.payload;
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
        ManagedBuffer s(size);
        uint8_t *str = s.getBytes();
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
            if(sensor->name == s.toManagedString())
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
                    if(0 < sample_rate && sample_rate < 100)
                        sensor->sensing_rate = 100;
                    else
                        sensor->sensing_rate = sample_rate;
                    if(!sensor->active_loop && sample_rate != 0)
                    {
                        sensor->active_loop = true;
                        create_fiber(&sensing_loop, (void *)sensor);
                    }
                }
            }
        }
        AppAndData *tmp = new AppAndData;
        tmp->app = this;
        tmp->msg = s;
        create_fiber(&send_sensing_ack, (void *)tmp);   
    }
    //From here the message are received from the sink
    else if(m.type == SET_GRADIENT_ACK)
    {
        if(dest == msg.source)
        {
            waiting_ack = false;
            ManagedBuffer b(1);
            b[0] = 0;
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
    delete []m.payload;
}


void ApplicationLayer::addSensor(ManagedString name, float (*f)(Sensor *)){
    Sensor *s = new Sensor;
    s->active_loop = false;
    s->min_value = false;
    s->max_value = false;
    s->sensing_rate = SENSING_INTERVAL;
    s->name = name;
    s->app = this;
    s->loop = f;
    sensors.push_back(s);
    if(connected)
    {
        send_microbit_info();
        s->active_loop = true;
        create_fiber(&sensing_loop, (void *)s);
    }
}

void ApplicationLayer::serial_send_debug(ManagedBuffer b){
    if(serial)
        serial->send(APPLICATION_ID, DEBUG, b);
}