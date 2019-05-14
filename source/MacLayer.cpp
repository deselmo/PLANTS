#include "MacLayer.h"
#include "MicroBitDevice.h"

#include <algorithm>


ManagedMacBuffer ManagedMacBuffer::of(MacBuffer& buf){
    ManagedMacBuffer b;
    b.b = ManagedBuffer((uint8_t *)&buf, sizeof(MacBuffer));
    return b;
}

bool ManagedMacBuffer::operator==(const ManagedMacBuffer& other){
    return b == other.b;
}


ManagedMacBuffer ManagedMacBuffer::Empty = ManagedMacBuffer {
    .b = ManagedBuffer { }
};

bool ManagedMacBuffer::queued(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->queued;
}
void ManagedMacBuffer::queued(bool val){
    MacBuffer * buf = (MacBuffer*)b.getBytes();
    buf->queued = val;
}

uint64_t ManagedMacBuffer::timestamp(){
    MacBuffer * buf = (MacBuffer*)b.getBytes();
    return buf->timestamp;
}
void ManagedMacBuffer::timestamp(uint64_t val){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    buf->type = val;
}


uint8_t ManagedMacBuffer::type(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->type;
}

uint8_t ManagedMacBuffer::attempt(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->attempt;
}
void ManagedMacBuffer::attempt(uint8_t val){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    buf->attempt = val;
}

uint32_t ManagedMacBuffer::destination(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->destination;
}

uint32_t ManagedMacBuffer::source(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->source;
}

uint8_t ManagedMacBuffer::frag(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->frag;
}

uint8_t ManagedMacBuffer::seq_number(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->seq_number;
}

uint8_t ManagedMacBuffer::control(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->control;
}

Payload ManagedMacBuffer::payload(){
    MacBuffer* buf = (MacBuffer*)b.getBytes();
    return buf->payload;
}

ManagedFragmentedPacket ManagedFragmentedPacket::of(FragmentedPacket& fp){
    ManagedFragmentedPacket mfp;
    mfp.b = ManagedBuffer((uint8_t *)&fp, sizeof(FragmentedPacket));
    return mfp;
}

bool ManagedFragmentedPacket::operator==(const ManagedFragmentedPacket& other){
    return b == other.b;
}

ManagedFragmentedPacket ManagedFragmentedPacket::Empty = ManagedFragmentedPacket { 
    .b = ManagedBuffer { }
};

uint16_t ManagedFragmentedPacket::length(){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    return buf->length;
}
void ManagedFragmentedPacket::length(uint16_t val){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    buf->length = val;
}

uint64_t ManagedFragmentedPacket::timestamp(){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    return buf->timestamp;
}
void ManagedFragmentedPacket::timestamp(uint64_t val){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    buf->timestamp = val;
}

bool ManagedFragmentedPacket::lastReceived(){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    return buf->lastReceived;
}
void ManagedFragmentedPacket::lastReceived(bool val){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    buf->lastReceived = val;
}

uint8_t ManagedFragmentedPacket::packet_number(){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    return buf->packet_number;
}
void ManagedFragmentedPacket::packet_number(uint8_t val){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    buf->packet_number = val;
}

std::vector<ManagedMacBuffer> ManagedFragmentedPacket::packets(){
    FragmentedPacket* buf = (FragmentedPacket*)b.getBytes();
    return buf->packets;
}

ManagedMacBufferFragmentReceived ManagedMacBufferFragmentReceived::of(MacBufferFragmentReceived& buf){
    ManagedMacBufferFragmentReceived mmbfr;
    mmbfr.b = ManagedBuffer((uint8_t *)&buf, sizeof(MacBufferFragmentReceived));
    return mmbfr;
}

bool ManagedMacBufferFragmentReceived::operator==(const ManagedMacBufferFragmentReceived& other){
    return b == other.b;
}

ManagedMacBufferFragmentReceived ManagedMacBufferFragmentReceived::Empty = ManagedMacBufferFragmentReceived {
    .b = ManagedBuffer { }
 };

std::map<uint8_t, ManagedFragmentedPacket> ManagedMacBufferFragmentReceived::fragmented(){
    MacBufferFragmentReceived * buf = (MacBufferFragmentReceived *)b.getBytes();
    return buf->fragmented;
}

