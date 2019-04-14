#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "MicroBit.h"
#include "MacLayer.h"
#include <map>
#include <vector>

#define NETWORK_BROADCAST 0

#define NETWORK_NET_ID 42

#define NETWORK_LAYER 421



/**
  * Mac Layer packet
  *    
  *     1         4           4          2           21
  * --------------------------------------------------------
  * | type | destination | source | seq control | payload |
  * --------------------------------------------------------
  * 
  * type -> uint8_t specify the type of the message
  *      0x00: Data packet
  *      0x01: Ack packet
  * 
  * destination -> uint32_t specify the mac level address of the receiver 
  * (the microbit serial number) if this is equals to 0x00 then the message
  * is sent in brodcast and no ack is expected
  * 
  * source -> uint32_t specify the mac level address of the sender
  * (the microbit serial number)
  * 
  * seq control -> uint16_t used for fragmentation
  *       divided in 3 fields
  *       4 bits are the fragment number
  *       1 bit  0 no more packets 1 more packet
  *       6 bits are the sequence number
  *       5 bits are the length of the payload in byte
  *       
  * 
  * length -> uint8_t specify the length of the payload in byte up to 255 byte
  * 
  * payload -> the payload.
  * 
  * with this organization and with the fact that 
  * MAX_PACKET_SIZE is 32 we have occupy 11 bytes for the header and we
  * can send up to to 21 bytes at once
  * 
  */


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
    std::vector<PacketBuffer *> inBuffer;
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
    PacketBuffer recv();
    
    virtual void systemTick();
};



#endif
