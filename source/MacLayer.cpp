#include "MacLayer.h"
#include "MicroBitDevice.h"

#include <algorithm>


MacLayer::MacLayer(MicroBit* uBit){
    seq_number = 0;
    this->uBit = uBit;
}

void MacLayer::init(){
    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND, this, &MacLayer::send_to_radio);
    uBit->messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, this, &MacLayer::recv_from_radio);
    system_timer_add_component(this);
}



int MacLayer::send(uint8_t *buffer, int len, uint32_t dest){
    
    // create two MacBuffer pointer one to the first and one to 
    // the last MacBuffer (used in case of fragmentation)
    uBit->serial.send("sending\n");
    vector<MacBuffer *> *toSend;
    if(len > MAC_LAYER_MAX_POSSIBLE_PAYLOAD_LENGTH)
    {
        uBit->serial.send("WTF?\n");
        return MAC_LAYER_PACKET_TOO_LARGE;
    }
    // if the length of the buffer is larger than the 
    // MAC_LAYER_PAYLOAD_LENGTH then we need to fragment
    if(len > MAC_LAYER_PAYLOAD_LENGTH)
    {
        
        toSend = prepareFragment(buffer, len, dest);
    }
    else
    {
        uBit->serial.send("yarrrr!\n");
        toSend = new vector<MacBuffer *>;
        MacBuffer *tmp = createMacBuffer(0,dest,len,buffer, 0, true);
        toSend->insert(toSend->begin(),tmp);
    }
    
    //outBuffer is empty
    if(outBuffer.empty())
    {
        outBuffer.insert(outBuffer.begin(), toSend->begin(), toSend->end());
        MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    }
    else
        outBuffer.insert(outBuffer.begin(), toSend->begin(), toSend->end());
   
    return MAC_LAYER_OK;

}

MacBuffer *MacLayer::createMacBuffer(uint8_t type, uint32_t dest, int len, uint8_t *buffer, uint8_t frag, bool inc){
    MacBuffer *buf = new MacBuffer;
    buf->attempt = 0;
    buf->type = type;
    buf->destination = dest;
    buf->source = microbit_serial_number();
    buf->frag = frag;
    buf->seq_number = seq_number;
    buf->control = setControl(len,inc);
    if(inc)
    {
        if(seq_number == 255)
            seq_number = 0;
        else
            seq_number++;
        
    }
    buf->queued = true;
    memcpy(&(buf->payload), buffer, len);
    return buf;
}

uint8_t MacLayer::setControl(int len, bool inc){
    uint8_t ret = (uint8_t) len;
    uint8_t tmp = getLength(ret);
    if(inc)
        return ret;
    
    ret |= (1 << 7);
    return ret;
}

vector<MacBuffer *> * MacLayer::prepareFragment(uint8_t *buffer, int len, uint32_t dest){
    vector<MacBuffer *> *ret = new vector<MacBuffer *>;
    int j = 1;
    int i;
    //necessary to be always sure to have more data to send after the for cycle
    for(i = 0; i < len - MAC_LAYER_PAYLOAD_LENGTH; i+=MAC_LAYER_PAYLOAD_LENGTH)
    {
        MacBuffer *tmp = createMacBuffer(0, dest, MAC_LAYER_PAYLOAD_LENGTH, buffer+i, j, false);
        ret->insert(ret->begin(),tmp);
        j++;
    }
    MacBuffer *tmp = createMacBuffer(0, dest, len - i, buffer + i, j, true);
    ret->insert(ret->begin(),tmp);
    return ret;
}


