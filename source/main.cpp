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


float humidity_loop(Sensor *){
    float x = uBit.io.P0.getAnalogValue();
    x = 1023 - x;
    return x*100./1023.;
}

int period = SENSING_INTERVAL;
int stillX = 0;
int stillY = 0;
int stillZ = 0;
int fallX;
int fallY;
int fallZ;
int state = 0;

float sens_accellerometer(Sensor *sensor){
    float ret = 0;
    if((int)sensor->sensing_rate != period)
    {
        uBit.accelerometer.setPeriod(sensor->sensing_rate);
        period = sensor->sensing_rate;
    }
    int thisX = uBit.accelerometer.getX();
    int thisY = uBit.accelerometer.getY();
    int thisZ = uBit.accelerometer.getZ();
    //I'm still!
    if(state == 0)
    {
        if(abs(stillX - thisX) > 500)
            state = 1;
        else if(abs(stillY - thisY) > 500)
            state = 1;
        else if(abs(stillZ - thisZ) > 500)
            state = 1;
        if(state == 1)
        {
            fallX = thisX;
            fallY = thisY;
            fallX = thisX;
            ret = 1;
        }
        else
        {
            stillX = thisX;
            stillY = thisY;
            stillZ = thisZ;
            ret = 0;
        }
    }
    else if(state == 1)
    {
        if(
            abs(fallX - thisX) > 500 ||
            abs(fallY - thisY) > 500 ||
            abs(fallZ - thisZ) > 500
          )
          {
            ;
          }
          else
          {
              state = 2;
          }
        fallX = thisX;
        fallY = thisY;
        fallZ = thisZ;
        ret = 1;
    }
    else if(state == 2)
    {
        ret = 1;
        if(abs(fallX - thisX) > 500)
        {
            if(
                abs(thisX - stillX) < 500 &&
                abs(thisY - stillY) < 500 &&
                abs(thisZ - stillZ) < 500
              )
            {
                state = 0;
                stillX = thisX;
                stillY = thisY;
                stillZ = thisZ;
                ret = 0;
            }
            else
                ret = 1;
            
        
        }
        else if(abs(fallY - thisY) > 500)
        {
            if(
                abs(thisX - stillX) < 500 &&
                abs(thisY - stillY) < 500 &&
                abs(thisZ - stillZ) < 500
              )
            {
                state = 0;
                stillX = thisX;
                stillY = thisY;
                stillZ = thisZ;
                ret = 0;
            }
            else
                ret = 1;
        
        }
        else if(abs(fallZ - thisZ) > 500)
        {
            if(
                abs(thisX - stillX) < 500 &&
                abs(thisY - stillY) < 500 &&
                abs(thisZ - stillZ) < 500
              )
            {
                state = 0;
                stillX = thisX;
                stillY = thisY;
                stillZ = thisZ;
                ret = 0;
            }
            else
                ret = 1;
        
        }
        fallX = thisX;
        fallY = thisY;
        fallZ = thisZ;
    }
    return ret;
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
        //uBit.accelerometer.configure();
        uBit.accelerometer.setPeriod(SENSING_INTERVAL);
        uBit.display.scroll("HERE");

        if(microbit_serial_number() == MICROBIT_ANDREA) {
            nl.avoid_rt_init_from(MICROBIT_YELLOW);
        }
        stillX = uBit.accelerometer.getX();
        stillY = uBit.accelerometer.getY();
        stillZ = uBit.accelerometer.getZ();
        // with debug
        // nl.init(&serial, false, true);

        // without debug
        ap.init(NULL,false);
        ap.addSensor("AButton", &sens_button);
        ap.addSensor("accellerometer", &sens_accellerometer);
        ap.addSensor("humidity",&humidity_loop);

        //nl.init();

        uBit.display.print("N");

        while(true) {
            uBit.sleep(100);
        }
    }

    release_fiber();
}
