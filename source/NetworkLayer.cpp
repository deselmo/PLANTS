#include "NetworkLayer.h"
#include <inttypes.h>

// DDPacket {
    DDPacket DDPacket::Empty = DDPacket {};

    DDPacket DDPacket::of(
        NetworkLayer* network_layer,
        DDPacketData  dd_packet_data
    ) {
        return DDPacket {
            .header = {
                .type = dd_packet_data.type,
                .source = network_layer->get_source(),
                .network_id = network_layer->get_network_id(),
                .forward = dd_packet_data.forward,
                .origin = dd_packet_data.origin,
                .length = dd_packet_data.payload.length(),
            },
            .payload = dd_packet_data.payload
        };
    }


    // extractor methods
    bool DDPacket::extractRT_INIT(uint64_t& broadcast_counter) {
        if (this->header.type != DD_RT_INIT ||
            this->payload.length() != sizeof(uint64_t)
        ) {
            return false;
        }
        broadcast_counter = * (uint64_t*) this->payload.getBytes();

        return true;
    }

    bool DDPacket::extractRT_ACK(DDNodeRoute& node_route, uint64_t& broadcast_counter) {
        if (this->header.type != DD_RT_ACK) {
            return false;
        }

        DDPayloadWithNodeRoute payload_with_node_route =
            DDPayloadWithNodeRoute::fromManagedBuffer(this->payload);

        if(payload_with_node_route.payload.length() != sizeof(uint64_t)) {
            return false;
        }
        broadcast_counter = * (uint64_t*) payload_with_node_route.payload.getBytes();

        node_route = payload_with_node_route.node_route;

        return !node_route.isEmpty();
    }

    bool DDPacket::extractCOMMAND(DDNodeRoute& node_route, ManagedBuffer& payload) {
        if (this->header.type != DD_COMMAND) {
            return false;
        }

        DDPayloadWithNodeRoute payload_with_node_route =
            DDPayloadWithNodeRoute::fromManagedBuffer(this->payload);

        if(payload_with_node_route.isEmpty()) {
            return false;
        }

        node_route = payload_with_node_route.node_route;

        payload = payload_with_node_route.payload;

        return true;
    }

    bool DDPacket::extractLEAVE(uint32_t& address) {
        if (this->header.type != DD_LEAVE ||
            this->payload.length() != sizeof(uint32_t)
        ) {
            return false;
        }

        address = * (uint32_t*) this->payload.getBytes();
        return true;
    }

    bool DDPacket::operator==(const DDPacket& other) {
        return this->header.type       == other.header.type       &&
               this->header.source     == other.header.source     &&
               this->header.network_id == other.header.network_id &&
               this->header.forward    == other.header.forward    &&
               this->header.origin     == other.header.origin     &&
               this->header.length     == other.header.length     &&
               this->payload    == other.payload;
    }
    bool DDPacket::operator!=(const DDPacket& other) {
        return !(*this==other);
    }

    bool DDPacket::isEmpty() {
        return *this == DDPacket::Empty;
    }

    ManagedBuffer DDPacket::toManagedBuffer() {
        if(*this==DDPacket::Empty)
            return ManagedBuffer::EmptyPacket;

        ManagedBuffer packet(sizeof(DDPacketHeader) + this->payload.length());

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(this->header);
        memcpy(
            packet.getBytes() + offset,
            &this->header,
            length); offset += length;

        length = this->payload.length();
        memcpy(
            packet.getBytes() + offset,
            this->payload.getBytes(),
            length); offset += length;

        return packet;
    }
    DDPacket DDPacket::fromManagedBuffer(ManagedBuffer packet) {
        DDPacket dd_packet;

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(dd_packet.header);
        memcpy(
            &dd_packet.header,
            packet.getBytes() + offset,
            length); offset += length;


        length = dd_packet.header.length;
        if(packet.length() < offset + length) return DDPacket::Empty;
        dd_packet.payload = ManagedBuffer(
            packet.getBytes() + offset,
            length); offset += length;

        return dd_packet;
    }
// }


