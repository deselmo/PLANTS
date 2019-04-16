#include "NetworkLayer.h"
#include "MicroBitDevice.h"


NetworkLayer::NetworkLayer(MicroBit* uBit) {
    this->uBit = uBit;
    this->mac_layer = new MacLayer(uBit);
}

void NetworkLayer::init(){
    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_RECEIVED, this, &NetworkLayer::recv_from_mac);
    
    system_timer_add_component(this);
}

void NetworkLayer::recv_from_mac(MicroBitEvent e) {
    PacketBuffer p = mac_layer->recv();

    DDPacketHeader ph = *((DDPacketHeader*)(p.getBytes()));
    if(ph.type != DD_DATA) {
        switch(ph.type) {
            case DD_COMMAND:
                break;
            case DD_JOIN:
                break;
            case DD_RT_ACK: 
                break;
            case DD_RT_INIT:
                break;
            default: 
                ;
        }
    }
    else {
        uint8_t *payload = p.getBytes()+sizeof(ph);
        this->inBuffer.push(PacketBuffer(payload, ph.length));
        MicroBitEvent evt(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED); 
    }
    

}


int NetworkLayer::send(uint8_t *buffer, int len){
    DDPacket dd_packet;

    if(am_sink) {

    } else {
        dd_packet = {
            .header = {
                .type = DD_DATA,
                .network_id = NETWORK_NET_ID,
                .source = microbit_serial_number(),
                .forward = rely,
                .origin = microbit_serial_number(),
                .length = len
            },
            .payload = buffer
        };
    }

    PacketBuffer packet(sizeof(dd_packet.header) + dd_packet.header.length);

    uint8_t *payload = packet.getBytes();
    
    memcpy(
        payload,
        &(dd_packet.header),
        sizeof(dd_packet.header)
    );
    memcpy(
        payload + sizeof(dd_packet.header),
        &(dd_packet.payload),
        dd_packet.header.length
    );

    ////////////////////////////////////////////////
    mac_layer->send(packet, dd_packet.header.forward);
}
