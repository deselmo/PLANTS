#include "MicroBit.h"

#include "MacLayer.h"

MicroBit uBit;
MacLayer m(&uBit);

void f(){
    m.printToSerial("maccontroldata "+ManagedString(m.maccontroldata));
    m.printToSerial("macbuffersent "+ManagedString(m.macbuffersent));
    m.printToSerial("macbuffer "+ManagedString(m.macbufferallocated));
    m.printToSerial("macbufferreceived "+ManagedString(m.macbufferreceived));
    m.printToSerial("macbufferfragmentedreceived "+ManagedString(m.macbufferfragmentedreceived));
    m.printToSerial("fragmentedpacket "+ManagedString(m.fragmentedpacket));
    m.printToSerial("PacketsBuffers "+ManagedString(m.getPacketsInQueue()));
}

void f1(MicroBitEvent){
    uBit.serial.send("Retransmitting\n");
}

void f2(MicroBitEvent){
    uBit.serial.send("Error not supported\n");
}

void f3(MicroBitEvent){
    uBit.serial.send("Error invalid parameter\n");
}

void f4(MicroBitEvent){
    uBit.serial.send("Received a packet not for me\n");
}

void f5(MicroBitEvent){
    uBit.serial.send("Received a packet not for me\n");
}

void f6(MicroBitEvent){
    uBit.serial.send("Packet completely sent\n");
}

void f7(MicroBitEvent){
    ManagedString p = m.recv();
    uBit.serial.send(p+"\n",SYNC_SPINWAIT);
    m.recv();
}


void printMemoryAndStop() {
    int len = 1;
    uBit.serial.printf("Checking memory with blocksize %d char ...\n", 1);
    while (true) {
        char *p = (char *) malloc(len);
        if (p == NULL) break; // this actually never happens, get a panic(20) outOfMemory first
        delete p;
        len++;
    }
    uBit.serial.printf("%d bytes", len);
}

void sendFake(){

}
int main()
{
    uBit.init();
    m.init();
    
    uint32_t id = microbit_serial_number();
    ManagedString s((int)id);
    uBit.serial.send(s);

    //uBit.messageBus.listen(MAC_LAYER, MAC_LAYER_TIMEOUT, f);
    //uBit.messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND, f1);
    //uBit.messageBus.listen(MAC_LAYER, MAC_LAYER_RADIO_ERROR, f2);
    //uBit.messageBus.listen(MAC_LAYER, 10, f3);
    //uBit.messageBus.listen(MAC_LAYER, 12, f4);
    //uBit.messageBus.listen(MAC_LAYER, 11, f5);
    uBit.messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_SENT, f6);
    uBit.messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_RECEIVED, f7);

    if(uBit.radio.enable() != MICROBIT_OK)
        uBit.panic();
    /*uint8_t arr[120];
    for(int i = 0; i < 60; i++)
        arr[i] = 'A';
    for(int i = 60; i < 120; i++)
        arr[i] = 'B';
    */
    if(ble_running())
        uBit.panic();

    while(1)
    {
        uBit.display.scroll("...");
    
        //m.send(arr, 120, (uint32_t) -1530673668);
        if(uBit.buttonA.isPressed())
            sendFake();
        uBit.sleep(300);
        f();
    }
    
    
    //release_fiber();
}
