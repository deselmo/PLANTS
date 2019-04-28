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

/**
  * Retreives packet payload data into NetworLayer::inBuffer.
  *
  * If a data packet is already available, then it will be returned immediately to the caller 
  * (application layer) in the form of a PacketBuffer.
  *
  * @return the data received, or an empty PacketBuffer if no data is available.
  */
PacketBuffer NetworkLayer::recv() {
    if(inBuffer.empty())
        return PacketBuffer::EmptyPacket;
    else {
        PacketBuffer p = inBuffer.front();
        inBuffer.pop();
        return p;
    }
    

}

void NetworkLayer::recv_from_mac(MicroBitEvent e) {
    PacketBuffer p = mac_layer->recv();

    DDPacketHeader ph = *((DDPacketHeader*)(p.getBytes()));
    if(ph.type != DD_DATA) {
        //it's a packet of the routing protocol
        switch(ph.type) {
            case DD_COMMAND:
                // devo modificare i "parametri di sensing", evento per il livello applicazione?
                break;
            case DD_JOIN:
                break;
            case DD_RT_ACK: 
                break;
            case DD_RT_INIT:
                // se sono il sink o se ho gia processato questo init devo ignorarlo
                // confronto il broadcast count mio e del pacchetto
                // se questo pacchetto è nuovo memorizzo chi me l'ha mandato come rely!!
                break;
            default: 
                ;
        }
    }
    else {
        // application level packet
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