ManagedMacBufferSent ManagedMacBufferSent::of(MacBufferSent& buf){
    ManagedMacBufferSent mmbs;
    mmbs.b = ManagedBuffer((uint8_t *)&buf, sizeof(MacBufferSent));
    return mmbs;
}


ManagedMacBufferSent ManagedMacBufferSent::Empty = ManagedMacBufferSent {
    .b = ManagedBuffer { }
 };

bool ManagedMacBufferSent::operator==(const ManagedMacBufferSent& other){
 return b == other.b;
}

uint8_t ManagedMacBufferSent::received(){
    MacBufferSent *buf = (MacBufferSent *)b.getBytes();
    return buf->received;
}

void ManagedMacBufferSent::received(uint8_t val){
    MacBufferSent *buf = (MacBufferSent *)b.getBytes();
    buf->received = val;
}

uint8_t ManagedMacBufferSent::total(){
    MacBufferSent *buf = (MacBufferSent *)b.getBytes();
    return buf->total;
}


MacLayer::MacLayer(MicroBit* uBit, SerialCom* serial){
    seq_number = 248;
    this->uBit = uBit;
    this->serial = serial;
}

void MacLayer::init(){
    uBit->messageBus.listen(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND, this, &MacLayer::send_to_radio);
    uBit->messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, this, &MacLayer::recv_from_radio);
    fiber_add_idle_component(this);
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
    vector<ManagedMacBuffer> toSend;
    if(len > MAC_LAYER_MAX_POSSIBLE_PAYLOAD_LENGTH)
    {
        return MAC_LAYER_PACKET_TOO_LARGE;
    }
    // if the length of the buffer is larger than the 
    // MAC_LAYER_PAYLOAD_LENGTH then we need to fragment
    if(len > MAC_LAYER_PAYLOAD_LENGTH)
    {
        ManagedBuffer b("fragmenting");
        if(serial != NULL)
            serial->send(MAC_LAYER, 1, b);
        toSend = prepareFragment(buffer, len, dest);
    }
    else
    {
        ManagedBuffer b("not fragmenting");
        if(serial != NULL)
            serial->send(MAC_LAYER, 1, b);
        ManagedMacBuffer tmp = createMacBuffer(0,dest,len,buffer, 0, true);
        toSend.insert(toSend.begin(),tmp);
    }
    macbuffersent++;
    MacBufferSent sent;
    sent.seq_number = seq_number-1;
    sent.total = toSend.size();
    sent.received = 0;
    ManagedMacBufferSent s = ManagedMacBufferSent::of(sent);
    waiting_for_ack[seq_number-1] = s;
    


    //outBuffer is empty
    if(outBuffer.empty())
    {
        outBuffer.insert(outBuffer.begin(), toSend.begin(), toSend.end());
        MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    }
    else
        outBuffer.insert(outBuffer.begin(), toSend.begin(), toSend.end());
    return MAC_LAYER_OK;
}

/**
 * creates 1 MacBuffer
 */
