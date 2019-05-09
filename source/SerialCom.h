#ifndef SERIAL_COM_H
#define SERIAL_COM_H


#define SERIAL_ID 300
#define SERIAL_DATA_READY 1

#include "mbed.h"
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


class SerialCom {

    MicroBit * uBit;
    std::map<uint16_t, Listener *> listeners;
    std::queue<ManagedBuffer> out;

    uint8_t id;
    uint8_t value;
    uint32_t len;
    uint32_t sent;
    uint8_t *buf;
    uint8_t state;
    uint32_t read;

    
    void send_to_serial(MicroBitEvent);

public:
    SerialCom(MicroBit *);
    void init();
    friend void execute_tasks(void *);
    template <typename T>
    void addListener(uint8_t, uint8_t, T* object, void (T::*handle)(ManagedBuffer));
    void send(uint8_t, uint8_t, uint8_t *, uint32_t);
    void send(uint8_t, uint8_t, ManagedBuffer);
    void send(uint8_t *, uint32_t);
    void send(ManagedBuffer);
};


template <typename T>
void SerialCom::addListener(uint8_t id, uint8_t value, T* object, void (T::*handler)(ManagedBuffer)){
    Listener * tmp = new Listener;
    MemberFunctionCallbackSerial * cb = new MemberFunctionCallbackSerial(object, handler);
    tmp->cb = cb;
    uint16_t hash = id << 8;
    hash += value;
    if(listeners[hash] != NULL)
    {
        tmp->next = listeners[hash];
        listeners[hash] = tmp;
    }
    else
    {
        tmp->next = NULL;
        listeners[hash] = tmp;
    } 
}

#endif