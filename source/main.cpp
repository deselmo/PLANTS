#include "MicroBit.h"
#include "NetworkLayer.h"
#include "ApplicationLayer.h"

#define NETWORK_ID 42

#define MICROBIT_YELLOW  2150725565
#define MICROBIT_WILLIAM 2796262830
#define MICROBIT_ANDREA  2426823503
#define MICROBIT_MATTEO  2764293628


MicroBit uBit;
SerialCom serial(&uBit);

NetworkLayer nl(&uBit, NETWORK_ID);
ApplicationLayer ap(&uBit, &nl);


float sens_button(Sensor *sensor){
    if(uBit.buttonA.isPressed())
        switch(microbit_serial_number())
        {
            case MICROBIT_ANDREA:
                return 1.0;
            case MICROBIT_WILLIAM:
                return 2.0;
            default:
                return 3.0;
        }
    switch (microbit_serial_number())
    {
        case MICROBIT_ANDREA:
            return -1.0;
        case MICROBIT_WILLIAM:
            return -2.0;
        default:
            return -3.0;
    }
}

int main() {
    uBit.init();
    serial.init();

    // SINK MODE
    if(microbit_serial_number() == MICROBIT_YELLOW) {
        uBit.display.print('s');

        // with debug
        // nl.init(&serial, true, true);

        // without debug
        ap.init(&serial, true);

        //nl.init(&serial, true);

        uBit.display.print('S');

        while(true) {
            if(uBit.buttonA.isPressed()) {
                nl.send("w", MICROBIT_WILLIAM);
                uBit.display.print("S");
            }

            if(uBit.buttonB.isPressed()) {
                nl.send("a", MICROBIT_ANDREA);
                uBit.display.print("S");
            }

            uBit.sleep(100);
        }
    }

    else {
        uBit.display.print("n");

        if(microbit_serial_number() == MICROBIT_ANDREA) {
            nl.avoid_rt_init_from(MICROBIT_YELLOW);
        }


        // with debug
        // nl.init(&serial, false, true);

        // without debug
        ap.init(NULL,false);
        ap.addSensor("AButton", &sens_button);

        //nl.init();

        uBit.display.print("N");

        while(true) {
            uBit.sleep(100);
        }
    }

    release_fiber();
}
