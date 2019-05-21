#include "MacLayer.h"
#include "MicroBitDevice.h"
#include "wait_api.h"
#include <math.h>

#include <algorithm>


MacLayer::MacLayer(MicroBit* uBit, SerialCom* serial, int transmitPower){
    seq_number = 248;
    this->uBit = uBit;
    this->serial = serial;
    this->uBit->radio.setTransmitPower(transmitPower);
}

void MacLayer::init(){
    this->uBit->radio.enable();
    this->uBit->seedRandom();

    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND, this, &MacLayer::send_to_radio);
    uBit->messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, this, &MacLayer::recv_from_radio);
    fiber_add_idle_component(this);

    if(serial != NULL) serial->send(MAC_LAYER, 1, "initiated");
}


ManagedBuffer MacLayer::recv(){
    if(inBuffer.empty())
        return ManagedBuffer::EmptyPacket;
    
    ManagedBuffer p = inBuffer.back();
    inBuffer.pop_back();
    return p;
}


int MacLayer::send(ManagedBuffer data, uint32_t dest){
    return send((uint8_t *)data.getBytes(), data.length(), dest);
}

/**
 * create 1 vector and destroy it
 * create n MacBuffer put in outBuffer
 * create 1 MacBufferSent put in waiting_for_ack
 * 
 */
int MacLayer::send(uint8_t *buffer, int len, uint32_t dest){
    
    // create two MacBuffer pointer one to the first and one to 
    // the last MacBuffer (used in case of fragmentation)
    vector<MacBuffer *> *toSend;
    if(len > MAC_LAYER_MAX_POSSIBLE_PAYLOAD_LENGTH)
    {
        return MAC_LAYER_PACKET_TOO_LARGE;
    }
    // if the length of the buffer is larger than the 
    // MAC_LAYER_PAYLOAD_LENGTH then we need to fragment
    if(len > MAC_LAYER_PAYLOAD_LENGTH)
    {
        if(serial != NULL)
            serial->send(MAC_LAYER, 1, "fragmenting");
        toSend = prepareFragment(buffer, len, dest);
    }
    else
    {
        if(serial != NULL)
            serial->send(MAC_LAYER, 1, "not fragmenting");
        toSend = new vector<MacBuffer *>;
        MacBuffer *tmp = createMacBuffer(0,dest,len,buffer, 0, true);
        toSend->insert(toSend->begin(),tmp);
    }
    macbuffersent++;
    if(dest != 0)
    {
        MacBufferSent *sent = new MacBufferSent;
        sent->seq_number = seq_number == 0 ? 255 : seq_number-1;
        sent->total = toSend->size();
        sent->received = 0;
        waiting_for_ack[seq_number-1] = sent;
    }


    //outBuffer is empty
    if(outBuffer.empty())
    {
        outBuffer.insert(outBuffer.begin(), toSend->begin(), toSend->end());
        MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    }
    else
        outBuffer.insert(outBuffer.begin(), toSend->begin(), toSend->end());
    delete toSend;
    return MAC_LAYER_OK;
}

/**
 * creates 1 MacBuffer
 */
MacBuffer *MacLayer::createMacBuffer(uint8_t type, uint32_t dest, int len, uint8_t *buffer, uint8_t frag, bool inc){
    macbufferallocated++;
    MacBuffer *buf = new MacBuffer;
    buf->attempt = 0;
    buf->type = type;
    buf->destination = dest;
    buf->source = microbit_serial_number();
    buf->frag = frag;
    buf->seq_number = seq_number;
    buf->timewait = 0;
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
    if(inc)
        return ret;
    
    ret |= (1 << 7);
    return ret;
}

int MacLayer::getPacketsInQueue(){
    return inBuffer.size();
}

/**
 * create 1 vector
 * create n MacBuffer
 */
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

/**
 * deletes 1 MacBuffer if is ACK otherwise put 1 MacBuffer into waiting
 */
