#ifndef FRAMESTACK_H
#define FRAMESTACK_H

typedef struct Frame Frame;
typedef struct FrameStack FrameStack;

#include "operandstack.h"
#include "attributes.h"

struct Frame
{
    JavaClass* jc;

    // Use strict floating points?
    uint8_t fp_strict;

    // How many operands should be moved from this frame to
    // the caller frame when the method returns?
    uint8_t returnCount;

    uint32_t pc, code_length;
    uint8_t* code;

    OperandStack* operands;
    int32_t* localVariables;

#ifdef DEBUG
    uint16_t max_locals;
#endif
};

struct FrameStack
{
    Frame* frame;
    struct FrameStack* next;
};

Frame* newFrame(JavaClass* jc, method_info* method);
void freeFrame(Frame* frame);
uint8_t pushFrame(FrameStack** fs, Frame* frame);
uint8_t popFrame(FrameStack** fs, Frame* outPtr);
void freeFrameStack(FrameStack** fs);

#endif // FRAMESTACK_H
