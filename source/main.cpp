#include "MicroBit.h"
#include "NetworkLayer.h"

#define NETWORK_ID 42

#define MICROBIT_YELLOW  2150725565
#define MICROBIT_WILLIAM 2796262830
#define MICROBIT_ANDREA  2426823503

#define APPLICATION_ID 100
#define APPLICATION_SERIAL 2

MicroBit uBit;
SerialCom serial(&uBit);

NetworkLayer nl(&uBit, NETWORK_ID);


void to_serial(ManagedBuffer message) {
    serial.send(APPLICATION_ID, APPLICATION_SERIAL, message);
}


void sink_recv(MicroBitEvent) {
    DDMessage message_received = nl.recv();

    if(message_received.isEmpty()) return;

    to_serial(message_received.payload);
    uBit.display.print(message_received.payload.toManagedString());
}

void sink_recv_connection(MicroBitEvent) {
    DDConnection connection = nl.recv_connection();
    if(connection == DDConnection::Empty) return;

    to_serial("node news");
    to_serial(ManagedBuffer((uint8_t*)&connection.address, sizeof(connection.address)));
    to_serial(ManagedBuffer((uint8_t*)&connection.status, sizeof(connection.status)));
    to_serial("");
}


void node_recv(MicroBitEvent) {
    DDMessage message_received = nl.recv();
    if(message_received.isEmpty) return;

    uBit.display.print(message_received.payload.toManagedString());
}

void node_recv_connection(MicroBitEvent) {
    DDConnection connection = nl.recv_connection();
    if(connection == DDConnection::Empty) return;

    if(connection.status == true) {
        uBit.display.print("C");
    } else {
        uBit.display.print("N");
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
        nl.init(&serial, true);

        uBit.display.print('S');

        uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, sink_recv);
        uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE, sink_recv_connection);

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
        nl.init();

        uBit.display.print("N");

        uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_PACKET_RECEIVED, node_recv);
        uBit.messageBus.listen(NETWORK_LAYER, NETWORK_LAYER_CONNECTION_UPDATE, node_recv_connection);

        ManagedBuffer message;
        switch(microbit_serial_number()) {
            case MICROBIT_ANDREA:
                message = "a";
                break;
            
            case MICROBIT_WILLIAM:
                message = "w";
                break;

            default:
                message = "m";
                break;
        }

        while(true) {
            if(uBit.buttonA.isPressed()) {
                nl.send(message);
            }

            if(uBit.buttonB.isPressed()) {
                uBit.display.print("R");
            }

            uBit.sleep(100);
        }
    }

    release_fiber();
}
