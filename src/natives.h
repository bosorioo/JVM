#ifndef NATIVES_H
#define NATIVES_H

#include <stdint.h>
#include "instructions.h"

InstructionFunction getNative(const uint8_t* className, int32_t classLen,
                              const uint8_t* methodName, int32_t methodLen);

#endif // NATIVES_H
