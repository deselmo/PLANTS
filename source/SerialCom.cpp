#include "SerialCom.h"

SerialCom::SerialCom(MicroBit *uBit){
    serial = &(uBit->serial);
}

void SerialCom::init(){
    fiber_add_idle_component(this);
    state = 0;
    read = 0;
    buf = NULL;
}

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

void SerialCom::idleTick(){
    switch(state){
        case 0:
            id = serial->read();
            state = 1;
        case 1:
            value = serial->read();
            state = 2;
        case 2:
            read += serial->read(((uint8_t *) &len),sizeof(uint32_t));
            if(read == 4)
            {
                state = 3;
                read = 0;
            }
            else
                break;
        case 3:
            if(buf == NULL)
                buf = new uint8_t [len];
            read += serial->read(buf+read,len-read);
            if(read == len)
            {
                ManagedBuffer b(buf, len);
                uint16_t hash = id << 8;
                hash += value;
                Listener *head = listeners[hash];
                while(head != NULL)
                {
                    head->cb->fire(b);
                    head = head->next;
                }
                delete [] buf;
                state = 0;
                read = 0;
            }
    }
}