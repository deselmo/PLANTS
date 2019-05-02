#include "MicroBit.h"

MicroBit uBit;

int main()
{
    uBit.init();
    
    uBit.display.scroll("PLANTS");
    
    PacketBuffer p = PacketBuffer::EmptyPacket;

    uBit.display.scroll(p.length());

    if(p == PacketBuffer::EmptyPacket)
        uBit.display.scroll("true");

    release_fiber();
}
