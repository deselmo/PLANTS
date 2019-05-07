#include "MemberFunctionCallbackSerial.h"

/**
  * Calls the method reference held by this MemberFunctionCallback.
  *
  * @param e The event to deliver to the method
  */
void MemberFunctionCallbackSerial::fire(ManagedBuffer e)
{
    invoke(object, method, e);
}

/**
  * A comparison of two MemberFunctionCallback objects.
  *
  * @return true if the given MemberFunctionCallback is equivalent to this one, false otherwise.
  */
bool MemberFunctionCallbackSerial::operator==(const MemberFunctionCallbackSerial &mfc)
{
    return (object == mfc.object && (memcmp(method,mfc.method,sizeof(method))==0));
}
