#ifndef MAC_LAYER_H
#define MAC_LAYER_H

#include "MicroBit.h"
#include "ManagedBuffer.h"
#include "SerialCom.h"
#include <map>
#include <vector>

// definition of retransmission parameters
#define MAC_LAYER_RETRANSMISSION_ATTEMPT 5  // number of time we try to resend a packet
#define MAC_LAYER_RETRANSMISSION_TIME 200   // delay between two retransmission

//result codes
#define MAC_LAYER_PACKET_TOO_LARGE 1        // the payload of the packet was too large
#define MAC_LAYER_OK 0                      // the operation was ok

// definition of packet payload
#define MAC_LAYER_PAYLOAD_LENGTH 116               // max length of a single packet payload
#define MAC_LAYER_HEADER_SIZE 12
#define MAC_LAYER_MAX_POSSIBLE_PAYLOAD_LENGTH 29580   // max length of a upper layer payload

// events ids values
#define MAC_LAYER_RADIO_ERROR 1             // error when sending a packet through the radio
#define MAC_LAYER_PACKET_READY_TO_SEND 2    // there is a packet ready to be sent through the radio
#define MAC_LAYER_PACKET_SENT 3             // a packet has been sent through the radio
#define MAC_LAYER_TIMEOUT 4                 // a packet has timed out and has been dropped
#define MAC_LAYER_PACKET_RECEIVED 5         // a packet has been received

#define MAC_LAYER_PROTOCOL 5                // the protocol that is used when sending a packet through the radio


#define MAC_LAYER 120                       // mac layer busEvent id

/**
  * Mac Layer packet
  *    
  *     1         4          4       1         1          1        116
  * ----------------------------------------------------------------------
  * | type | destination | source | frag | seq_number | control | payload |
  * ----------------------------------------------------------------------
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
  * frag -> uint8_t specify the fragment number
  * 
  * seq_number -> unit8_t specify the sequence number
  * 
  * control -> uint8_t divided in 2 field
  *     1 bit 0 no more packets 1 more packets
  *     7 bits length of the payload in byte
  * 
  * payload -> the payload.
  * 
  * with this organization and with the fact that 
  * MAX_PACKET_SIZE is 32 we have occupy 11 bytes for the header and we
  * can send up to to 21 bytes at once
  * 
  */

// a packet of the mac layer protocol

struct Payload;
struct MacBuffer;

struct ManagedMacBuffer{
    ManagedBuffer b;

    static ManagedMacBuffer of(MacBuffer&);

    bool operator==(const ManagedMacBuffer& other);

    bool queued();
    void queued(bool);

    void timestamp(uint32_t);

    

    uint8_t type();
    uint8_t attempt();
    void attempt(uint8_t);
    

    uint32_t destination();
    uint32_t source();
    uint8_t frag();
    uint8_t seq_number();
    uint8_t control();
    Payload payload();
};

struct Payload{
    uint8_t payload[MAC_LAYER_PAYLOAD_LENGTH];
};

struct MacBuffer{
    bool queued;                                // is the packet queued for transmission
    uint64_t timestamp;                         // when the packet has been sent the last time
    uint8_t attempt;                            // number of time the packets has been sent 

    uint8_t type;                               // the type of the packet
    uint32_t destination;                       // the mac address of the destination of the packet
    uint32_t source;                            // the mac address of the source
    uint8_t frag;                               // the fragment number
    uint8_t seq_number;                         // the sequence number
    uint8_t control;                            // the control value
    Payload payload;  // the payload of this packet
};

struct ManagedFragmentedPacket{
    ManagedBuffer b;

    static ManagedFragmentedPacket Empty; // TODO init to all 0s

    bool operator==(const ManagedFragmentedPacket& other);

    static ManagedFragmentedPacket of(FragmentedPacket);

    uint16_t length();
    std::vector<ManagedMacBuffer> packets();

};

struct FragmentedPacket{
    

    uint64_t timestamp;
    bool lastReceived;
    uint8_t packet_number;
    uint16_t length;
    std::vector<ManagedMacBuffer> packets;
};

struct ManagedMacBufferFragmentReceived{
    ManagedBuffer b;

    static ManagedMacBufferFragmentReceived Empty; // TODO init to all 0s

    bool operator==(const ManagedMacBufferFragmentReceived& other);


    static ManagedMacBufferFragmentReceived of(MacBufferFragmentReceived);
};

struct MacBufferFragmentReceived{

    uint32_t source;
    std::map<uint8_t, ManagedFragmentedPacket>fragmented;
};

struct ManagedMacBufferSent{
    ManagedBuffer b;
    
    static ManagedMacBufferSent of(MacBufferSent);
};

struct MacBufferSent{
    uint8_t seq_number;
    uint8_t total;
    uint8_t received;
};

class MacLayer : public MicroBitComponent{


