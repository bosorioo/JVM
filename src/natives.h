#ifndef NATIVES_H
#define NATIVES_H

#include <stdint.h>
#include "jvm.h"

typedef uint8_t(*NativeFunction)(JavaVirtualMachine* jvm, Frame* frame, const uint8_t* descriptor_utf8, int32_t utf8_len);

NativeFunction getNative(const uint8_t* className, int32_t classLen,
                         const uint8_t* methodName, int32_t methodLen,
                         const uint8_t* descriptor, int32_t descrLen);

#endif // NATIVES_H