void MacLayer::send_to_radio(MicroBitEvent){
    MacBuffer * toSend = outBuffer.back();
    wait_us(toSend->timewait);
    // if(toSend->timewait > 0)
    // {
    //     this->uBit->sleep(10);
    //     toSend->timewait--;
    //     outBuffer.pop_back();
    //     outBuffer.insert(outBuffer.begin(), toSend);
    //     MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    //     return;
    // }
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
    
    int res = uBit->radio.datagram.send(p);
    if(res == MICROBIT_OK)
    {

        outBuffer.pop_back();
        // the message sent was a data packet and a unicast message then handle the
        // retransmission of the packet
        if(toSend->type == 0 && toSend->destination != 0)
        {
            if(serial != NULL)
                serial->send(MAC_LAYER,1,"sent message not brodcast");
            toSend->timestamp = system_timer_current_time();
            uint16_t hash = getHash(toSend);
            if(waiting[hash] == toSend)
                toSend->attempt++;
            else
                waiting[hash] = toSend; 
            toSend->queued = false;  
        }
        // the message sent was an ack packet so nothing more to do   
        else if(toSend->type == 1)
        {
            if(serial != NULL)
                serial->send(MAC_LAYER,1,"sent ack");
            macbufferallocated--;
            delete toSend;
        }
        // the message sent was a brodcast packet
        else
        {
            if(serial != NULL)
                serial->send(MAC_LAYER,1,"sent Brodcast");
            macbufferallocated--;
            delete toSend;
        }
        
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
    
}
//destination sending E 773474883
//source sending E -1530673668

//destination receiving E 773474883
//source receiving E -2144241731


/**
 * create 1 MacBuffer deleted if not for us or the packet is an ACK or has been already received
 * 
 */
void MacLayer::recv_from_radio(MicroBitEvent){
    if(serial != NULL) serial->send(MAC_LAYER, 1, "something recv");
    
    PacketBuffer packet = uBit->radio.datagram.recv();

    if(packet == PacketBuffer::EmptyPacket) {
        return;
    }

    uint8_t *payload = packet.getBytes();
    macbufferallocated++;
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


    //received a unicast packet
    if(received->destination == microbit_serial_number())
    {
        if(received->type == 0)
        {
            
            sendAck(received);
            
            // if already received just ignore the packet after having sent
            // the ack back
            if(checkRepetition(received))
            {
                macbufferallocated--;
                delete received;
            }
            else
            {
                addToFragmented(received);
            }
            
        } 
        else if(received->type == 1)
        {
            accomplishAck(received);
            delete received;
            macbufferallocated--;
        }
           
    }
    // received brodcast message just don't send ack
    else if (received->destination == 0)
    {
        if(checkRepetition(received))
        {
            macbufferallocated--;
            delete received;
        }
        else
        {
            addToFragmented(received);
        }
        
    }
    // received message not for us discard it
    else
    {
        MicroBitEvent evt(MAC_LAYER, 12);
        macbufferallocated--;
        delete received;
    }

}

bool compare_mac_buffers(const MacBufferReceived * first, const MacBufferReceived * second){
    return first->frag < second->frag;
}

void MacLayer::addToDataReady(FragmentedPacket *buf){
    if(serial != NULL) serial->send(MAC_LAYER, 1, "start addToDataReady");
    if(buf->packets.front()->mac_buffer == NULL)
    {
        if(serial != NULL)
            serial->send(MAC_LAYER, 1, "this packet has already been added to to received one's");
        return;
    }
    ManagedBuffer p(buf->length);
    uint8_t *bytes = p.getBytes();
    int len = 0;
    vector<MacBufferReceived *>::iterator it;
    for(it = buf->packets.begin(); it != buf->packets.end(); ++it)
    {
        MacBuffer *b = (*it)->mac_buffer;
        uint8_t tmp = getLength(b->control);
        memcpy(bytes+len,b->payload, tmp);
        len += tmp;
        macbufferallocated--;
        (*it)->mac_buffer = NULL;
        delete b;
    }
    inBuffer.insert(inBuffer.begin(), p);
    if(serial != NULL) serial->send(MAC_LAYER, 1, "MAC_LAYER_PACKET_RECEIVED");
    MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_RECEIVED);
}

void MacLayer::addToFragmented(MacBuffer *fragment){
    MacBufferReceived *mbr = new MacBufferReceived;
    mbr->frag = fragment->frag;
    mbr->seq_number = fragment->seq_number;
    mbr->mac_buffer = fragment;
    if(serial != NULL) serial->send(MAC_LAYER, 1, "start addToFragmented");
    if(receive_buffer[fragment->source] == NULL)
    {
        macbufferfragmentedreceived++;
        receive_buffer[fragment->source] = new MacBufferFragmentReceived;
        receive_buffer[fragment->source]->source = fragment->source;
    }
    MacBufferFragmentReceived *received_fragment = receive_buffer[fragment->source];
    if(received_fragment->fragmented[fragment->seq_number] == NULL)
    {
        if(serial != NULL) serial->send(MAC_LAYER, 1, "received_fragment->fragmented[fragment->seq_number] == NULL");
        fragmentedpacket++;
        FragmentedPacket *tmp = new FragmentedPacket;
        tmp->lastReceived = false;
        tmp->length = 0;
        tmp->packet_number = 0;
        tmp->timestamp = 0;
        received_fragment->fragmented[fragment->seq_number] = tmp;        
    }
    FragmentedPacket *fragments = received_fragment->fragmented[fragment->seq_number];
    fragments->packets.push_back(mbr);
    fragments->timestamp = system_timer_current_time();
    if(isLast(fragment))
    {
        fragments->lastReceived = true;
        fragments->packet_number = fragment->frag;
    }
    fragments->length += getLength(fragment->control);

    if(serial != NULL) serial->send(MAC_LAYER, 1, "before fragments->lastReceived");
    if(fragments->lastReceived)
    {
        if(serial != NULL) serial->send(MAC_LAYER, 1, "fragments->lastReceived");
        if((fragments->packets.size() == 1 && fragments->packet_number == 0) || fragments->packets.size() == fragments->packet_number)
        {
            if(serial != NULL) serial->send(MAC_LAYER, 1, "fragments->packets.size() == fragments->packet_number");
            std::sort(fragments->packets.begin(),fragments->packets.end(),compare_mac_buffers);
            addToDataReady(fragments);
        }
    }

    
}

    
bool MacLayer::isLast(MacBuffer *fragment){
    uint8_t control = fragment->control;
    return !(control & 1 << 7);
}