    std::vector<ManagedMacBuffer> outBuffer;                 // list of outgoing packets
    std::vector<ManagedBuffer> inBuffer;               // list of completely received messages
    MicroBit *uBit;                                     // Microbit
    uint8_t seq_number;                               // personal sequence number from 0-63
    std::map<uint32_t, ManagedMacBufferFragmentReceived> receive_buffer; 
    std::map<uint16_t, ManagedMacBuffer> waiting;            // packets sent which we are waiting for an ack
    std::map<uint8_t, ManagedMacBufferSent> waiting_for_ack;
    std::vector<uint32_t> disconnected_destination;
    SerialCom* serial;

    /**
     * Create a list of MacBuffer from the buffer passed the list
     * represent a fragmented payload
     * 
     * @param  buffer the payload of the packets
     * 
     * @param len the length of the buffer
     * 
     * @param the destination mac address of the packets
     * 
     * @param last pointer which will be set to the last element of the list
     * 
     * @return the vector which contains the fragments
     */
    vector<ManagedMacBuffer> prepareFragment(uint8_t *buffer, int len, uint32_t dest);
    
    /**
     * This function create a new MacBuffer with the arguments passed
     * 
     * @param type the type of the packet
     * 
     * @param dest the mac address of the destination
     * 
     * @param len the length in byte of the payload between 0 and 21
     * 
     * @param buffer the payload of the message
     * 
     * @param frag the fragment number of this message
     * 
     * @param inc indicates if we have to increment the sequence number after creating this packet
     * 
     * @return the new initialized MacBuffer
     */
    ManagedMacBuffer createMacBuffer(uint8_t type, uint32_t dest, int len, uint8_t *buffer, uint8_t frag, bool inc);
   
    /**
     * Prepare the control bits of a packet and increment the sequence number
     * (if necessary)
     * 
     * @param len value must be between 0 and 120
     * 
     * @param inc indicates if we need also to increment the sequence number
     *        if is false the sequence number won't be incremented and is
     *        expected to receive more packet for this sequence number
     *        this parameter is also used to set the "more packets" field
     * 
     * @return the control field after being set with the correct values
     */
    uint8_t setControl(int len, bool inc);
    
    /**
     * Function to be invoked when an event MAC_LAYER_PACKET_READY_TO_SEND
     * is fired.
     * 
     * This function retrieve one packet from the list of outgoing packet
     * and transmits it over the radio. 
     * 
     * This function waits for the packet to be sent before returning 
     * 
     */
    void send_to_radio(MicroBitEvent);
    void recv_from_radio(MicroBitEvent);

    /**
     * This function add to the queue of the outgoing packets an ack packet
     * that is the response for the received packet
     * 
     * @param received the MacBuffer to ackwnoledge
     */
    void sendAck(ManagedMacBuffer received);

    /**
     * This function checks if the received MacBuffer is present in the 
     * list of recently received packets if so update the timestamp of the
     * packet
     * 
     * @param received the MacBuffer received to be checked
     */
    bool checkRepetition(ManagedMacBuffer received);

    /**
     * This function adds the received MacBuffer to the list of
     * recently received packets this list is used to ignore retransmitted packets
     * 
     * @param received the MacBuffer received
     */
    void addToRecentlyReceived(ManagedMacBuffer received);
    void addToFragmented(ManagedMacBuffer fragment);
    void addToDataReady(ManagedFragmentedPacket buf);
    void accomplishAck(ManagedMacBuffer ack);
    bool isComplete(ManagedFragmentedPacket fragmented);
    bool isLast(ManagedMacBuffer fragment);
    void orderPackets(ManagedFragmentedPacket fragmented);


    /**
     * This function retrieve the length field from a control bits
     * 
     * @param control the control bits
     * 
     * @return the length bits as an uint8_t
     */
    uint8_t getLength(uint8_t control);

    /**
     * This function test if this packet represent the last fragment
     * 
     * @param control the control bits
     * 
     * @return true if the last fragment flag is set false otherwise
     */
    bool isLastFragment(uint8_t control);

    /**
     * This function get the hash of a packet
     * concatenating the frag number and the sequence number
     * 
     * @param buf the MacBuffer to get the hash
     * 
     * @return the hash of the packet
     */
    uint16_t getHash(ManagedMacBuffer buf);

public:

    //debug
    int macbufferallocated;
    int maccontroldata;
    int macbuffersent;
    int macbufferreceived;
    int macbufferfragmentedreceived;
    int fragmentedpacket;

    MacLayer(MicroBit* uBit, SerialCom* serial);
    void init();

    int send(uint8_t *buffer, int len, uint32_t dest);
    int send(ManagedBuffer data, uint32_t dest);
    int send(ManagedString s, uint32_t dest);
    uint32_t getDisconnectedDestination();
    ManagedBuffer recv();
    
    int getPacketsInQueue();
    void printToSerial(ManagedString s);
    virtual void idleTick();
};



#endif