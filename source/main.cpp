/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/ 

#include "MicroBit.h"

MicroBit    uBit;

void onData(MicroBitEvent);
int mySend(ManagedString pkt);

int main()
{
    // Initialise the micro:bit runtime.
    uBit.init();
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    uBit.radio.enable();

    uBit.display.scroll("ATTIVO");
    uBit.sleep(200);

    while(1)
    {
        if (uBit.buttonA.isPressed()) {
            int res = mySend("1");
            if(res == 0) uBit.display.scroll("ACK RICEVUTO");
            else if(res == -1) uBit.display.scroll("ACK NON RICEVUTO");
            else uBit.display.scroll("ERRORE");
        }

        uBit.sleep(200);
    }

}

void onData(MicroBitEvent)
{
    ManagedString s = uBit.radio.datagram.recv();

    if (s == "1") {
        uBit.display.print("A");
        uBit.radio.datagram.send("ACK");
    }
}

int mySend(ManagedString pkt) {
    int count = 0;
    uBit.messageBus.ignore(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    while(count < 5) {
        if(uBit.radio.datagram.send(pkt) == MICROBIT_INVALID_PARAMETER) {
            uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
            return MICROBIT_INVALID_PARAMETER;
        }
        uBit.sleep(1000);

        ManagedString ack = uBit.radio.datagram.recv();
        if((ack == ManagedString("ACK")) != true) {
            uBit.display.scroll(ack);
            count ++;
        }
        else {
            uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
            return 0;
        }
    }

    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    return -1;
}