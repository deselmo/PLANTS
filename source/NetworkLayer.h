#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include <map>
#include <queue>

#include "MicroBit.h"
#include "MacLayer.h"
#include "ManagedBuffer.h"
#include "SerialCom.h"

#define NETWORK_LAYER 43

#define NETWORK_LAYER_DD_RT_INIT_INTERVAL      60000 // 1 minute
#define NETWORK_LAYER_DD_JOIN_REQUEST_INTERVAL 1000  // 1 second

enum {
    NETWORK_BROADCAST,
    NETWORK_LAYER_PACKET_RECEIVED,
    NETWORK_LAYER_COMMAND_RECEIVED,
    NETWORK_LAYER_RT_BROKEN,
    NETWORK_LAYER_PACKET_READY_TO_SEND,
    NETWORK_LAYER_NODE_CONNECTED,
    NETWORK_LAYER_SERIAL_ROUTING_TABLE,
};


enum DDType {
    DD_RT_INIT, // broadcast
    DD_DATA,    // to rely,
    DD_COMMAND, // to forward list
    DD_RT_ACK,  // to rely with list of traversed nodes
    DD_JOIN,    // (to rely) if (rt_formed) else (to broadcast)
    DD_LEAVE,   // to rely, contain a node id and the broadcast counter
};


enum DDSend_state {
    DD_READY_TO_SEND,
    DD_WAIT_TO_BROADCAST,
    DD_WAIT_TO_SINK,
    DD_WAIT_TO_SUBTREE,
};


enum DDSerialMode {
    DD_SERIAL_GET,
    DD_SERIAL_PUT,
    DD_SERIAL_CLEAR,
};


struct DDPacketData;
struct DDPacket;
struct DDNodeRoute;
struct DDPayloadWithNodeRoute;
class  NetworkLayer;


struct DDPacketData {
    const DDType   type;
    const uint32_t forward;
    const uint32_t origin;
    ManagedBuffer  payload;
};


struct DDPacket {
    DDType   type;
    uint32_t source;
    uint16_t network_id;
    uint32_t forward;
    uint32_t origin;
    uint32_t length;
    ManagedBuffer  payload;


    static DDPacket of(
        NetworkLayer*,
        DDPacketData
    );


    bool extractRT_INIT(uint64_t& broadcast_counter);

    bool extractRT_ACK(DDNodeRoute&, uint64_t& broadcast_counter);

    bool extractCOMMAND(DDNodeRoute&, ManagedBuffer& payload);

    bool extractLEAVE(uint32_t& address, uint64_t& broadcast_number);


    bool operator==(const DDPacket&);
    bool operator!=(const DDPacket&);

    static DDPacket Empty;
    bool isEmpty();

    ManagedBuffer toManagedBuffer();
    static DDPacket fromManagedBuffer(ManagedBuffer);
};


struct DDNodeRoute {
    uint32_t      size;
    ManagedBuffer addresses;


    static DDNodeRoute of(
        uint32_t address
    );

    static DDNodeRoute of(
        uint32_t* address_array,
        uint32_t  size
    );


    uint32_t* get_address_array();

    // add a new node at the start of the contained node list
    void push(uint32_t address);

    // return and remove the first node at the start of the contained node list
    // if no node is contained, then return NULL
    uint32_t take();

    bool operator==(const DDNodeRoute&);
    bool operator!=(const DDNodeRoute&);

    static DDNodeRoute Empty;
    bool isEmpty();

    ManagedBuffer toManagedBuffer();
    static DDNodeRoute fromManagedBuffer(ManagedBuffer);
};


struct DDPayloadWithNodeRoute {
    DDNodeRoute   node_route;
    uint32_t      length;
    ManagedBuffer payload;

    static DDPayloadWithNodeRoute of(
        DDNodeRoute   node_route,
        ManagedBuffer payload
    );

    bool operator==(const DDPayloadWithNodeRoute&);
    bool operator!=(const DDPayloadWithNodeRoute&);

    static DDPayloadWithNodeRoute Empty;
    bool isEmpty();

    ManagedBuffer toManagedBuffer();
    static DDPayloadWithNodeRoute fromManagedBuffer(ManagedBuffer);
};


class NetworkLayer : public MicroBitComponent {
    MicroBit *uBit;
    SerialCom *serial;
    MacLayer mac_layer;

    std::queue<DDPacket> outBufferPackets;

    // DD_DATA packets waiting to be processed by application level
    std::queue<ManagedBuffer> inBufferPackets;

    // Queue of new node connected with the respective broadcast number
    std::queue<std::tuple<bool, uint32_t, uint64_t>> inBufferNodes;

    const uint32_t network_id;
    const bool     sink_mode;
    const uint32_t source;


    volatile uint64_t broadcast_counter;
    volatile bool     rt_formed = false;
    volatile uint32_t rely      = 0;

    // time of the last DD_RT_INIT sent
    volatile uint64_t time_last_operation = 0;

    // initial send_state Send instance variable
    volatile DDSend_state send_state = DD_READY_TO_SEND;
    volatile uint32_t     sending_to = 0;


    // needed for the serial
    volatile bool serial_waiting = false;
    ManagedBuffer received_buffer = ManagedBuffer::EmptyPacket;


    private:
        // event functions
        void send_to_mac(MicroBitEvent);
        void packet_sent(MicroBitEvent); // evt handler for MAC_LAYER_PACKET_SENT
        void packet_timeout(MicroBitEvent);
        void recv_from_mac(MicroBitEvent); // evt handler for MAC_LAYER_PACKET_RECEIVED events

        void recv_from_serial(ManagedBuffer);


        // utility functions
        bool elapsed_from_last_operation(uint64_t elapsed_time);

        // method called when the node lost the connection to its rely
        void rt_disconnect();

        // method called when the node lost the connection to its rely
        void rt_connect(uint64_t broadcast_counter, uint32_t rely);

        
        void incr_broadcast_counter();

        void get_store_broadcast_counter();
        void put_store_broadcast_counter();

        // serial comunication
        bool get_serial_node_route(uint32_t destination, DDNodeRoute&);
        bool put_serial_node_route(DDNodeRoute);
        bool clear_serial_node_routes();


        // send functions
        void send_data(ManagedBuffer payload);
        void send_data(ManagedBuffer payload, uint32_t origin);
        void send_rt_init();
        void send_rt_ack();
        void send_rt_ack(DDNodeRoute, uint32_t origin);
        void send_join_request();
        void send_command(
            DDNodeRoute,
            ManagedBuffer payload,
            uint32_t destination,
            uint32_t origin
        );
        void send_command(ManagedBuffer payload, DDNodeRoute, uint32_t destination);
        void send_leave();
        void send_leave(ManagedBuffer payload, uint32_t origin);


    public:
        // create a new network layer not in sink_mode
        // uBit must be initialized with the init method
        NetworkLayer(
            MicroBit* uBit,
            uint16_t network_id
        );

        
        // create a new network layer in sink_mode
        // uBit and serial must be initialized with the init method
        NetworkLayer(
            MicroBit* uBit,
            uint16_t network_id,
            SerialCom* serial
        );
        
        void init();

        uint16_t get_network_id() { return this->network_id; };
        uint32_t get_source() { return this->source; };

        // send to the rely
        bool send(ManagedBuffer);

        // if we know a path, only the sink
        bool send(ManagedBuffer, uint32_t destination);

        // interface provided to application layer
        ManagedBuffer recv();

        // interface provided to application layer
        std::tuple<bool, uint32_t, uint64_t> recv_node();
        
        virtual void systemTick();
};

#endif
