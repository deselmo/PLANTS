#include "MicroBit.h"
#include "MemberFunctionCallbackSerial.h"
#include "ManagedBuffer.h"

#include <map>
#include <queue>


struct Listener
{
    MemberFunctionCallbackSerial *cb;
    Listener *next;
};


class SerialCom : MicroBitComponent{

    MicroBitSerial * serial;
    std::map<uint16_t, Listener *> listeners;

    uint8_t id;
    uint8_t value;
    uint32_t len;
    uint8_t *buf;
    uint8_t state;
    uint32_t read;

    void init();

public:
    SerialCom(MicroBit * uBit);
    template <typename T>
    void addListener(uint8_t id, uint8_t value, T* object, void (T::*handler)(ManagedBuffer));

    virtual void idleTick();

};

