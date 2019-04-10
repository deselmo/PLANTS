#include "MicroBit.h"

#define MY_APP 4000
#define RECEIVE_ACK 1
#define RECEIVE_DATAGRAM 2
#define ACK_NOT_RECEIVED 3


MicroBit uBit;

ManagedString receivedData;
bool receivedAck = false;

void onReceive(MicroBitEvent) {
    ManagedString s = uBit.radio.datagram.recv();
    if(s == ManagedString(""))  {
        return;
    }
    if(s == ManagedString("ACK")) {
        MicroBitEvent(MY_APP, RECEIVE_ACK);
    } else {
        uBit.radio.datagram.send("ACK");
        receivedData = s;
        MicroBitEvent(MY_APP, RECEIVE_DATAGRAM);
    }
}

void onCheckAck(MicroBitEvent) {
    receivedAck = true;
}

int mySend(ManagedString s) {
    receivedAck = false;

    uBit.messageBus.listen(MY_APP, RECEIVE_ACK, onCheckAck);
    int result = ACK_NOT_RECEIVED;
    for(int i = 0; i < 5; i++) {
        if(uBit.radio.datagram.send(s) == MICROBIT_INVALID_PARAMETER) {
            result = MICROBIT_INVALID_PARAMETER;
            break;
        }
    
        uBit.sleep(1000);

        if(receivedAck) {
            result = MICROBIT_OK;
            break;
        }
    }
    uBit.messageBus.ignore(MY_APP, RECEIVE_ACK, onCheckAck);

    return result;
}

void onDatagramReceived(MicroBitEvent) {
    uBit.display.scroll(receivedData);
}


int main()
{
    uBit.init();
    uBit.radio.enable();
    
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onReceive);
    uBit.messageBus.listen(MY_APP, RECEIVE_DATAGRAM, onDatagramReceived);

    uBit.display.scroll("V");


    while(1)
    {
        if(uBit.buttonA.isPressed()) {
            uBit.display.scroll("i");
            switch(mySend("ciao")) {
                case MICROBIT_INVALID_PARAMETER:
                    uBit.display.scroll("invalid parameter");
                    break;
                case ACK_NOT_RECEIVED:
                    uBit.display.scroll("no ack ricevuto");
                    break;
                case MICROBIT_OK:
                    uBit.display.scroll("ack ricevuto");
                    break;
            }

            uBit.display.scroll("f");
        }

        uBit.sleep(100);
    }
    
    release_fiber();
}