ManagedMacBuffer MacLayer::createMacBuffer(uint8_t type, uint32_t dest, int len, uint8_t *buffer, uint8_t frag, bool inc){
    MacBuffer buf;
    buf.attempt = 0;
    buf.type = type;
    buf.destination = dest;
    buf.source = microbit_serial_number();
    buf.frag = frag;
    buf.seq_number = seq_number;
    buf.control = setControl(len,inc);
    if(inc)
    {
        if(seq_number == 255)
            seq_number = 0;
        else
            seq_number++;
    }
    buf.queued = true;
    memcpy(&(buf.payload.payload), buffer, len);
    return ManagedMacBuffer::of(buf);
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
vector<ManagedMacBuffer> MacLayer::prepareFragment(uint8_t *buffer, int len, uint32_t dest){
    vector<ManagedMacBuffer> ret;
    int j = 1;
    int i;
    //necessary to be always sure to have more data to send after the for cycle
    for(i = 0; i < len - MAC_LAYER_PAYLOAD_LENGTH; i+=MAC_LAYER_PAYLOAD_LENGTH)
    {
        ManagedMacBuffer tmp = createMacBuffer(0, dest, MAC_LAYER_PAYLOAD_LENGTH, buffer+i, j, false);
        ret.insert(ret.begin(),tmp);
        j++;
    }
    ManagedMacBuffer tmp = createMacBuffer(0, dest, len - i, buffer + i, j, true);
    ret.insert(ret.begin(),tmp);
    return ret;
}

/**
 * deletes 1 MacBuffer if is ACK otherwise put 1 MacBuffer into waiting
 */
void MacLayer::send_to_radio(MicroBitEvent){
    ManagedMacBuffer toSend = outBuffer.back();
    PacketBuffer p(MICROBIT_RADIO_MAX_PACKET_SIZE);
    uint8_t *payload = p.getBytes();
    int tmp = sizeof(uint8_t);
    payload[0] = toSend.type();
    uint32_t destination = toSend.destination();
    uint32_t source = toSend.source();
    uint8_t frag = toSend.frag();
    uint8_t seq_number = toSend.seq_number();
    uint8_t control = toSend.control();
    Payload macbuffer = toSend.payload();
    memcpy(payload+tmp,&(destination), sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(payload+tmp,&(source),sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(payload+tmp,&(frag),sizeof(uint8_t));
    tmp += sizeof(uint8_t);

    memcpy(payload+tmp,&(seq_number),sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(payload+tmp,&(control),sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(payload+tmp,(macbuffer.payload),MAC_LAYER_PAYLOAD_LENGTH);
    
    int res = uBit->radio.datagram.send(p);
    if(res == MICROBIT_OK)
    {

        outBuffer.pop_back();
        // the message sent was a data packet and a unicast message then handle the
        // retransmission of the packet
        if(toSend.type() == 0 && toSend.destination() != 0)
        {
            ManagedBuffer b("sent message not brodcast");
            if(serial != NULL)
                serial->send(MAC_LAYER,1,b);
            toSend.timestamp(system_timer_current_time());
            uint16_t hash = getHash(toSend);
            if(waiting[hash] == toSend)
                toSend.attempt(toSend.attempt() + 1);
            else
                waiting[hash] = toSend; 
            toSend.queued(false);  
        }
        // the message sent was an ack packet so nothing more to do   
        else if(toSend.type() == 1)
        {
            ManagedBuffer b("sent ack");
            if(serial != NULL)
                serial->send(MAC_LAYER,1,b);
            macbufferallocated--;
        }
        // the message sent was a brodcast packet
        else
        {
            ManagedBuffer b("sent Brodcast");
            if(serial != NULL)
                serial->send(MAC_LAYER,1,b);
            macbufferallocated--;
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
    PacketBuffer packet = uBit->radio.datagram.recv();
    uint8_t *payload = packet.getBytes();
    macbufferallocated++;
    MacBuffer received;
    int tmp = sizeof(uint8_t);
    memcpy(&(received.type), payload, sizeof(uint8_t));
    memcpy(&(received.destination), payload+tmp, sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(&(received.source),payload+tmp, sizeof(uint32_t));
    tmp += sizeof(uint32_t);
    memcpy(&(received.frag), payload+tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(&(received.seq_number),payload+tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(&(received.control), payload+tmp, sizeof(uint8_t));
    tmp += sizeof(uint8_t);
    memcpy(&(received.payload.payload), payload+tmp, sizeof(uint8_t)*MAC_LAYER_PAYLOAD_LENGTH);

    ManagedMacBuffer recv = ManagedMacBuffer::of(received);

    //received a unicast packet
    if(recv.destination() == microbit_serial_number())
    {
        if(recv.type() == 0)
        {
            
            sendAck(recv);
            
            // if already received just ignore the packet after having sent
            // the ack back
            if(!checkRepetition(recv))
                addToFragmented(recv);
            
        } 
        else if(recv.type() == 1)
            accomplishAck(recv);
           
    }
    // received brodcast message just don't send ack
    else if (recv.destination() == 0)
    {
        if(!checkRepetition(recv))
            addToFragmented(recv);
        
    }
    // received message not for us discard it
    else
    {
        MicroBitEvent evt(MAC_LAYER, 12);
        macbufferallocated--;
    }

}

bool compare_mac_buffers(ManagedMacBuffer first, ManagedMacBuffer second){
    return first.frag() < second.frag();
}

void MacLayer::addToDataReady(ManagedFragmentedPacket buf){
    ManagedBuffer p(buf.length());
    uint8_t *bytes = p.getBytes();
    int len = 0;
    vector<ManagedMacBuffer>::iterator it;
    for(it = buf.packets().begin(); it != buf.packets().end(); ++it)
    {
        uint8_t tmp = getLength((*it).control());
        memcpy(bytes+len,(*it).payload().payload, tmp);
        len += tmp;
        macbufferallocated--;
    }
    inBuffer.insert(inBuffer.begin(), p);
    MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_RECEIVED);
}

void MacLayer::addToFragmented(ManagedMacBuffer fragment){
    if(receive_buffer[fragment.source()] == ManagedMacBufferFragmentReceived::Empty)
    {
        MacBufferFragmentReceived f;
        f.source = fragment.source();
        ManagedMacBufferFragmentReceived ff = ManagedMacBufferFragmentReceived::of(f);
        receive_buffer[fragment.source()] = ff;
    }
    ManagedMacBufferFragmentReceived received_fragment = receive_buffer[fragment.source()];
    if(received_fragment.fragmented()[fragment.seq_number()] == ManagedFragmentedPacket::Empty)
    {
        FragmentedPacket t;
        t.lastReceived = false;
        t.length = 0;
        t.packet_number = 0;
        t.timestamp = 0;
        ManagedFragmentedPacket mfp = ManagedFragmentedPacket::of(t);
        received_fragment.fragmented()[fragment.seq_number()] = mfp;        
    }
    ManagedFragmentedPacket fragments = received_fragment.fragmented()[fragment.seq_number()];
    fragments.packets().push_back(fragment);
    fragments.timestamp(system_timer_current_time());
    if(isLast(fragment))
    {
        fragments.lastReceived(true);
        fragments.packet_number(fragment.frag());
    }
    fragments.length(fragments.length() + getLength(fragment.control()));
    if(fragments.lastReceived())
    {
        if(fragments.packets().size() == fragments.packet_number())
        {
            received_fragment.fragmented().erase(fragment.seq_number());
            std::sort(fragments.packets().begin(),fragments.packets().end(),compare_mac_buffers);
            addToDataReady(fragments);
            /*if(received_fragment->fragmented.empty())
            {
                macbufferfragmentedreceived--;
                delete received_fragment;
                receive_buffer.erase(fragment->source);
            }*/
        }
    }    
}

bool MacLayer::isLast(ManagedMacBuffer fragment){
    uint8_t control = fragment.control();
    return !(control & 1 << 7);
}

bool MacLayer::checkRepetition(ManagedMacBuffer received){
    if(!(receive_buffer[received.source()] == ManagedMacBufferFragmentReceived::Empty))
    {
        ManagedMacBufferFragmentReceived tmp = receive_buffer[received.source()];
        if(!(tmp.fragmented()[received.seq_number()] == ManagedFragmentedPacket::Empty))
        {
            ManagedFragmentedPacket fp = tmp.fragmented()[received.seq_number()];
            vector<ManagedMacBuffer>::iterator it;
            for(it = fp.packets().begin(); it != fp.packets().end(); ++it)
                if(received.seq_number() == (*it).seq_number() &&
                   received.frag() == (*it).frag())
                   return true;
        }
    }
    return false;
}


/**
 * create 1 MacBuffer put into outBuffer
 */
void MacLayer::sendAck(ManagedMacBuffer received){
    MacBuffer ack;
    ack.type = 1;
    ack.destination = received.source();
    ack.source = microbit_serial_number();
    ack.frag = received.frag();
    ack.seq_number = received.seq_number();
    ack.control = 0;
    ManagedMacBuffer buf = ManagedMacBuffer::of(ack);
    if(outBuffer.empty())
    {
        outBuffer.insert(outBuffer.begin(), buf);
        MicroBitEvent evt(MAC_LAYER, MAC_LAYER_PACKET_READY_TO_SEND);
    }
    else
        outBuffer.insert(outBuffer.begin(), buf);  
}

uint8_t MacLayer::getLength(uint8_t control){
   uint8_t mask = 127;
   control &= mask;
   return control;
}

bool MacLayer::isLastFragment(uint8_t control){
    return !(control & (1 << 8));
}

uint16_t MacLayer::getHash(ManagedMacBuffer buf){
    uint16_t ret;
    ret = buf.seq_number();
    ret |= (buf.frag() << 8);
    return ret;
}

/**
 * delete 1 MacBuffer from waiting
 * delete 1 MacBufferSent from waiting_for_ack
 */
void MacLayer::accomplishAck(ManagedMacBuffer ack){
    uint16_t hash = getHash(ack);
    if(!(waiting[hash] == ManagedMacBuffer::Empty))
    {
        ManagedMacBuffer accomplished = waiting[hash];
        //if in the outgoing queue, remove it
        if(accomplished.queued())
            outBuffer.erase(std::remove(outBuffer.begin(), outBuffer.end(), accomplished), outBuffer.end());
        waiting.erase(hash);
    }
    uint8_t seq = ack.seq_number();
    if(!(waiting_for_ack[seq] == ManagedMacBufferSent::Empty))
    {
        ManagedMacBufferSent s = waiting_for_ack[seq];
        s.received(s.received() + 1);
        if(s.total() == s.received())
        {
            waiting_for_ack.erase(seq);      
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
    map<uint16_t, ManagedMacBuffer>::iterator it;
    uint64_t now = system_timer_current_time();
    for(it = waiting.begin(); it != waiting.end();)
    {
        if(serial != NULL)
            serial->send(MAC_LAYER,1,"first for");
        if(it->second.queued())
            ++it;
        else if(now - it->second.timestamp() < MAC_LAYER_RETRANSMISSION_TIME)
            ++it;
        else if(it->second.attempt() < MAC_LAYER_RETRANSMISSION_ATTEMPT)
        {
            if(serial != NULL)
                serial->send(MAC_LAYER,1,"strange");
            it->second.queued(true);
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
            if(serial != NULL)
                serial->send(MAC_LAYER,1,"strange 1");
            ManagedMacBuffer tmp = it->second;
            waiting.erase(it++);
            if(disconnected_destination.size() < 4)
                disconnected_destination.insert(disconnected_destination.begin(), tmp.destination());
            uint8_t seq = tmp.seq_number();
            ManagedMacBufferSent s = waiting_for_ack[seq];
            if(!(s == ManagedMacBufferSent::Empty))
            {
                waiting_for_ack.erase(seq);
            }
            MicroBitEvent evt(MAC_LAYER, MAC_LAYER_TIMEOUT);
        } 
    }
    
    map<uint32_t, ManagedMacBufferFragmentReceived>::iterator it1;
    for(it1 = receive_buffer.begin(); it1 != receive_buffer.end();)
    {
        if(serial != NULL)
            serial->send(MAC_LAYER,1,"impossibile");
        ManagedMacBufferFragmentReceived f = it1->second;
        if(f == ManagedMacBufferFragmentReceived::Empty)
        {
            receive_buffer.erase(it1++);
            continue;
        }
        map<uint8_t, ManagedFragmentedPacket>::iterator it2;
        for(it2 = f.fragmented().begin(); it2 != f.fragmented().end();)
        {
            ManagedFragmentedPacket fp = it2->second;
            if(fp == ManagedFragmentedPacket::Empty)
            {
                f.fragmented().erase(it2++);
                continue;
            }
            if(now - fp.timestamp() < (MAC_LAYER_RETRANSMISSION_ATTEMPT + 1)*MAC_LAYER_RETRANSMISSION_TIME)
            {
                ++it2;
            }
            else
            {
                f.fragmented().erase(it2++);
            }
        }
        if(f.fragmented().empty())
        {
            receive_buffer.erase(it1++);
        }
        else
        {
            ++it1;
        }
        
    }
    //if(serial != NULL)
    //    serial->send(MAC_LAYER,1,"finito");
    
}
// 1