bool MacLayer::checkRepetition(MacBuffer *received){
    
    if(receive_buffer[received->source] != NULL)
    {
        MacBufferFragmentReceived *tmp = receive_buffer[received->source];
        if(tmp->fragmented[received->seq_number] != NULL)
        {
            MacBufferReceived * mbr = new MacBufferReceived;
            mbr->seq_number = received->seq_number;
            mbr->frag = received->frag;
            mbr->mac_buffer = received;
            FragmentedPacket *fp = tmp->fragmented[received->seq_number];
            if(std::find_if(fp->packets.begin(), fp->packets.end(), [mbr](MacBufferReceived * x){return mbr->seq_number == x->seq_number && mbr->frag == x->frag;}) != fp->packets.end())
            {
                delete mbr;
                return true;
            }
            delete mbr;
        }
    }
    return false;
}


/**
 * create 1 MacBuffer put into outBuffer
 */
void MacLayer::sendAck(MacBuffer *received){
    macbufferallocated++;
    MacBuffer *ack = new MacBuffer {};
    ack->type = 1;
    ack->destination = received->source;
    ack->source = microbit_serial_number();
    ack->frag = received->frag;
    ack->seq_number = received->seq_number;
    ack->control = 0;
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

/**
 * delete 1 MacBuffer from waiting
 * delete 1 MacBufferSent from waiting_for_ack
 */
void MacLayer::accomplishAck(MacBuffer *ack){
    uint16_t hash = getHash(ack);
    if(waiting[hash] != NULL)
    {
        MacBuffer *accomplished = waiting[hash];
        //if in the outgoing queue, remove it
        if(accomplished->queued)
            outBuffer.erase(std::remove(outBuffer.begin(), outBuffer.end(), accomplished), outBuffer.end());
        waiting.erase(hash);
        macbufferallocated--;
        delete accomplished;
    }
    uint8_t seq = ack->seq_number;
    if(waiting_for_ack[seq] != NULL)
    {
        MacBufferSent *s = waiting_for_ack[seq];
        s->received++;
        if(s->total == s->received)
        {
            waiting_for_ack.erase(seq);
            macbuffersent--;
            delete s;        
            MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_SENT);
        }
    }
}


uint32_t MacLayer::getDisconnectedDestination(){
    if(disconnected_destination.empty())
        return 0;
    uint32_t ret = disconnected_destination.back();
    disconnected_destination.pop_back();
    return ret;
}


void MacLayer::idleTick(){
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
            it->second->queued = true;
            // it->second->timewait = uBit->random(it->second->attempt);
            it->second->timewait = uBit->random((pow(2, it->second->attempt) - 1) * 100);
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
            MacBuffer *tmp = it->second;
            waiting.erase(it++);
            if(disconnected_destination.size() < 4)
                disconnected_destination.insert(disconnected_destination.begin(), tmp->destination);
            uint8_t seq = tmp->seq_number;
            MacBufferSent *s = waiting_for_ack[seq];
            if(s != NULL)
            {
                waiting_for_ack.erase(seq);
                macbuffersent--;
                delete s;
            }
            macbufferallocated--;
            delete tmp;
            MicroBitEvent evt(MAC_LAYER, MAC_LAYER_TIMEOUT);
        } 
    }
    
    map<uint32_t, MacBufferFragmentReceived *>::iterator it1;
    for(it1 = receive_buffer.begin(); it1 != receive_buffer.end();)
    {
        MacBufferFragmentReceived *f = it1->second;
        if(f == NULL)
        {
            receive_buffer.erase(it1++);
            continue;
        }
        map<uint8_t, FragmentedPacket *>::iterator it2;
        for(it2 = f->fragmented.begin(); it2 != f->fragmented.end();)
        {
            FragmentedPacket *fp = it2->second;
            if(fp == NULL)
            {
                f->fragmented.erase(it2++);
                continue;
            }
            if(now - fp->timestamp < (MAC_LAYER_RETRANSMISSION_ATTEMPT + 1)*MAC_LAYER_RETRANSMISSION_TIME)
            {
                ++it2;
            }
            else
            {
                vector<MacBufferReceived *>::iterator it3;
                for(it3 = fp->packets.begin(); it3 != fp->packets.end();)
                {
                    MacBufferReceived *mbr = *it3;
                    if(mbr != NULL)
                    {
                        MacBuffer *mb = mbr->mac_buffer;
                        if(mb != NULL)
                            delete mb;
                        delete mbr;
                    }
                    ++it3;                    
                }
                f->fragmented.erase(it2++);
                delete fp;
            }
        }
        
        if(f->fragmented.empty())
        {
            delete f;
            macbufferfragmentedreceived--;
            receive_buffer.erase(it1++);
        }
        else
        {
            ++it1;
        }
        
    }
    
}