#include "ApplicationLayer.h"

ApplicationLayer::ApplicationLayer(MicroBit* uBit, NetworkLayer* nl, SerialCom* serial, bool sink_mode){
    this->uBit = uBit;
    this->nl = nl;
    this->serial = serial;
    this->sink_mode = sink_mode;
}

void sensing_loop(void *par){
    ApplicationLayer *app = (ApplicationLayer *)par;
    while(true)
    {

        app->uBit->sleep(SENSING_INTERVAL);
    }
}

void ApplicationLayer::init(){
    this->uBit->messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, this, &ApplicationLayer::recv_from_network);
    this->nl->init();
    if(this->serial)
    {
        this->serial->addListener(APPLICATION_ID, SENSING_REQ, this, &ApplicationLayer::recv_sensing_req);
        this->serial->addListener(APPLICATION_ID, REMOVE_SENSING_REQ, this, &ApplicationLayer::recv_remove_sensing_req);
    }

    create_fiber(&sensing_loop, (void *) this);
}