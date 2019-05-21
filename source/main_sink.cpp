// #include "MicroBit.h"
// #include "NetworkLayer.h"

// #define NETWORK_ID 42

// MicroBit uBit;
// SerialCom serial(&uBit);
// NetworkLayer nl(&uBit, NETWORK_ID, &serial, true, true);

// uint32_t node_address = 0;

// void recv(MicroBitEvent) {
//     ManagedBuffer message_received = nl.recv();

//     if(message_received == ManagedBuffer::EmptyPacket) return;

//     serial.send(100, 2, message_received);
//     uBit.display.print(message_received.toManagedString());
// }

// void recv_node(MicroBitEvent) {
//     DDNodeConnection node_connection = nl.recv_node();

//     if(node_connection.isEmpty()) return;

//     if(node_connection.status) {
//         node_address = node_connection.address;
//     }
// }


// int main() {
//     uBit.init();
//     serial.init();

//     uBit.display.print('s');
//     nl.init();
//     uBit.display.print('S');

//     uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, recv);
//     uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_NODE_CONNECTIONS, recv_node);

//     while(true) {
//         if(uBit.buttonA.isPressed()) {
//             nl.send("a", node_address);
//         }

//         if(uBit.buttonB.isPressed()) {
//             nl.send("b", node_address);
//         }

//         uBit.sleep(100);
//     }

//     release_fiber();
// }
