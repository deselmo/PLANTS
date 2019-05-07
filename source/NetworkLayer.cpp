#include "NetworkLayer.h"
#include "MicroBitDevice.h"


NetworkLayer::NetworkLayer(MicroBit* uBit) {
    this->uBit = uBit;
    this->mac_layer = new MacLayer(uBit);
}

void NetworkLayer::init() {
    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_RECEIVED, this, &NetworkLayer::recv_from_mac);
    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_SENT, this, &NetworkLayer::packet_sent);
    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_TIMEOUT, this, &NetworkLayer::packet_timeout);

    fiber_add_idle_component(this);
}

void NetworkLayer::packet_sent(MicroBitEvent e) {
    this->type_last_sent = DD_NONE;
}

void NetworkLayer::packet_timeout(MicroBitEvent e) {
    switch(this->type_last_sent) {
        case DD_DATA:
        case DD_RT_ACK:
        case DD_JOIN: {
                // this packet are sent to rely, if fail it means the rt is broken
                this->rt_formed = false;
                this->rely = false;
                MicroBitEvent evt(NETWORK_LAYER, NETWORK_LAYER_RT_BROKEN); 
            }
        case DD_COMMAND: 
        case DD_RT_INIT:
        default: 
            break;
    }
    this->type_last_sent = DD_NONE;
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

    if(p == PacketBuffer::EmptyPacket)
        return;

    DDPacketHeader ph = *((DDPacketHeader*)(p.getBytes()));
    if(ph.type != DD_DATA) {
        //it's a packet of the routing protocol
        switch(ph.type) {
            case DD_COMMAND:
                // devo modificare i "parametri di sensing", evento per il livello applicazione?
                MicroBitEvent evt(NETWORK_LAYER, NETWORK_LAYER_COMMAND_RECEIVED); 
                break;
            case DD_JOIN:
                break;
            case DD_RT_ACK:
                if(am_sink)
                    // devo aggiornare la routing table
                    delete &p;
                else {                
                    uint8_t count = *(p.getBytes()+sizeof(ph));
                    // if is an old ack, discard it
                    if(count<this->broadcast_count) 
                        delete &p;
                    // forward to rely
                    this->send_to_mac(p, this->rely);
                }
                break;
            case DD_RT_INIT:
                uint8_t count = *(p.getBytes()+sizeof(ph));
                // I am sink or I have already processed this packet
                if(count<=this->broadcast_count) 
                    delete &p;
                else {
                    // it's a refresh
                    this->broadcast_count = count;
                    this->rt_formed = true;
                    this->rely = ph.source;
                    // forwarding rt init
                    this->send_to_mac(p, NETWORK_BROADCAST);
                    // sending rt ack
                    this->send_ack(count);
                }
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
