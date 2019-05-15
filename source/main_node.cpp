#include "MicroBit.h"
#include "NetworkLayer.h"

#define NETWORK_ID 42

MicroBit uBit;
SerialCom serial(&uBit);
NetworkLayer nl(&uBit, NETWORK_ID, &serial, false);

void send(const char* str) {
    serial.send(101, 1, str);
}

int main() {
    uBit.init();
    serial.init();

    uBit.display.print('n');
    nl.init();
    uBit.display.print('N');


    while(true) {
        if(uBit.buttonA.isPressed()) {
            uBit.display.print("s");
            send("s");
        }

        if(uBit.buttonB.isPressed()) {
            uBit.display.print("n");
            serial.send(NETWORK_LAYER_INTERNALS, NETWORK_LAYER_SERIAL_ROUTING_TABLE, "n");
        }

        uBit.sleep(100);
    }

    release_fiber();
}