// DDNodeRoute {
    DDNodeRoute DDNodeRoute::Empty = DDNodeRoute {};


    // factory functions
    DDNodeRoute DDNodeRoute::of(
        uint32_t      address
    ) {
        return DDNodeRoute {
            .size = 1,
            .addresses = ManagedBuffer(
                (uint8_t*) &address,
                sizeof(address)
            )
        };
    }

    DDNodeRoute DDNodeRoute::of(
            uint32_t*     address_array,
            uint32_t      size
    ) {
        return DDNodeRoute {
            .size = size,
            .addresses = ManagedBuffer(
                (uint8_t*) address_array,
                size * sizeof(uint32_t)
            )
        };
    }


    void DDNodeRoute::push(uint32_t node_address) {
        ManagedBuffer new_addresses(sizeof(node_address) + this->addresses.length());

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(node_address);
        memcpy(
            new_addresses.getBytes() + offset,
            &node_address,
            length); offset += length;

        length = this->addresses.length();
        memcpy(
            new_addresses.getBytes() + offset,
            this->addresses.getBytes(),
            length); offset += length;

        this->size++;
        this->addresses = new_addresses;
    }
    uint32_t DDNodeRoute::take() {
        if(!(this->size > 0))
            return 0;

        uint32_t node_address = *(uint32_t*)(this->addresses.getBytes());

        this->size--;
        this->addresses = ManagedBuffer(
            this->addresses.getBytes() + sizeof(uint32_t),
            this->addresses.length() - sizeof(uint32_t)
        );

        return node_address;
    }
    uint32_t* DDNodeRoute::get_address_array() {
        return (uint32_t*) this->addresses.getBytes();
    }


    bool DDNodeRoute::operator==(const DDNodeRoute& other) {
        return this->size      == other.size      &&
               this->addresses == other.addresses;
    }
    bool DDNodeRoute::operator!=(const DDNodeRoute& other) {
        return !(*this==other);
    }

    bool DDNodeRoute::isEmpty() {
        return *this == DDNodeRoute::Empty;
    }

    ManagedBuffer DDNodeRoute::toManagedBuffer() {
        if(*this == DDNodeRoute::Empty ||
           this->addresses.length() != this->size * sizeof(uint32_t)) {
            return ManagedBuffer::EmptyPacket;
        }

        ManagedBuffer packet(
            sizeof(this->size) +
            this->addresses.length()
        );

        uint8_t *buffer = packet.getBytes();

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(this->size);
        memcpy(
            buffer + offset,
            &this->size,
            length); offset += length;

        length = this->addresses.length();
        memcpy(
            buffer + offset,
            this->addresses.getBytes(),
            length); offset += length;

        return packet;
    }
    DDNodeRoute DDNodeRoute::fromManagedBuffer(ManagedBuffer packet) {
        DDNodeRoute node_route;

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(node_route.size);
        memcpy(
            &node_route.size,
            packet.getBytes(),
            length); offset += length;

        length = node_route.size * sizeof(uint32_t);
        if(packet.length() < offset + length) return DDNodeRoute::Empty;
        node_route.addresses = ManagedBuffer(
            packet.getBytes() + offset,
            length); offset += length;

        return node_route;
    }
// }


// DDPayloadWithNodeRoute {
    DDPayloadWithNodeRoute DDPayloadWithNodeRoute::Empty = DDPayloadWithNodeRoute {};


    DDPayloadWithNodeRoute DDPayloadWithNodeRoute::of(
        DDNodeRoute   node_route,
        ManagedBuffer payload
    ) {
        return DDPayloadWithNodeRoute {
            .node_route = node_route,
            .length = payload.length(),
            .payload = payload
        };
    }


    bool DDPayloadWithNodeRoute::operator==(const DDPayloadWithNodeRoute& other) {
        return this->node_route == other.node_route &&
               this->length     == other.length     &&
               this->payload    == other.payload;
    }
    bool DDPayloadWithNodeRoute::operator!=(const DDPayloadWithNodeRoute& other) {
        return !(*this==other);
    }

    bool DDPayloadWithNodeRoute::isEmpty() {
        return *this == DDPayloadWithNodeRoute::Empty;
    }

    ManagedBuffer DDPayloadWithNodeRoute::toManagedBuffer() {
        if(*this == DDPayloadWithNodeRoute::Empty)
            return ManagedBuffer::EmptyPacket;;

        ManagedBuffer packet(
            sizeof(this->node_route.size) +
            this->node_route.addresses.length() +
            sizeof(this->length) +
            this->payload.length()
        );

        uint8_t *buffer = packet.getBytes();

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(this->node_route.size);
        memcpy(
            buffer + offset,
            &node_route.size,
            length); offset += length;

        length = this->node_route.addresses.length();
        memcpy(
            buffer + offset,
            this->node_route.addresses.getBytes(),
            length); offset += length;

        length = sizeof(this->length);
        memcpy(
            buffer + offset,
            &this->length,
            length); offset += length;

        length = this->payload.length();
        memcpy(
            buffer + offset,
            this->payload.getBytes(),
            length); offset += length;

        return packet;
    }
    DDPayloadWithNodeRoute DDPayloadWithNodeRoute::fromManagedBuffer(ManagedBuffer packet) {
        DDPayloadWithNodeRoute payload_with_node_route;

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(payload_with_node_route.node_route.size);
        if(packet.length() < offset + length) return DDPayloadWithNodeRoute::Empty;
        memcpy(
            &payload_with_node_route.node_route.size,
            packet.getBytes() + offset,
            length); offset += length;

        length = payload_with_node_route.node_route.size * sizeof(uint32_t);
        if(packet.length() < offset + length) return DDPayloadWithNodeRoute::Empty;
        payload_with_node_route.node_route.addresses = ManagedBuffer(
            packet.getBytes() + offset,
            length); offset += length;

        length = sizeof(payload_with_node_route.length);
        if(packet.length() < offset + length) return DDPayloadWithNodeRoute::Empty;
        memcpy(
            &payload_with_node_route.length,
            packet.getBytes() + offset,
            length); offset += length;

        length = payload_with_node_route.length;
        if(packet.length() < offset + length) return DDPayloadWithNodeRoute::Empty;
        payload_with_node_route.payload = ManagedBuffer(
            packet.getBytes() + offset,
            length); offset += length;

        return payload_with_node_route;
    }
