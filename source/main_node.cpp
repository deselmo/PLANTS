// #include "MicroBit.h"
// #include "NetworkLayer.h"

// #define NETWORK_ID 42

// MicroBit uBit;
// SerialCom serial(&uBit);
// NetworkLayer nl(&uBit, NETWORK_ID, NULL, false);

// void recv(MicroBitEvent) {
//     ManagedBuffer message_received = nl.recv();

//     if(message_received == ManagedBuffer::EmptyPacket) return;

//     uBit.display.print(message_received.toManagedString());
// }

// void recvRt(MicroBitEvent) {}

// int main() {
//     uBit.init();
//     serial.init();

//     uBit.display.print('n');
//     nl.init();
//     uBit.display.print('N');

//     uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, recv);
//     uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_RT_INIT, recvRt);
//     uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_RT_BROKEN, recvRt);

//     while(true) {
//         if(uBit.buttonA.isPressed()) {
//             nl.send("a");
//         }

//         if(uBit.buttonB.isPressed()) {
//             nl.send("b");
//         }

//         uBit.sleep(100);
//     }

//     release_fiber();
// }
