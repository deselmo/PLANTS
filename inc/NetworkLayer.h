#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "MicroBit.h"
#include "MacLayer.h"
#include "ManagedBuffer.h"
#include "SerialCom.h"

#define NETWORK_LAYER 130
#define NETWORK_LAYER_INTERNALS 131

#define NETWORK_BROADCAST 0
#define NETWORK_LAYER_DD_RT_INIT_INTERVAL_FAST 500 // 2 second
#define NETWORK_LAYER_DD_RT_INIT_INTERVAL      60000 // 1 minute
#define NETWORK_LAYER_DD_JOIN_REQUEST_INTERVAL 1000  // 1 second


// PUBLIC EVENTS
enum {
    /**
     * Event raised when a new packet is received:
     * a sink receives a DD_DATA
     * a node receives a DD_COMMAND
     */
    NETWORK_LAYER_PACKET_RECEIVED = 1,

    /**
     * Event raised:
     * - in sink mode when a node logically connects or loses connection to the sink
     * - in not sink mode when the node logically connects or loses connection to a sink
     */
    NETWORK_LAYER_CONNECTION_UPDATE,
};

struct DDConnection {
    bool     status;
    uint32_t address;

    bool operator==(const DDConnection&);
    bool operator!=(const DDConnection&);

    static DDConnection Empty;
    bool isEmpty();
};


// Private events, DO NOT USE
enum {
    NETWORK_LAYER_PACKET_READY_TO_SEND = 1,
    NETWORK_LAYER_SERIAL_ROUTING_TABLE = 2,
    NETWORK_LAYER_SERIAL_READY_TO_SEND,
    NETWORK_LAYER_INCREMENT_BROADCAST_COUNTER
};


enum DDType {
    DD_RT_INIT,         // broadcast
    DD_DATA,            // to rely,
    DD_COMMAND,         // to forward list
    DD_RT_ACK,          // to rely with list of traversed nodes
    DD_JOIN,            // (to rely) if (rt_formed) else (to broadcast)
    DD_LEAVE,           // to rely, contain a node id and the broadcast counter
    DD_CONNECTION_LOST, // to broadcast, connection loss alert, used from subtree node
};


enum DDSend_state {
    DD_READY_TO_SEND,
    DD_WAIT_TO_BROADCAST,
    DD_WAIT_TO_SINK,
    DD_WAIT_TO_SUBTREE,
};


enum DDSerialSendState {
    DD_SERIAL_READY_TO_SEND,
    DD_SERIAL_SEND_CLEAR,
    DD_SERIAL_SEND_GET,
    DD_SERIAL_SEND_PUT,
};


enum DDSerialMode {
    DD_SERIAL_GET       = 0,
    DD_SERIAL_PUT       = 1,
    DD_SERIAL_CLEAR     = 2,
    DD_SERIAL_INIT      = 3,
    DD_SERIAL_INIT_ACK  = 4,
};

struct DDPacketHeader;
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

struct DDPacketHeader {
    DDType   type;
    uint32_t source;
    uint16_t network_id;
    uint32_t forward;
    uint32_t origin;
    uint32_t length;
};

struct DDPacket {
    DDPacketHeader header;
    ManagedBuffer  payload;


    static DDPacket of(
        NetworkLayer*,
        DDPacketData
    );


    bool extractRT_INIT(uint64_t& broadcast_counter);

    bool extractRT_ACK(DDNodeRoute&, uint64_t& broadcast_counter);

    bool extractCOMMAND(DDNodeRoute&, ManagedBuffer& payload);

    bool extractLEAVE(uint32_t& address);


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
    MacLayer mac_layer;
    SerialCom *serial;

    std::queue<DDPacket> outBufferPackets;

    // DD_DATA packets waiting to be processed by application level
    std::queue<ManagedBuffer> inBufferPackets;

    // Queue of new node connected with the respective broadcast number
    std::queue<DDConnection> inBufferNodes;

    const uint32_t network_id;
    bool     sink_mode;
    uint32_t source;

    bool debug;

    volatile uint64_t broadcast_counter = 0;
    volatile bool     rt_formed = false;
    volatile uint32_t rely      = 0;
    volatile bool fast_rt_init  = false;

    // time of the last DD_RT_INIT sent
    volatile uint64_t time_last_operation = 0;

    // initial send_state Send instance variable
    volatile DDSend_state send_state = DD_READY_TO_SEND;
    volatile uint32_t     sending_to = 0;


    // needed for the serial
    std::queue<ManagedBuffer> outSerialPackets;
    ManagedBuffer serial_received_buffer;
    ManagedBuffer serial_packet_in_send;
    volatile bool serial_initiated = false;
    DDSerialSendState serial_send_state = DD_SERIAL_READY_TO_SEND;
    ManagedBuffer serial_send_get_payload;
    uint32_t serial_send_put_origin = 0;
    volatile bool serial_wait_route_found = false;
    volatile bool serial_route_found = false;


    // avoid_rt_init_from
    bool avoid_rt_init_from_enabled = false;
    uint32_t avoid_rt_init_from_address = 0;


    private:
        // event functions
        void send_to_mac(MicroBitEvent);
        void packet_sent(MicroBitEvent); // evt handler for MAC_LAYER_PACKET_SENT
        void packet_timeout(MicroBitEvent);
        void recv_from_mac(MicroBitEvent); // evt handler for MAC_LAYER_PACKET_RECEIVED events

        void send_to_serial(MicroBitEvent);
        void recv_from_serial(ManagedBuffer);
        void recv_from_serial(MicroBitEvent);


        // utility functions
        bool elapsed_from_last_operation(uint64_t elapsed_time);

        // method called when the node lost the connection to its rely
        void rt_disconnect();

        // method called when the node lost the connection to its rely
        void rt_connect(uint64_t broadcast_counter, uint32_t rely);

        
        void incr_broadcast_counter(MicroBitEvent);

        void get_store_broadcast_counter();
        void put_store_broadcast_counter();

        // serial comunication
        void serial_wait_init();

        void serial_get_node_route(uint32_t destination);
        void serial_put_node_route(DDNodeRoute, uint32_t destination);
        void serial_clear_node_routes();
        void serial_send(ManagedBuffer payload);

        inline void serial_send_debug(ManagedBuffer payload);


        // send functions
        void send_data(ManagedBuffer payload);
        void send_data(ManagedBuffer payload, uint32_t origin);
        void send_rt_init();
        void send_rt_ack();
        void send_rt_ack(DDNodeRoute, uint32_t origin);
        void send_join_request();
        void send_join_request(uint32_t origin);
        void send_command(
            DDNodeRoute,
            ManagedBuffer payload,
            uint32_t destination,
            uint32_t origin
        );
        void send_command(ManagedBuffer payload, DDNodeRoute, uint32_t destination);
        void send_leave();
        void send_leave(ManagedBuffer payload, uint32_t origin);
        void send_connection_lost();


    public:
        NetworkLayer(
            MicroBit* uBit,
            uint16_t network_id,
            int transmitPower = 0
        );

        void init(
            SerialCom* serial = NULL,
            bool sink_mode = false,
            bool debug = false
        );

        uint16_t get_network_id() { return this->network_id; };
        uint32_t get_source() { return this->source; };

        // send to the rely, only for normal nodes
        bool send(ManagedBuffer);

        // if we know a path, only sinks
        bool send(ManagedBuffer, uint32_t destination);

        // interface provided to application layer
        ManagedBuffer recv();

        // interface provided to application layer
        DDConnection recv_connection();

        void avoid_rt_init_from(uint32_t address);

        virtual void idleTick();
};

#endif