void MacLayer::send_to_radio(MicroBitEvent){
    //uBit->serial.send("sending to Radio\n");
    MacBuffer * toSend = outBuffer.back();
    PacketBuffer p(MICROBIT_RADIO_MAX_PACKET_SIZE);
    uint8_t *payload = p.getBytes();
    int tmp = sizeof(uint8_t);
    payload[0] = toSend->type;
    memcpy(payload+tmp,&(toSend->destination), sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(payload+tmp,&(toSend->source),sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(payload+tmp,&(toSend->frag),sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(payload+tmp,&(toSend->seq_number),sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(payload+tmp,&(toSend->control),sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(payload+tmp,&(toSend->payload),MAC_LAYER_PAYLOAD_LENGTH);
    

    /*
    ManagedString * s;
    s = new ManagedString((int)f.length);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)f.version);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)f.group);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)f.protocol);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    /*
    s = new ManagedString((int)toSend->type);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)toSend->destination);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)toSend->source);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)toSend->frag);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)toSend->seq_number);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)toSend->control);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    

    s = new ManagedString((int)f.payload[0]);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    int size = sizeof(uint8_t);
    uint32_t tmp1;
    memcpy(&tmp1,f.payload+size,sizeof(uint32_t));
    size += sizeof(uint32_t);
    s = new ManagedString((int)tmp1);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    memcpy(&tmp1,f.payload+size,sizeof(uint32_t));
    size += sizeof(uint32_t);
    s = new ManagedString((int)tmp1);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    uint8_t tmp2;
    memcpy(&tmp2,f.payload+size,sizeof(uint8_t));
    size += sizeof(uint8_t);
    s = new ManagedString((int)tmp2);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    memcpy(&tmp2,f.payload+size,sizeof(uint8_t));
    size += sizeof(uint8_t);
    s = new ManagedString((int)tmp2);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    memcpy(&tmp2,f.payload+size,sizeof(uint8_t));
    size += sizeof(uint8_t);
    s = new ManagedString((int)tmp2);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    */
    int res = uBit->radio.datagram.send(p);
    if(res == MICROBIT_OK)
    {

        outBuffer.pop_back();
        // the message sent was a data packet and a unicast message then handle the
        // retransmission of the packet
        if(toSend->type == 0 && toSend->destination != 0)
        {
            toSend->timestamp = system_timer_current_time();
            uint16_t hash = getHash(toSend);
            if(waiting[hash] == toSend)
                toSend->attempt++;
            else
                waiting[hash] = toSend; 
            toSend->queued = false;  
        }   
        else
            delete toSend;
        
        if(!outBuffer.empty())
            MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    }
    else if(res == MICROBIT_INVALID_PARAMETER)
    {
        MicroBitEvent evt(MAC_LAYER,MAC_LAYER_RADIO_ERROR);
    }
    else
    {
        MicroBitEvent evt(MAC_LAYER,10);
    }
    
    //uBit->serial.send("till here lmao\n");
    
}
//destination sending E 773474883
//source sending E -1530673668

//destination receiving E 773474883
//source receiving E -2144241731

void MacLayer::recv_from_radio(MicroBitEvent){

    PacketBuffer packet = uBit->radio.datagram.recv();
    /*while(packet != NULL && packet->protocol != MAC_LAYER_PROTOCOL)
    {   
        ManagedString s1((int)packet->protocol);
        uBit->serial.send(s1);
        uBit->serial.send("\n");
        packet = uBit->radio.recv();
    }*/

    uint8_t *payload = packet.getBytes();
    MacBuffer * received = new MacBuffer;
    int tmp = sizeof(uint8_t);
    memcpy(&(received->type), payload, sizeof(uint8_t));
    memcpy(&(received->destination), payload+tmp, sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(&(received->source),payload+tmp, sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(&(received->frag), payload+tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(&(received->seq_number),payload+tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(&(received->control), payload+tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(&(received->payload), payload+tmp, sizeof(uint8_t)*MAC_LAYER_PAYLOAD_LENGTH);


    if(received->destination == microbit_serial_number())
    {
        if(received->type == 0)
        {
            sendAck(received);

            // if already received just ignore the packet after having sent
            // the ack back
            if(checkRepetition(received))
                delete received;
            else
            {
                addToRecentlyReceived(received);
            
            }
            
        } 
        else if(received->type == 1)
        {
            accomplishAck(received);
            delete received;
        }
           
    }
    else
    {
        MicroBitEvent evt(MAC_LAYER, 12);
        delete received;
    }
    

}

void MacLayer::addToRecentlyReceived(MacBuffer *received){
    if(recently_received[received->source] == NULL)
    {
        recently_received[received->source] = new MacBufferReceived;
        recently_received[received->source]->source = received->source;
    }
    MacBufferReceived *tmp = recently_received[received->source];
    uint16_t hash = getHash(received);
    MacControlData *control = new MacControlData;
    control->timestamp = system_timer_current_time();
    control->frag = received->frag;
    control->seq_number = received->seq_number;
    tmp->packets[hash] = control;
    ManagedString *s = new ManagedString((int)recently_received[received->source]->source);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)recently_received[received->source]->packets[hash]->frag);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
    s = new ManagedString((int)recently_received[received->source]->packets[hash]->seq_number);
    uBit->serial.send(*s);
    uBit->serial.send("\n");
    delete s;
}

void MacLayer::addToDataReady(MacBuffer *buf){
  

}

void MacLayer::addToFragmented(MacBuffer *fragment){
    
}

bool compare_mac_buffers(const MacBuffer * first, const MacBuffer * second){
    
}

void MacLayer::orderPackets(FragmentedPacket *fragmented){
    
}


bool MacLayer::checkRepetition(MacBuffer *received){
    if(recently_received[received->source] != NULL)
    {
        MacBufferReceived *tmp = recently_received[received->source];
        uint16_t hash = getHash(received);
        if(tmp->packets[hash] != NULL)
        {
            tmp->packets[hash]->timestamp = system_timer_current_time();
            return true;
        }
    }
    return false;
}


void MacLayer::sendAck(MacBuffer *received){
    MacBuffer *ack = new MacBuffer;
    ack->type = 1;
    ack->destination = received->source;
    ack->source = microbit_serial_number();
    ack->frag = received->frag;
    ack->seq_number = received->seq_number;
    ack->control = 0;
    uBit->serial.send("Sending ACK\n");
    if(outBuffer.empty())
    {
        outBuffer.insert(outBuffer.begin(), ack);
        MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    }
    else
        outBuffer.insert(outBuffer.begin(), ack);  
}

uint8_t MacLayer::getLength(uint8_t control){
   uint8_t mask = 127;
   control &= mask;
   return control;
}

bool MacLayer::isLastFragment(uint8_t control){
    return !(control & (1 << 8));
}

uint16_t MacLayer::getHash(MacBuffer *buf){
    uint16_t ret;
    ret = buf->seq_number;
    ret |= (buf-> frag << 8);
    return ret;
}

void MacLayer::accomplishAck(MacBuffer *ack){
    uint16_t hash = getHash(ack);
    if(waiting[hash] != NULL)
    {
        MacBuffer *accomplished = waiting[hash];
        //if in the outgoing queue, remove it
        if(accomplished->queued)
            outBuffer.erase(std::remove(outBuffer.begin(), outBuffer.end(), accomplished), outBuffer.end());
        waiting.erase(hash);
        delete accomplished;
    }
}


void MacLayer::systemTick(){
    //uBit->serial.send("systemTick\n", ASYNC);
    map<uint16_t, MacBuffer *>::iterator it;
    uint64_t now = system_timer_current_time();
    for(it = waiting.begin(); it != waiting.end();)
    {
        if(it->second->queued)
            ++it;
        else if(now - it->second->timestamp < MAC_LAYER_RETRANSMISSION_TIME)
            ++it;
        else if(it->second->attempt < MAC_LAYER_RETRANSMISSION_ATTEMPT)
        {
            //uBit->serial.send("Retransmitting\n", ASYNC);
            if(outBuffer.empty())
            {
                outBuffer.insert(outBuffer.begin(), it->second);
                MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
            }
            else
                outBuffer.insert(outBuffer.begin(), it->second);
            ++it;
        }
        else
        {
            //uBit->serial.send("Discarding\n", ASYNC);
            MacBuffer *tmp = it->second;
            waiting.erase(it++);
            delete tmp;
            MicroBitEvent evt(MAC_LAYER, MAC_LAYER_TIMEOUT);
        } 
    }
    
    map<uint32_t, MacBufferReceived *>::iterator it1;
    for(it1 = recently_received.begin(); it1 != recently_received.end();)
    {
        /*
        ManagedString s((int)it1->first);
        uBit->serial.send(s);
        uBit->serial.send("\n");
        if(it1->second == NULL)
        {
            ++it1;
            uBit->serial.send("WTF?\n");
            continue;
        }
        */
        map<uint16_t, MacControlData *>::iterator it2;
        for(it2 = it1->second->packets.begin(); it2 != it1->second->packets.end();)
        {
            
            if(it2->second == NULL)
            {
                ++it2;
                uBit->serial.send("WTF2?\n");
                continue;
            }
            else
            {
                uBit->serial.send("OK!\n");
            }
            
            /*
            if(now - it2->second->timestamp < (MAC_LAYER_RETRANSMISSION_ATTEMPT + 1)*MAC_LAYER_RETRANSMISSION_TIME)
            {
                uBit->serial.send("continuing\n");
                ++it2;
            }
            else
            {
                MacControlData *tmp = it2->second;
                it1->second->packets.erase(it2++);
                delete tmp;
                uBit->serial.send("Removing received\n");
            }
            */
            ++it2;
        }
        /*
        if(it1->second->packets.empty())
        {
            MacBufferReceived *tmp = it1->second;
            recently_received.erase(it1++);
            delete tmp;
            uBit->serial.send("Removing entry\n");
        }
        else
        {
            uBit->serial.send("continuing outside\n");
            ++it1;
        }
        */
       ++it1;
    }
    
}