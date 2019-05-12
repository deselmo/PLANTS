#include "SerialCom.h"

SerialCom::SerialCom(MicroBit *uBit){
    this->uBit = uBit;
}

void execute_tasks(void *par){
    SerialCom * serial = (SerialCom *) par;
    while(true)
    {
        switch(serial->state)
        {
            case 0:
                serial->id = serial->uBit->serial.read();
                serial->state = 1;
            case 1:
                serial->value = serial->uBit->serial.read();
                serial->state = 2;
            case 2:
                serial->read += serial->uBit->serial.read(((uint8_t *) &(serial->len)),sizeof(uint32_t));
                if(serial->read == 4)
                {
                    serial->state = 3;
                    serial->read = 0;
                }
                else
                    break;
            case 3:
                if(serial->buf == NULL)
                    serial->buf = new uint8_t [serial->len];
                serial->read += serial->uBit->serial.read(serial->buf+serial->read,serial->len-serial->read);
                if(serial->read == serial->len)
                {
                    ManagedBuffer b(serial->buf, serial->len);
                    uint16_t hash = serial->id << 8;
                    hash += serial->value;
                    Listener *head = serial->listeners[hash];
                    while(head != NULL)
                    {
                        head->cb->fire(b);
                        head = head->next;
                    }
                    delete [] serial->buf;
                    serial->buf = NULL;
                    serial->state = 0;
                    serial->read = 0;
                }
        }
        //serial->uBit->sleep(100);
    }
}

void SerialCom::init(){
    uBit->serial.baud(19200);
    uBit->messageBus.listen(SERIAL_ID, SERIAL_DATA_READY, this, &SerialCom::send_to_serial);
    state = 0;
    read = 0;
    buf = NULL;
    sent = 0;
    create_fiber(&execute_tasks, (void *) this);
}



void SerialCom::send(uint8_t id, uint8_t value, uint8_t *buf, uint32_t len){
    ManagedBuffer b(len + sizeof(uint8_t)*2 + sizeof(uint32_t));
    b[0] = id;
    b[1] = value;
    uint8_t * bytes = b.getBytes();
    memcpy(bytes+sizeof(uint8_t)*2,&len,sizeof(uint32_t));
    memcpy(bytes+sizeof(uint8_t)*2+sizeof(uint32_t),buf,len);
    send(b);
}

void SerialCom::send(uint8_t id, uint8_t value, ManagedBuffer buf){
    send(id,value,buf.getBytes(), buf.length());
}

void SerialCom::send(uint8_t *buf, uint32_t len){
    ManagedBuffer b(buf,len);
    send(b);
}

void SerialCom::send(ManagedBuffer buf){
    if(out.empty())
    {
        out.push(buf);
        MicroBitEvent evt(SERIAL_ID, SERIAL_DATA_READY);
    }
    else
        out.push(buf);   
}

void SerialCom::send_to_serial(MicroBitEvent e){
    ManagedBuffer toSend(out.front());
    sent += uBit->serial.send(toSend.getBytes()+sent, toSend.length() - sent);
    if(sent == toSend.length())
    {
        out.pop();
        sent = 0;
        if(!out.empty())
            MicroBitEvent evt(SERIAL_ID, SERIAL_DATA_READY);
        
    }
    else
        MicroBitEvent evt(SERIAL_ID, SERIAL_DATA_READY);
    
}