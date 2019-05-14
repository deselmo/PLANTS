#include "NetworkLayer.h"


// DDPacket {
    DDPacket DDPacket::Empty = DDPacket {
        .type       = (DDType) 0,
        .source     = 0,
        .network_id = 0,
        .forward    = 0,
        .origin     = 0,
        .length     = 0,
        .payload    = ManagedBuffer(1)
    };

    DDPacket DDPacket::of(
        NetworkLayer* network_layer,
        DDPacketData  dd_packet_data
    ) {
        return DDPacket {
            .type = dd_packet_data.type,
            .source = network_layer->get_source(),
            .network_id = network_layer->get_network_id(),
            .forward = dd_packet_data.forward,
            .origin = dd_packet_data.origin,
            .length = dd_packet_data.payload.length(),
            .payload = dd_packet_data.payload
        };
    }


    // extractor methods
    bool DDPacket::extractRT_INIT(uint64_t& broadcast_counter) {
        if (this->type != DD_RT_INIT ||
            this->payload.length() != sizeof(uint64_t)
        ) {
            return false;
        }

        memcpy(
            &broadcast_counter,
            this->payload.getBytes(),
            sizeof(uint64_t)
        );

        return true;
    }

    bool DDPacket::extractRT_ACK(DDNodeRoute& node_route, uint64_t& broadcast_counter) {
        if (this->type != DD_RT_ACK) {
            return false;
        }

        DDPayloadWithNodeRoute payload_with_node_route =
            DDPayloadWithNodeRoute::fromManagedBuffer(this->payload);

        if(payload_with_node_route.payload.length() != sizeof(uint64_t)) {
            return false;
        }

        memcpy(
            &broadcast_counter,
            this->payload.getBytes(),
            sizeof(uint64_t)
        );

        node_route = payload_with_node_route.node_route;

        return !node_route.isEmpty();
    }

    bool DDPacket::extractCOMMAND(DDNodeRoute& node_rotue, ManagedBuffer& payload) {
        if (this->type != DD_COMMAND) {
            return false;
        }

        DDPayloadWithNodeRoute payload_with_node_route =
            DDPayloadWithNodeRoute::fromManagedBuffer(this->payload);

        if(payload_with_node_route.isEmpty()) {
            return false;
        }

        node_rotue = payload_with_node_route.node_route;

        payload = payload_with_node_route.payload;

        return true;
    }
    
    bool DDPacket::extractLEAVE(uint32_t& address, uint64_t& broadcast_number) {
        if (this->type != DD_LEAVE ||
            this->payload.length() != sizeof(uint32_t) + sizeof(uint64_t)
        ) {
            return false;
        }

        uint64_t offset = 0;
        memcpy(
            &address,
            this->payload.getBytes(),
            offset += sizeof(uint32_t)
        );
        memcpy(
            &broadcast_number,
            this->payload.getBytes() + offset,
            sizeof(uint64_t)
        );

        return true;
    }


    bool DDPacket::operator==(const DDPacket& other) {
        return this->type       == other.type       &&
               this->source     == other.source     &&
               this->network_id == other.network_id &&
               this->forward    == other.forward    &&
               this->origin     == other.origin     &&
               this->length     == other.length     &&
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

        ManagedBuffer packet(sizeof(DDPacket) + this->payload.length());

        uint64_t offset = 0;
        memcpy(
            packet.getBytes(),
            this,
            offset += sizeof(DDPacket)
        );
        memcpy(
            packet.getBytes() + offset,
            this->payload.getBytes(),
            this->payload.length()
        );

        return packet;
    }
    DDPacket DDPacket::fromManagedBuffer(ManagedBuffer packet) {
        if(packet.length() < sizeof(DDPacket))
            return DDPacket::Empty;

        DDPacket dd_packet;

        uint64_t offset = 0;
        memcpy(
            (void*) &dd_packet,
            (void*) packet.getBytes(),
            offset += sizeof(DDPacket)
        );

        dd_packet.payload = ManagedBuffer(
            packet.getBytes() + offset,
            dd_packet.length
        );

        return dd_packet;
    }
// }


