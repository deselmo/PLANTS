#include "ManagedBuffer.h"
#include "ErrorNo.h"

// Create the EmptyPacket reference.
ManagedBuffer ManagedBuffer::EmptyPacket = ManagedBuffer(1);

/**
  * Default Constructor.
  * Creates an empty Packet Buffer.
  *
  * @code
  * ManagedBuffer p();
  * @endcode
  */
ManagedBuffer::ManagedBuffer()
{
    this->init(NULL, 0);
}

/**
  * Constructor.
  * Creates a new ManagedBuffer of the given size.
  *
  * @param length The length of the buffer to create.
  *
  * @code
  * ManagedBuffer p(16);         // Creates a ManagedBuffer 16 bytes long.
  * @endcode
  */
ManagedBuffer::ManagedBuffer(uint32_t length)
{
    this->init(NULL, length);
}

/**
  * Constructor.
  * Creates an empty Packet Buffer of the given size,
  * and fills it with the data provided.
  *
  * @param data The data with which to fill the buffer.
  *
  * @param length The length of the buffer to create.
  *
  * @code
  * uint8_t buf = {13,5,2};
  * ManagedBuffer p(buf, 3);         // Creates a ManagedBuffer 3 bytes long.
  * @endcode
  */
ManagedBuffer::ManagedBuffer(uint8_t *data, uint32_t length)
{
    this->init(data, length);
}

/**
  * Copy Constructor.
  * Add ourselves as a reference to an existing ManagedBuffer.
  *
  * @param buffer The ManagedBuffer to reference.
  *
  * @code
  * ManagedBuffer p();
  * ManagedBuffer p2(p); // Refers to the same packet as p.
  * @endcode
  */
ManagedBuffer::ManagedBuffer(const ManagedBuffer &buffer)
{
    ptr = buffer.ptr;
    ptr->incr();
}

/**
  * Internal constructor-initialiser.
  *
  * @param data The data with which to fill the buffer.
  *
  * @param length The length of the buffer to create.
  */
void ManagedBuffer::init(uint8_t *data, uint32_t length)
{
    ptr = (ManagedPacketData *) malloc(sizeof(ManagedPacketData) + length);
    ptr->init();

    ptr->length = length;

    // Copy in the data buffer, if provided.
    if (data)
        memcpy(ptr->payload, data, length);
}

/**
  * Destructor.
  *
  * Removes buffer resources held by the instance.
  */
ManagedBuffer::~ManagedBuffer()
{
    ptr->decr();
}

/**
  * Copy assign operation.
  *
  * Called when one ManagedBuffer is assigned the value of another using the '=' operator.
  *
  * Decrements our reference count and free up the buffer as necessary.
  *
  * Then, update our buffer to refer to that of the supplied ManagedBuffer,
  * and increase its reference count.
  *
  * @param p The ManagedBuffer to reference.
  *
  * @code
  * uint8_t buf = {13,5,2};
  * ManagedBuffer p1(16);
  * ManagedBuffer p2(buf, 3);
  *
  * p1 = p2;
  * @endcode
  */
ManagedBuffer& ManagedBuffer::operator = (const ManagedBuffer &p)
{
    if(ptr == p.ptr)
        return *this;

    ptr->decr();
    ptr = p.ptr;
    ptr->incr();

    return *this;
}

/**
  * Array access operation (read).
  *
  * Called when a ManagedBuffer is dereferenced with a [] operation.
  *
  * Transparently map this through to the underlying payload for elegance of programming.
  *
  * @code
  * ManagedBuffer p1(16);
  * uint8_t data = p1[0];
  * @endcode
  */
uint8_t ManagedBuffer::operator [] (uint32_t i) const
{
    return ptr->payload[i];
}

/**
  * Array access operation (modify).
  *
  * Called when a ManagedBuffer is dereferenced with a [] operation.
  *
  * Transparently map this through to the underlying payload for elegance of programming.
  *
  * @code
  * ManagedBuffer p1(16);
  * p1[0] = 42;
  * @endcode
  */
uint8_t& ManagedBuffer::operator [] (uint32_t i)
{
    return ptr->payload[i];
}

/**
  * Equality operation.
  *
  * Called when one ManagedBuffer is tested to be equal to another using the '==' operator.
  *
  * @param p The ManagedBuffer to test ourselves against.
  *
  * @return true if this ManagedBuffer is identical to the one supplied, false otherwise.
  *
  * @code
  * MicroBitDisplay display;
  * uint8_t buf = {13,5,2};
  * ManagedBuffer p1();
  * ManagedBuffer p2();
  *
  * if(p1 == p2)                    // will be true
  *     display.scroll("same!");
  * @endcode
  */
bool ManagedBuffer::operator== (const ManagedBuffer& p)
{
    if (ptr == p.ptr)
        return true;
    else
        return (ptr->length == p.ptr->length && (memcmp(ptr->payload, p.ptr->payload, ptr->length)==0));
}

/**
  * Sets the byte at the given index to value provided.
  *
  * @param position The index of the byte to change.
  *
  * @param value The new value of the byte (0-255).
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * ManagedBuffer p1(16);
  * p1.setByte(0,255);              // Sets the first byte in the buffer to the value 255.
  * @endcode
  */
int ManagedBuffer::setByte(uint32_t position, uint8_t value)
{
    if (position < ptr->length)
    {
        ptr->payload[position] = value;
        return MICROBIT_OK;
    }
    else
    {
        return MICROBIT_INVALID_PARAMETER;
    }
}

/**
  * Determines the value of the given byte in the packet.
  *
  * @param position The index of the byte to read.
  *
  * @return The value of the byte at the given position, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * ManagedBuffer p1(16);
  * p1.setByte(0,255);              // Sets the first byte in the buffer to the value 255.
  * p1.getByte(0);                  // Returns 255.
  * @endcode
  */
int ManagedBuffer::getByte(uint32_t position)
{
    if (position < ptr->length)
        return ptr->payload[position];
    else
        return MICROBIT_INVALID_PARAMETER;
}

/**
  * Provide a pointer to a memory location containing the packet data.
  *
  * @return The contents of this packet, as an array of bytes.
  */
uint8_t*ManagedBuffer::getBytes()
{
    return ptr->payload;
}

/**
  * Gets number of bytes in this buffer
  *
  * @return The size of the buffer in bytes.
  *
  * @code
  * ManagedBuffer p1(16);
  * p1.length(); // Returns 16.
  * @endcode
  */
uint32_t ManagedBuffer::length()
{
    return ptr->length;
}
