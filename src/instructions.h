#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdlib.h>
#include <stdint.h>
#include "jvm.h"
#include "framestack.h"

typedef uint8_t (*InstructionFunction)(JavaVirtualMachine* jvm, Frame* currentFrame);

InstructionFunction fetchOpcodeFunction(uint8_t opcode);

#endif // INSTRUCTIONS_H
