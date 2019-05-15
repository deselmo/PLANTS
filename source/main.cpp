// #include "MicroBit.h"
// #include "NetworkLayer.h"

// #define NETWORK_ID 42

// #define MAIN 101
// #define DEBUG 1

// MicroBit uBit;
// SerialCom serial(&uBit);
// NetworkLayer nl(&uBit, NETWORK_ID, &serial, true);

// void send(const char* str) {
//     serial.send(MAIN, DEBUG, str);
// }

// int main() {
//     uBit.init();
//     serial.init();

//     uBit.display.print('w');
//     nl.init();
//     uBit.display.print('S');


//     while(true) {
//         if(uBit.buttonA.isPressed()) {
//             uBit.display.print("a");
//             send("a");
//         }

//         if(uBit.buttonB.isPressed()) {
//             uBit.display.print("b");
//             serial.send(NETWORK_LAYER_INTERNALS, NETWORK_LAYER_SERIAL_ROUTING_TABLE, "b");
//         }

//         uBit.sleep(100);
//     }

//     release_fiber();
// }
