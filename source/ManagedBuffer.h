#ifndef DD_MANAGED_BUFFER_H
#define DD_MANAGED_BUFFER_H

#include "RefCounted.h"

struct ManagedPacketData : RefCounted
{
    uint32_t        length;             // The length of the payload in bytes
    uint8_t         payload[0];         // User / higher layer protocol data
};

/**
  * Class definition for a ManagedBuffer.
  * A ManagedBuffer holds a series of bytes that can be sent or received from the MicroBitRadio channel.
  *
  * @note This is a mutable, managed type.
  */
class ManagedBuffer
{
    ManagedPacketData      *ptr;     // Pointer to payload data

    public:

    /**
      * Provide a pointer to a memory location containing the packet data.
      *
      * @return The contents of this packet, as an array of bytes.
      */
    uint8_t *getBytes();

    /**
      * Default Constructor.
      * Creates an empty Packet Buffer.
      *
      * @code
      * ManagedBuffer p();
      * @endcode
      */
    ManagedBuffer();

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
    ManagedBuffer(int length);

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
    ManagedBuffer(uint8_t *data, int length);

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
    ManagedBuffer(const ManagedBuffer &buffer);

    /**
      * Internal constructor-initialiser.
      *
      * @param data The data with which to fill the buffer.
      *
      * @param length The length of the buffer to create.
      */
    void init(uint8_t *data, int length);

    /**
      * Destructor.
      *
      * Removes buffer resources held by the instance.
      */
    ~ManagedBuffer();

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
    ManagedBuffer& operator = (const ManagedBuffer& p);

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
    uint8_t operator [] (int i) const;

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
    uint8_t& operator [] (int i);

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
    bool operator== (const ManagedBuffer& p);

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
    int setByte(int position, uint8_t value);

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
    int getByte(int position);

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
    uint32_t length();

    static ManagedBuffer EmptyPacket;
};

#endif
