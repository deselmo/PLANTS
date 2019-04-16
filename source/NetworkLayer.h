#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "MicroBit.h"
#include "MacLayer.h"
#include <map>
#include <vector>
#include <queue>

#define NETWORK_BROADCAST 0

#define NETWORK_NET_ID 42

#define NETWORK_LAYER 421
#define NETWORK_LAYER_PACKET_RECEIVED 23



enum DDtype{
    DD_RT_INIT,
    DD_DATA,
    DD_COMMAND,
    DD_RT_ACK,
    DD_JOIN
};


struct DDPacketHeader{
    DDtype type;
    uint32_t network_id;
    uint32_t source;
    uint32_t forward;
    uint32_t origin;
    uint16_t length;
};


struct DDPacket{
    DDPacketHeader header;
    uint8_t *payload;
};


class NetworkLayer : public MicroBitComponent{
    std::vector<DDPacket *> outBuffer;
    std::queue<PacketBuffer> inBuffer; // DD_DATA packets waiting to be processed by application level
    MicroBit *uBit;
    MacLayer *mac_layer;
    bool am_sink;
    uint32_t network_id;
    uint32_t broadcast_count;
    uint32_t rely;
    bool rt_formed;


public:
    NetworkLayer(MicroBit* uBit);
    void init();

    int send(uint8_t *buffer, int len);
    int send(PacketBuffer data);
    int send(ManagedString s);

    void recv_from_mac(MicroBitEvent e); // evt handler for MAC_LAYER_PACKET_RECEIVED events

    PacketBuffer recv(); // interface provided to application layer
    
    virtual void systemTick();
};



#endif