// DDNodeRoute {
    DDNodeRoute DDNodeRoute::Empty = DDNodeRoute {
        .size      = 0,
        .addresses = ManagedBuffer(1)
    };


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
        this->size++;
        ManagedBuffer nodes(this->addresses.length() + sizeof(uint32_t));

        uint64_t offset = 0;
        memcpy(
            addresses.getBytes(),
            &node_address,
            offset += sizeof(uint32_t)
        );
        memcpy(
            addresses.getBytes() + offset,
            this->addresses.getBytes(),
            this->addresses.length()
        );
        this->addresses = addresses;
    }
    uint32_t DDNodeRoute::take() {
        if(!(this->size > 0))
            return 0;

        uint32_t node_address = *(this->addresses.getBytes());

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
        if(*this == DDNodeRoute::Empty) {
            return ManagedBuffer::EmptyPacket;
        }

        ManagedBuffer packet(
            sizeof(DDNodeRoute) +
            this->addresses.length()
        );

        uint8_t *buffer = packet.getBytes();

        uint64_t offset = 0;
        memcpy(
            buffer,
            this,
            offset += sizeof(DDNodeRoute)
        );
        memcpy(
            buffer + offset,
            this->addresses.getBytes(),
            this->addresses.length()
        );

        return packet;
    }
    DDNodeRoute DDNodeRoute::fromManagedBuffer(ManagedBuffer packet) {
        if(packet.length() < sizeof(DDNodeRoute))
            return DDNodeRoute::Empty;

        DDNodeRoute node_route;

        uint64_t offset = 0;
        memcpy(
            (void*) &node_route,
            (void*) packet.getBytes(),
            offset += sizeof(DDNodeRoute)
        );

        node_route.addresses = ManagedBuffer(
            packet.getBytes() + offset,
            node_route.size * sizeof(uint32_t)
        );

        return node_route;
    }
// }


// DDPayloadWithNodeRoute {
    DDPayloadWithNodeRoute DDPayloadWithNodeRoute::Empty = DDPayloadWithNodeRoute {
        .node_route = DDNodeRoute::Empty,
        .length     = 0,
        .payload    = ManagedBuffer(1)
    };


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
            return ManagedBuffer::EmptyPacket;

        ManagedBuffer packet(
            sizeof(DDPayloadWithNodeRoute) +
            this->node_route.addresses.length() +
            this->payload.length()
        );

        uint8_t *buffer = packet.getBytes();

        uint64_t offset = 0;
        memcpy(
            buffer,
            this,
            offset += sizeof(DDPayloadWithNodeRoute)
        );
        memcpy(
            buffer + offset,
            this->node_route.addresses.getBytes(),
            this->node_route.addresses.length()
        );
        offset += this->node_route.addresses.length();
        memcpy(
            buffer + offset,
            this->payload.getBytes(),
            this->payload.length()
        );

        return packet;
    }
    DDPayloadWithNodeRoute DDPayloadWithNodeRoute::fromManagedBuffer(ManagedBuffer packet) {
        if(packet.length() < sizeof(DDPayloadWithNodeRoute))
            return DDPayloadWithNodeRoute::Empty;

        DDPayloadWithNodeRoute payload_with_node_route;

        uint64_t offset = 0;
        memcpy(
            (void*) &payload_with_node_route,
            (void*) packet.getBytes(),
            offset += sizeof(DDPayloadWithNodeRoute)
        );

        payload_with_node_route.node_route.addresses = ManagedBuffer(
            packet.getBytes() + offset,
            payload_with_node_route.node_route.size * sizeof(uint32_t)
        );
        offset += payload_with_node_route.node_route.size * sizeof(uint32_t);

        payload_with_node_route.payload = ManagedBuffer(
            packet.getBytes() + offset,
            payload_with_node_route.length
        );

        return payload_with_node_route;
    }
// }