// }


// DDConnection {
    DDConnection DDConnection::Empty = DDConnection {};

    bool DDConnection::operator==(const DDConnection& other) {
        return  this->status            == other.status &&
                this->address           == other.address;
    }

    bool DDConnection::operator!=(const DDConnection& other) {
        return !(*this==other);
    }

    bool DDConnection::isEmpty() {
        return *this == DDConnection::Empty;
    }
// }


// DDMessage {
    DDMessage DDMessage::Empty = DDMessage {};

    bool DDMessage::operator==(const DDMessage& other) {
        return  this->source  == other.source &&
                this->payload == other.payload;
    }

    bool DDMessage::operator!=(const DDMessage& other) {
        return !(*this==other);
    }

    bool DDMessage::isEmpty() {
        return *this == DDMessage::Empty;
    }
// }


// NetworkLayer {
    NetworkLayer::NetworkLayer(
        MicroBit*  uBit,
        uint16_t   network_id,
        int transmitPower
    )
        : uBit                (uBit)
        , mac_layer           (MacLayer(
                                    uBit,
                                    NULL,
                                    transmitPower)
                                )
        , network_id          (network_id)
    {};


    void NetworkLayer::init(
        SerialCom* serial,
        bool sink_mode,
        bool debug
    ) {
        this->serial = serial;
        this->sink_mode = sink_mode;
        this->debug = debug;

        this->uBit->messageBus.listen(
            MAC_LAYER,
            MAC_LAYER_PACKET_RECEIVED,
            this, &NetworkLayer::recv_from_mac
        );

        this->uBit->messageBus.listen(
            NETWORK_LAYER_INTERNALS,
            NETWORK_LAYER_PACKET_READY_TO_SEND,
            this, &NetworkLayer::send_to_mac
        );

        this->uBit->messageBus.listen(
            MAC_LAYER,
            MAC_LAYER_PACKET_SENT,
            this, &NetworkLayer::packet_sent
        );

        this->uBit->messageBus.listen(
            MAC_LAYER,
            MAC_LAYER_TIMEOUT,
            this, &NetworkLayer::packet_timeout
        );

        if(this->serial) {
            this->uBit->messageBus.listen(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                this, &NetworkLayer::recv_from_serial
            );

            this->serial->addListener(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                this, &NetworkLayer::recv_from_serial
            );

            this->uBit->messageBus.listen(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_READY_TO_SEND,
                this, &NetworkLayer::send_to_serial
            );


            this->serial_wait_init();
        }


        if(this->sink_mode) {
            this->get_store_broadcast_counter();

            this->uBit->messageBus.listen(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_INCREMENT_BROADCAST_COUNTER,
                this, &NetworkLayer::incr_broadcast_counter
            );
            
            this->serial_send_debug("broadcast counter retrieved:");
            this->serial_send_debug(ManagedBuffer((uint8_t*)&this->broadcast_counter, sizeof(this->broadcast_counter)));
        }


        this->mac_layer.init();

        this->source = microbit_serial_number();
        this->serial_packet_in_send = ManagedBuffer::EmptyPacket;
        this->serial_send_get_payload = ManagedBuffer::EmptyPacket;
        this->serial_received_buffer = ManagedBuffer::EmptyPacket;

        fiber_add_idle_component(this);
    }


    void NetworkLayer::idleTick() {
        if(this->sink_mode) {
            if(this->serial_send_state != DD_SERIAL_READY_TO_SEND) {
                return;
            }

            if(this->outSerialPackets.size()) {
                this->serial_send_debug("found serial message_to_send");

                DDSerialMode serial_mode = (DDSerialMode) this->outSerialPackets.front().getByte(0);

                switch(serial_mode){
                    case DD_SERIAL_GET:
                        this->serial_send_state = DD_SERIAL_SEND_GET;
                        break;
                    case DD_SERIAL_PUT:
                        this->serial_send_state = DD_SERIAL_SEND_PUT;
                        break;
                    case DD_SERIAL_CLEAR:
                        this->serial_send_debug("clear");
                        this->serial_send_state = DD_SERIAL_SEND_CLEAR;
                        break;
                    case DD_SERIAL_INIT:
                    case DD_SERIAL_INIT_ACK:
                        return;
                }
                
                MicroBitEvent(
                    NETWORK_LAYER_INTERNALS,
                    NETWORK_LAYER_SERIAL_READY_TO_SEND
                );

                return;
            }
        }


        if(this->sink_mode) {
            if( (this->fast_rt_init &&
                 this->elapsed_from_last_operation(NETWORK_LAYER_DD_RT_INIT_INTERVAL_FAST)
                ) || this->elapsed_from_last_operation(NETWORK_LAYER_DD_RT_INIT_INTERVAL)
            ) {
                this->serial_send_debug("timeout triggered");
                this->fast_rt_init = false;
                MicroBitEvent(NETWORK_LAYER_INTERNALS,
                    NETWORK_LAYER_INCREMENT_BROADCAST_COUNTER
                );
            }
        } else if(!this->rt_formed &&
                this->elapsed_from_last_operation(NETWORK_LAYER_DD_JOIN_REQUEST_INTERVAL)
        ) {
            this->send_join_request();
        }

        if(this->send_state == DD_READY_TO_SEND && this->outBufferPackets.size()) {
            switch(this->outBufferPackets.front().header.type) {
                case DD_DATA:
                case DD_RT_ACK:
                case DD_LEAVE:
                    this->send_state = DD_WAIT_TO_SINK;
                    break;
                
                case DD_COMMAND:
                    this->send_state = DD_WAIT_TO_SUBTREE;
                    break;

                case DD_CONNECTION_LOST:
                case DD_RT_INIT:
                    this->send_state = DD_WAIT_TO_BROADCAST;
                    break;

                case DD_JOIN:
                    if(this->rt_formed) {
                        this->send_state = DD_WAIT_TO_SINK;
                    } else {
                        this->send_state = DD_WAIT_TO_BROADCAST;
                    }
            }

            if(this->send_state != DD_READY_TO_SEND) {
                MicroBitEvent(
                    NETWORK_LAYER_INTERNALS,
                    NETWORK_LAYER_PACKET_READY_TO_SEND
                );
            }
        }
    }


    bool NetworkLayer::send(ManagedBuffer payload) {
        if(this->sink_mode || !this->rt_formed) return false;
        
        this->send_data(payload);
        return true;
    }

    bool NetworkLayer::send(ManagedBuffer payload, uint32_t destination) {
        if(!this->sink_mode) return false;

        this->serial_send_get_payload = payload;

        this->serial_wait_route_found = true;
        this->serial_get_node_route(destination);
        while(this->serial_wait_route_found) { this->uBit->sleep(50); }

        return this->serial_route_found;
    }


    /**
     * Retreives packet payload data into NetworLayer::inBuffer.
     *
     * If a data packet is already available, then it will be returned immediately to the caller 
     * (application layer) in the form of a ManagedBuffer.
     *
     * @return the data received, or an empty ManagedBuffer if no data is available.
     */
    DDMessage NetworkLayer::recv() {
        if(inBufferPackets.empty())
            return DDMessage::Empty;

        DDMessage message = inBufferPackets.front();
        inBufferPackets.pop();
        return message;
    }

    DDConnection NetworkLayer::recv_connection() {
        if(inBufferNodes.empty())
            return DDConnection::Empty;

        DDConnection connected_node = inBufferNodes.front();
        inBufferNodes.pop();

        return connected_node;
    }


    // event functions
    void NetworkLayer::recv_from_mac(MicroBitEvent) {
        ManagedBuffer received_mac_buffer = mac_layer.recv();

        DDPacket dd_packet = DDPacket::fromManagedBuffer(received_mac_buffer);

        if(dd_packet.header.network_id != this->network_id) {
            return;
        }

        if(this->sink_mode) {
            switch(dd_packet.header.type) {
                case DD_RT_INIT: {
                } break;

                case DD_DATA: {
                    if(dd_packet.header.forward != this->source) {
                        return;
                    }

                    this->serial_send_debug("RECV: DD_DATA");
                    this->inBufferPackets.push(DDMessage { 
                        .source  = dd_packet.header.origin,
                        .payload = dd_packet.payload
                    });
                    MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED); 

                } break;

                case DD_COMMAND: {
                    return;
                } break;

                case DD_RT_ACK: {
                    if(dd_packet.header.forward != this->source) {
                        return;
                    }

                    DDNodeRoute node_route;
                    uint64_t broadcast_counter;

                    if(!dd_packet.extractRT_ACK(node_route, broadcast_counter) ||
                            broadcast_counter != this->broadcast_counter) {
                        return;
                    }
                    this->serial_send_debug("RECV: DD_RT_ACK");

                    this->serial_put_node_route(node_route, dd_packet.header.origin);
                } break;

                case DD_JOIN: {
                    this->serial_send_debug("RECV: DD_JOIN");
                    if(!this->fast_rt_init) {
                        this->time_last_operation = system_timer_current_time();
                        this->fast_rt_init = true;
                    }
                } break;

                case DD_LEAVE: {
                    if(dd_packet.header.forward != this->source) {
                        return;
                    }

                    this->serial_send_debug("RECV: DD_LEAVE");
                    uint32_t disconnected_node_address;

                    if(!dd_packet.extractLEAVE(disconnected_node_address)) {
                        return;
                    }

                    this->inBufferNodes.push(DDConnection {
                        .status            = false,
                        .address           = disconnected_node_address,
                    });
                    MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE);
                } break;

                case DD_CONNECTION_LOST: {
                } break;
            }
        }
        else {
            switch(dd_packet.header.type) {
                case DD_RT_INIT: {
                    uint64_t broadcast_counter_received;
                    if (!dd_packet.extractRT_INIT(broadcast_counter_received) ||
                        broadcast_counter_received <= this->broadcast_counter) {
                        return;
                    }

                    this->serial_send_debug("RECV: DD_RT_INIT");

                    if(this->avoid_rt_init_from_enabled &&
                            this->avoid_rt_init_from_address == dd_packet.header.source) {
                        this->serial_send_debug("refused: connection rt_init connection avoid");
                        return;
                    }

                    this->rt_connect(broadcast_counter_received, dd_packet.header.source);
                    this->send_rt_ack();

                    this->send_rt_init();
                } break;

                case DD_DATA: {
                    if(dd_packet.header.forward != this->source ||
                       !this->rt_formed) {
                        return;
                    }
                    this->serial_send_debug("RECV: DD_DATA");
                    this->send_data(dd_packet.payload, dd_packet.header.origin);
                } break;

                case DD_COMMAND: {
                    if(dd_packet.header.forward != this->source) {
                        return;
                    }

                    DDNodeRoute node_route;
                    ManagedBuffer payload;

                    if(dd_packet.header.forward != this->source || 
                       !dd_packet.extractCOMMAND(node_route, payload)) {
                        return;
                    }

                    if(node_route.size == 0) {
                        this->serial_send_debug("RECV: DD_COMMAND, for me");
                        this->inBufferPackets.push(DDMessage {
                            .source  = dd_packet.header.origin,
                            .payload = payload
                        });
                        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED); 


                    } else {
                        this->serial_send_debug("RECV: DD_COMMAND, to forward");
                        uint32_t destination = node_route.take();

                        this->send_command(
                            node_route,
                            payload,
                            destination,
                            dd_packet.header.origin
                        );
                    }

                } break;

                case DD_RT_ACK: {
                    if(dd_packet.header.forward != this->source ||
                       !this->rt_formed) {
                        return;
                    }

                    DDNodeRoute node_route;
                    uint64_t broadcast_counter;

                    if(!dd_packet.extractRT_ACK(node_route, broadcast_counter) ||
                       broadcast_counter != this->broadcast_counter) {
                        return;
                    }
                    this->serial_send_debug("RECV: DD_RT_ACK");
                    node_route.push(this->source);
                    this->send_rt_ack(node_route, dd_packet.header.origin);
                } break;

                case DD_JOIN: {
                    if(!this->rt_formed) {
                        return;
                    }
                    this->serial_send_debug("RECV: DD_JOIN");

                    DDNodeRoute node_route;

                    node_route.push(this->source);

                    this->send_join_request(dd_packet.header.origin);
                } break;

                case DD_LEAVE: {
                    if(dd_packet.header.forward != this->source ||
                       !this->rt_formed) {
                        return;
                    }
                    this->serial_send_debug("RECV: DD_LEAVE");

                    this->send_leave(dd_packet.payload, dd_packet.header.origin);
                } break;

                case DD_CONNECTION_LOST: {
                    if(this->rt_formed &&
                       dd_packet.header.source == this->rely) {
                        this->rt_disconnect();
                    }
                }
            }
        }
    }

    void NetworkLayer::send_to_mac(MicroBitEvent) {
        DDPacket dd_packet = this->outBufferPackets.front();
        this->outBufferPackets.pop();
        this->sending_to = dd_packet.header.forward;

        ManagedBuffer packet = dd_packet.toManagedBuffer();
        switch(dd_packet.header.type) {
            case DD_RT_INIT:
                this->serial_send_debug("SEND: DD_RT_INIT");
                break;
            case DD_DATA:
                this->serial_send_debug("SEND: DD_DATA");
                break;
            case DD_COMMAND:
                this->serial_send_debug("SEND: DD_COMMAND");
                break;
            case DD_RT_ACK:
                this->serial_send_debug("SEND: DD_RT_ACK");
                break;
            case DD_JOIN:
                this->serial_send_debug("SEND: DD_JOIN");
                break;
            case DD_LEAVE:
                this->serial_send_debug("SEND: DD_LEAVE");
                break;
            case DD_CONNECTION_LOST:
                this->serial_send_debug("SEND: DD_CONNECTION_LOST");
                break;
        }
        this->mac_layer.send(packet.getBytes(), packet.length(), dd_packet.header.forward);

        if(this->send_state == DD_WAIT_TO_BROADCAST) {
            this->send_state = DD_READY_TO_SEND;
        }
    }

    void NetworkLayer::packet_sent(MicroBitEvent) {
        switch(this->send_state) {
            case DD_WAIT_TO_SINK:
            case DD_WAIT_TO_SUBTREE:
                this->send_state = DD_READY_TO_SEND;

                break;

            case DD_READY_TO_SEND:
            case DD_WAIT_TO_BROADCAST:
                return;
        }
    }

    void NetworkLayer::packet_timeout(MicroBitEvent) {
        switch(this->send_state) {
            case DD_WAIT_TO_SINK:
                this->rt_disconnect();

                break;

            case DD_WAIT_TO_SUBTREE:
                this->send_leave();

                break;

            case DD_READY_TO_SEND:
            case DD_WAIT_TO_BROADCAST:
                return;
        }

        this->send_state = DD_READY_TO_SEND;
    }

    void NetworkLayer::recv_from_serial(ManagedBuffer serial_received_buffer) {
        this->serial_received_buffer = serial_received_buffer;
        MicroBitEvent(NETWORK_LAYER_INTERNALS, NETWORK_LAYER_SERIAL_ROUTING_TABLE);
    }

    void NetworkLayer::recv_from_serial(MicroBitEvent) {
        uint8_t code = this->serial_received_buffer.getByte(0);

        if(code == DD_SERIAL_INIT) {
            uint8_t mode = DD_SERIAL_INIT_ACK;

            this->serial_send_debug("init");

            this->serial->send(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                ManagedBuffer(&mode, sizeof(mode))
            );

            if(this->serial_send_state != DD_SERIAL_READY_TO_SEND) {
                this->serial->send(
                    NETWORK_LAYER_INTERNALS,
                    NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                    this->serial_packet_in_send
                );
            }

            this->serial_initiated = true;

            return;
        }
        
        switch(this->serial_send_state) {
            case DD_SERIAL_SEND_CLEAR:
                this->send_rt_init();

                break;

            case DD_SERIAL_SEND_GET: {
                DDNodeRoute node_route;

                if(serial_received_buffer == ManagedBuffer::EmptyPacket) {
                    this->serial_route_found = false;
                }

                else {
                    this->serial_route_found = *serial_received_buffer.getBytes();

                    if(this->serial_route_found) {

                        DDNodeRoute node_route = DDNodeRoute {
                            .size = (serial_received_buffer.length()-1) / sizeof(uint32_t),
                            .addresses = ManagedBuffer(
                                serial_received_buffer.getBytes()+1,
                                (serial_received_buffer.length()-1)
                            )
                        };

                        uint32_t destination = node_route.take();

                        this->send_command(
                            this->serial_send_get_payload,
                            node_route,
                            destination
                        );
                    }
                }

                this->serial_wait_route_found = false;

            } break;

            case DD_SERIAL_SEND_PUT:
                this->inBufferNodes.push(DDConnection {
                    .status            = true,
                    .address           = this->serial_send_put_origin
                });

                MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE);

                break;

            case DD_SERIAL_READY_TO_SEND:
                break;

        }
        
        this->serial_send_state = DD_SERIAL_READY_TO_SEND;
    }

    void NetworkLayer::send_to_serial(MicroBitEvent) {
        this->serial_packet_in_send = this->outSerialPackets.front();
        this->outSerialPackets.pop();

        this->serial->send(
            NETWORK_LAYER_INTERNALS,
            NETWORK_LAYER_SERIAL_ROUTING_TABLE,
            this->serial_packet_in_send
        );
    }


    // utility functions
    bool NetworkLayer::elapsed_from_last_operation(uint64_t interval) {
        uint64_t current_time = system_timer_current_time();
        uint64_t elapsed_time = current_time - this->time_last_operation;

        if(elapsed_time > interval) {
            this->time_last_operation = current_time;

            return true;
        }
        return false;
    }

    void NetworkLayer::rt_disconnect() {
        this->serial_send_debug("rt_DISCONNETCTED");

        this->rt_formed = false;

        std::queue<DDPacket> emptyOutBufferPackets;

        std::swap(this->outBufferPackets, emptyOutBufferPackets);

        this->send_connection_lost();

        this->inBufferNodes.push(DDConnection {
            .status            = false,
            .address           = this->rely,
        });

        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE);
    }

    void NetworkLayer::rt_connect(uint64_t broadcast_counter, uint32_t source) {
        this->serial_send_debug("rt_CONNECTED");
        this->broadcast_counter = broadcast_counter;

        this->rely = source;
        this->rt_formed = true;

        this->inBufferNodes.push(DDConnection {
            .status            = true,
            .address           = this->rely,
        });
        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE);
    }

    void NetworkLayer::incr_broadcast_counter(MicroBitEvent) {
        this->broadcast_counter++;
        this->put_store_broadcast_counter();
        this->send_rt_init();
        // this->serial_clear_node_routes();
    }

    void NetworkLayer::get_store_broadcast_counter() {
        KeyValuePair* stored_broadcast_counter = this->uBit->storage.get("bc");

        if(stored_broadcast_counter != NULL) {
            this->serial_send_debug("get bc non null");
            memcpy(
                (uint8_t*) &this->broadcast_counter,
                stored_broadcast_counter->value, sizeof(this->broadcast_counter)
            );

            delete stored_broadcast_counter;
        }
    }

    void NetworkLayer::put_store_broadcast_counter() {
        this->uBit->storage.put("bc",
            (uint8_t*) &this->broadcast_counter,
            sizeof(this->broadcast_counter)
        );
    }


    // serial comunication
    void NetworkLayer::serial_wait_init() {
        while(!this->serial_initiated) {
            this->uBit->sleep(50);
        }
    }

    void NetworkLayer::serial_send(ManagedBuffer packet) {
        this->outSerialPackets.push(packet);
    }

    inline void NetworkLayer::serial_send_debug(ManagedBuffer message) {
        if(this->debug && this->serial != NULL)
            this->serial->send(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_DEBUG,
                message
            );
    }

    void NetworkLayer::serial_get_node_route(uint32_t destination) {
        uint8_t mode = DD_SERIAL_GET;
        ManagedBuffer packet(sizeof(mode) + sizeof(destination));

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(mode);
        memcpy(
            packet.getBytes() + offset,
            &mode,
            length); offset += length;

        length = sizeof(destination);
        memcpy(
            packet.getBytes() + offset,
            &destination,
            length); offset += length;

        this->serial_send(packet);
    }

    void NetworkLayer::serial_put_node_route(DDNodeRoute node_route, uint32_t destination) {
        uint8_t mode = DD_SERIAL_PUT;
        ManagedBuffer packet(sizeof(mode) + node_route.addresses.length());

        uint64_t offset = 0;
        uint64_t length = 0;

        length = sizeof(mode);
        memcpy(
            packet.getBytes(),
            &mode,
            length); offset += length;

        length = node_route.addresses.length();
        memcpy(
            packet.getBytes() + offset,
            node_route.addresses.getBytes(),
            length); offset += length;

        this->serial_send_put_origin = destination;
        this->serial_send(packet);
    }

    void NetworkLayer::serial_clear_node_routes() {
        uint8_t mode = DD_SERIAL_CLEAR;

        ManagedBuffer packet(&mode, sizeof(mode));

        this->serial_send(packet);
    }

    // send functions
    void NetworkLayer::send_data(ManagedBuffer payload) {
        this->send_data(payload, this->source);
    }
    void NetworkLayer::send_data(ManagedBuffer payload, uint32_t origin) {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_DATA,
                    .forward = this->rely,
                    .origin  = origin,
                    .payload = payload
                }
            )
        );
    }

    void NetworkLayer::send_rt_init() {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_RT_INIT,
                    .forward = NETWORK_BROADCAST,
                    .origin  = 0,
                    .payload = ManagedBuffer(
                        (uint8_t*)&this->broadcast_counter,
                        sizeof(this->broadcast_counter))
                }
            )
        );
    }

    void NetworkLayer::send_rt_ack() {
        this->send_rt_ack(DDNodeRoute::of(this->source), this->source);
    }
    void NetworkLayer::send_rt_ack(DDNodeRoute node_route, uint32_t origin) {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_RT_ACK,
                    .forward = this->rely,
                    .origin  = origin,
                    .payload = DDPayloadWithNodeRoute::of(
                        node_route,
                        ManagedBuffer(
                            (uint8_t*)&this->broadcast_counter,
                            sizeof(this->broadcast_counter)
                        )
                    ).toManagedBuffer()
                }
            )
        );
    }

    void NetworkLayer::send_join_request() {
        this->send_join_request(this->source);
    }

    void NetworkLayer::send_join_request(uint32_t origin) {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_JOIN,
                    .forward = (this->rt_formed) ? this->rely : NETWORK_BROADCAST,
                    .origin  = origin,
                    .payload = ManagedBuffer()
                }
            )
        );
    }

    void NetworkLayer::send_command(
        DDNodeRoute node_route,
        ManagedBuffer payload,
        uint32_t destination,
        uint32_t origin
    ) {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_COMMAND,
                    .forward = destination,
                    .origin  = origin,
                    .payload = DDPayloadWithNodeRoute::of(
                        node_route, payload
                    ).toManagedBuffer()
                }
            )
        );
    }
    void NetworkLayer::send_command(
        ManagedBuffer payload,
        DDNodeRoute node_route,
        uint32_t destination
    ) {
        this->send_command(
            node_route,
            payload,
            destination,
            this->source
        );
    }

    void NetworkLayer::send_leave() {
        if(this->sink_mode) {
            this->inBufferNodes.push(DDConnection {
                .status            = false,
                .address           = this->sending_to,
            });
            MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE);

            return;
        }

        ManagedBuffer payload = ManagedBuffer(
            sizeof(this->sending_to)
        );

        memcpy(
            payload.getBytes(),
            (uint8_t*) &this->sending_to,
            sizeof(this->sending_to));

        this->send_leave(payload, this->source);
    }
    void NetworkLayer::send_leave(ManagedBuffer payload, uint32_t origin) {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_LEAVE,
                    .forward = this->rely,
                    .origin  = origin,
                    .payload = payload
                }
            )
        );
    }

    void NetworkLayer::send_connection_lost() {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_CONNECTION_LOST,
                    .forward = NETWORK_BROADCAST,
                    .origin  = this->source,
                    .payload = ManagedBuffer()
                }
            )
        );
    }


    void NetworkLayer::avoid_rt_init_from(uint32_t address) {
        this->avoid_rt_init_from_enabled = true;
        this->avoid_rt_init_from_address = address;
    }
// }
