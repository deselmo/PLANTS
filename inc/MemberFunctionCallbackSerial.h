#ifndef MEMBER_FUNCTION_CALLBACK_SERIAL_H
#define MEMBER_FUNCTION_CALLBACK_SERIAL_H

#include "mbed.h"
#include "ManagedBuffer.h"
#include "PacketBuffer.h"
#include "MicroBitCompat.h"

class MemberFunctionCallbackSerial{
    private:
    void* object;
    uint32_t method[4];
    void (*invoke)(void *object, uint32_t *method, ManagedBuffer e);
    template <typename T> static void methodCall(void* object, uint32_t*method, ManagedBuffer e);

    public:

    /**
      * Constructor. Creates a MemberFunctionCallback based on a pointer to given method.
      *
      * @param object The object the callback method should be invooked on.
      *
      * @param method The method to invoke.
      */
    template <typename T> MemberFunctionCallbackSerial(T* object, void (T::*method)(ManagedBuffer e));

    /**
      * A comparison of two MemberFunctionCallback objects.
      *
      * @return true if the given MemberFunctionCallback is equivalent to this one, false otherwise.
      */
    bool operator==(const MemberFunctionCallbackSerial &mfc);

    /**
      * Calls the method reference held by this MemberFunctionCallback.
      *
      * @param e The event to deliver to the method
      */
    void fire(ManagedBuffer e);
};

/**
  * Constructor. Creates a MemberFunctionCallback based on a pointer to given method.
  *
  * @param object The object the callback method should be invooked on.
  *
  * @param method The method to invoke.
  */
template <typename T>
MemberFunctionCallbackSerial::MemberFunctionCallbackSerial(T* object, void (T::*method)(ManagedBuffer e))
{
    this->object = object;
    memclr(this->method, sizeof(this->method));
    memcpy(this->method, &method, sizeof(method));
    invoke = &MemberFunctionCallbackSerial::methodCall<T>;
}

/**
  * A template used to create a static method capable of invoking a C++ member function (method)
  * based on the given parameters.
  *
  * @param object The object the callback method should be invooked on.
  *
  * @param method The method to invoke.
  *
  * @param method The MicroBitEvent to supply to the given member function.
  */
template <typename T>
void MemberFunctionCallbackSerial::methodCall(void *object, uint32_t *method, ManagedBuffer e)
{
    T* o = (T*)object;
    void (T::*m)(ManagedBuffer);
    memcpy(&m, method, sizeof(m));

    (o->*m)(e);
}

#endif