// NetworkLayer {
    NetworkLayer::NetworkLayer(
        MicroBit*  uBit,
        uint16_t   network_id,
        SerialCom* serial,
        bool sink_mode
    )
        : uBit                (uBit)
        , mac_layer           (MacLayer(uBit))
        , serial              (serial)
        , network_id          (network_id)
        , sink_mode           (sink_mode)
    {};


    void NetworkLayer::init() {
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

        if(this->sink_mode) {
            this->get_store_broadcast_counter();

            this->serial->addListener(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                this, &NetworkLayer::recv_from_serial
            );

            this->serial_wait_init();
        }

        this->mac_layer.init();

        this->source = microbit_serial_number();
        this->serial_in_sending_buffer = ManagedBuffer::EmptyPacket;
        this->serial_send_get_payload = ManagedBuffer::EmptyPacket;

        fiber_add_idle_component(this);
    }


    void NetworkLayer::idleTick() {
        if(this->serial_send_state != DD_SERIAL_SEND_NONE) {
            return;
        }

        if(this->sink_mode) {
            if(this->elapsed_from_last_operation(NETWORK_LAYER_DD_RT_INIT_INTERVAL)) {
                this->serial->send(101,1,"timeout: send initialing new rt_init");

                this->incr_broadcast_counter();
            }
        } else if(!this->rt_formed &&
                this->elapsed_from_last_operation(NETWORK_LAYER_DD_JOIN_REQUEST_INTERVAL)
        ) {
            this->serial->send(101,1,"timeout: send join request");
            this->send_join_request();
        }

        if(this->send_state == DD_READY_TO_SEND && this->outBufferPackets.size()) {
            switch(this->outBufferPackets.front().type) {
                case DD_DATA:
                case DD_RT_ACK:
                case DD_LEAVE:
                    this->send_state = DD_WAIT_TO_SINK;
                    break;
                
                case DD_COMMAND:
                    this->send_state = DD_WAIT_TO_SUBTREE;
                    break;

                case DD_RT_INIT:
                case DD_JOIN:
                    this->send_state = DD_WAIT_TO_BROADCAST;
                    break;
            }

            if(this->send_state != DD_READY_TO_SEND) {
                MicroBitEvent(
                    NETWORK_LAYER_INTERNALS,
                    NETWORK_LAYER_PACKET_READY_TO_SEND
                );
            }
        }
    }


    void NetworkLayer::send(ManagedBuffer payload) {
        this->send_data(payload);
    }

    void NetworkLayer::send(ManagedBuffer payload, uint32_t destination) {
        DDNodeRoute node_route;

        this->serial_send_get_destination = destination;
        this->serial_send_get_payload = payload;

        this->serial_get_node_route(destination);
    }


    /**
     * Retreives packet payload data into NetworLayer::inBuffer.
     *
     * If a data packet is already available, then it will be returned immediately to the caller 
     * (application layer) in the form of a ManagedBuffer.
     *
     * @return the data received, or an empty ManagedBuffer if no data is available.
     */
    ManagedBuffer NetworkLayer::recv() {
        if(inBufferPackets.empty())
            return ManagedBuffer::EmptyPacket;
        else {
            ManagedBuffer p = inBufferPackets.front();
            inBufferPackets.pop();
            return p;
        }
    }

    std::tuple<bool, uint32_t, uint64_t> NetworkLayer::recv_node() {
        tuple<bool, uint32_t, uint64_t> connected_node = inBufferNodes.front();
        inBufferNodes.pop();

        return connected_node;
    }


    // event functions
    void NetworkLayer::recv_from_mac(MicroBitEvent) {
        ManagedBuffer received_mac_buffer = mac_layer.recv();

        DDPacket dd_packet = DDPacket::fromManagedBuffer(received_mac_buffer);

        if(dd_packet.isEmpty()) {
            return;
        }

        if(this->sink_mode) {
            switch(dd_packet.type) {
                case DD_RT_INIT: {
                    return;
                } break;

                case DD_DATA: {
                    this->inBufferPackets.push(dd_packet.payload);
                    MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED); 

                } break;

                case DD_COMMAND: {
                    return;
                } break;

                case DD_RT_ACK: {
                    DDNodeRoute node_route;
                    uint64_t broadcast_counter;

                    if(!dd_packet.extractRT_ACK(node_route, broadcast_counter) ||
                    broadcast_counter != this->broadcast_counter) {
                        return;
                    }

                    this->serial_send_put_origin = dd_packet.origin;

                    this->serial_put_node_route(node_route);
                } break;

                case DD_JOIN: {
                    this->send_rt_init();
                } break;

                case DD_LEAVE: {
                    uint32_t disconnected_node_address;
                    uint64_t broadcast_counter;

                    if(!dd_packet.extractLEAVE(disconnected_node_address, broadcast_counter) ||
                    broadcast_counter != this->broadcast_counter) {
                        return;
                    }

                    this->inBufferNodes.push(tuple<bool, uint32_t, uint64_t>(
                        false,
                        disconnected_node_address,
                        this->broadcast_counter
                    ));
                    MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_NODE_CONNECTED);
                } break;
            }
        }
        else {
            switch(dd_packet.type) {
                case DD_RT_INIT: {
                    uint64_t broadcast_counter_received;

                    if (!dd_packet.extractRT_INIT(broadcast_counter_received) ||
                        broadcast_counter_received <= this->broadcast_counter) {
                        return;
                    }

                    this->rt_connect(broadcast_counter_received, dd_packet.source);

                    this->send_rt_ack();

                    this->send_rt_init();
                } break;

                case DD_DATA: {
                    if(this->rt_formed) {
                        this->send_data(dd_packet.payload, dd_packet.origin);
                    }
                } break;

                case DD_COMMAND: {
                    DDNodeRoute node_route;
                    ManagedBuffer payload;

                    if(!dd_packet.extractCOMMAND(node_route, payload)) {
                        return;
                    }

                    if(node_route.size == 0) {
                        // I am the destination
                        this->inBufferPackets.push(dd_packet.payload);
                        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED); 


                    } else {
                        // I am not the destination
                        uint32_t destination = node_route.take();

                        this->send_command(
                            node_route,
                            payload,
                            destination,
                            dd_packet.origin
                        );
                    }

                } break;

                case DD_RT_ACK: {
                    DDNodeRoute node_route;
                    uint64_t broadcast_counter;

                    if(!dd_packet.extractRT_ACK(node_route, broadcast_counter) ||
                    broadcast_counter != this->broadcast_counter) {
                        return;
                    }

                    node_route.push(this->source);

                    this->send_rt_ack(node_route, dd_packet.origin);
                } break;

                case DD_JOIN: {
                    this->send_rt_init();
                } break;

                case DD_LEAVE: {
                    if(!this->rt_formed) {
                        return;
                    }

                    uint32_t disconnected_node_address;
                    uint64_t broadcast_counter;

                    if(!dd_packet.extractLEAVE(disconnected_node_address, broadcast_counter) ||
                    broadcast_counter != this->broadcast_counter) {
                        return;
                    }

                    this->send_leave(dd_packet.payload, dd_packet.origin);
                } break;
            }
        }
    }

    void NetworkLayer::send_to_mac(MicroBitEvent) {
        DDPacket dd_packet = this->outBufferPackets.front();
        this->outBufferPackets.pop();
        this->sending_to = dd_packet.forward;

        ManagedBuffer packet = dd_packet.toManagedBuffer();
        this->serial->send(101,1,"sending..");
        this->mac_layer.send(packet.getBytes(), packet.length(), dd_packet.forward);
        this->serial->send(101,1,"sended");

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
        uint8_t code = serial_received_buffer.getByte(0);

        this->serial->send(101,1,"received message from serial");
        this->serial->send(101,1, serial_received_buffer);

        if(code == DD_SERIAL_INIT) {
            uint8_t mode = DD_SERIAL_INIT_ACK;

            this->serial->send(
                NETWORK_LAYER_INTERNALS,
                NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                ManagedBuffer(&mode, sizeof(mode))
            );
            if(this->serial_send_state != DD_SERIAL_SEND_NONE) {
                this->serial->send(
                    NETWORK_LAYER_INTERNALS,
                    NETWORK_LAYER_SERIAL_ROUTING_TABLE,
                    this->serial_in_sending_buffer
                );
            }

            this->serial_initiated = true;
        }
        
        else {
            this->serial->send(101,1,"managing received message..");
            switch(this->serial_send_state) {
                case DD_SERIAL_SEND_NONE:
                    this->serial->send(101,1,"   NONE");
                    return;

                case DD_SERIAL_SEND_CLEAR:
                    this->serial->send(101,1,"   CLEAR");

                    this->send_rt_init();

                    break;

                case DD_SERIAL_SEND_GET: {
                    this->serial->send(101,1,"   GET");

                    DDNodeRoute node_route;

                    if(serial_received_buffer == ManagedBuffer::EmptyPacket) {
                        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_SEND_COMMAND_FAIL);
                    }

                    node_route = DDNodeRoute {
                        .size = serial_received_buffer.length() / sizeof(uint32_t),
                        .addresses = serial_received_buffer
                    };

                    this->send_command(
                        this->serial_send_get_payload,
                        node_route,
                        this->serial_send_get_destination
                    );

                    MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_SEND_COMMAND_OK);

                } break;

                case DD_SERIAL_SEND_PUT:
                    this->serial->send(101,1,"   PUT");

                    this->inBufferNodes.push(std::tuple<bool, uint32_t, uint64_t>(
                        true,
                        this->serial_send_put_origin,
                        broadcast_counter
                    ));
                    MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_NODE_CONNECTED);

                    break;
            }
            
            this->serial_send_state = DD_SERIAL_SEND_NONE;
            
        }
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
        this->rt_formed = false;
        this->rely = 0;

        std::queue<DDPacket> emptyOutBufferPackets;

        std::swap(this->outBufferPackets, emptyOutBufferPackets);

        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_RT_BROKEN);
    }

    void NetworkLayer::rt_connect(uint64_t broadcast_counter, uint32_t rely) {
        this->broadcast_counter = broadcast_counter;
        this->rely = rely;
        this->rt_formed = true;

        MicroBitEvent(NETWORK_LAYER, NETWORK_LAYER_INIT);
    }

    void NetworkLayer::incr_broadcast_counter() {
        this->broadcast_counter++;
        this->put_store_broadcast_counter();
        this->serial_clear_node_routes();
    }

    void NetworkLayer::get_store_broadcast_counter() {
        KeyValuePair* stored_broadcast_counter = this->uBit->storage.get("bc");

        if(stored_broadcast_counter == NULL) {
            this->broadcast_counter = 0;
        } else {
            uint64_t broadcast_counter;

            memcpy(
                (uint8_t*) &broadcast_counter,
                stored_broadcast_counter->value, sizeof(broadcast_counter)
            );

            delete stored_broadcast_counter;

            this->broadcast_counter=broadcast_counter;
        }
    }

    void NetworkLayer::put_store_broadcast_counter() {
        this->uBit->storage.put("bc",
            (uint8_t*) &this->broadcast_counter,
            sizeof(this->broadcast_counter)
        );
    }

    void NetworkLayer::serial_wait_init() {
        while(!this->serial_initiated) {
            this->uBit->sleep(100);
        }
    }

    void NetworkLayer::serial_send(ManagedBuffer packet) {
        this->serial_in_sending_buffer = packet;

        this->serial->send(101,1,"serial sending");
        this->serial->send(101,1,packet);

        this->serial->send(
            NETWORK_LAYER_INTERNALS,
            NETWORK_LAYER_SERIAL_ROUTING_TABLE,
            packet
        );
    }

    void NetworkLayer::serial_get_node_route(uint32_t destination) {
        uint8_t mode = DD_SERIAL_GET;
        ManagedBuffer packet(sizeof(mode) + sizeof(destination));

        uint64_t offset = 0;
        memcpy(
            packet.getBytes(),
            &mode,
            offset += sizeof(mode)
        );
        memcpy(
            packet.getBytes() + offset,
            &destination,
            sizeof(destination)
        );

        this->serial_send_state = DD_SERIAL_SEND_GET;
        this->serial_send(packet);
    }

    void NetworkLayer::serial_put_node_route(DDNodeRoute node_route) {
        uint8_t mode = DD_SERIAL_PUT;
        ManagedBuffer packet(sizeof(mode) + node_route.addresses.length());

        uint64_t offset = 0;
        memcpy(
            packet.getBytes(),
            &mode,
            offset += sizeof(mode)
        );
        memcpy(
            packet.getBytes() + offset,
            node_route.addresses.getBytes(),
            node_route.addresses.length()
        );

        this->serial_send_state = DD_SERIAL_SEND_PUT;
        this->serial_send(packet);
    }

    void NetworkLayer::serial_clear_node_routes() {
        uint8_t mode = DD_SERIAL_CLEAR;

        ManagedBuffer packet(&mode, sizeof(mode));

        this->serial_send_state = DD_SERIAL_SEND_CLEAR;
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
        this->serial->send(101,1,"pushing oin queue..");
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_RT_INIT,
                    .forward = NETWORK_BROADCAST,
                    .origin  = 0,
                    .payload = ManagedBuffer(
                        (uint8_t*)&broadcast_counter,
                        sizeof(broadcast_counter))
                }
            )
        );
        this->serial->send(101,1,"pushed");
    }

    void NetworkLayer::send_rt_ack() {
        this->send_rt_ack(DDNodeRoute::of(this->source), this->source);
    }
    void NetworkLayer::send_rt_ack(DDNodeRoute node_route, uint32_t origin) {
        uint64_t broadcast_counter = this->broadcast_counter;

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
                            (uint8_t*)&broadcast_counter,
                            sizeof(broadcast_counter)
                        )
                    ).toManagedBuffer()
                }
            )
        );
    }

    void NetworkLayer::send_join_request() {
        this->outBufferPackets.push(
            DDPacket::of (
                this,
                DDPacketData {
                    .type    = DD_JOIN,
                    .forward = NETWORK_BROADCAST,
                    .origin  = this->source,
                    .payload = DDNodeRoute::of(
                        this->source
                    ).toManagedBuffer()
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
        ManagedBuffer payload = ManagedBuffer(
            sizeof(this->sending_to) + sizeof(this->broadcast_counter)
        );

        uint64_t offset = 0;
        memcpy(
            payload.getBytes(),
            (uint8_t*) &this->sending_to,
            sizeof(this->sending_to)
        );
        offset += sizeof(this->sending_to);
        memcpy(
            payload.getBytes() + offset,
            (uint8_t*) &this->broadcast_counter,
            sizeof(this->broadcast_counter)
        );

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
// }
