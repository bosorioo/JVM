#ifndef FRAMESTACK_H
#define FRAMESTACK_H

typedef struct Frame Frame;
typedef struct FrameStack FrameStack;

#include "operandstack.h"

struct Frame
{
    OperandStack* operands;
};

struct FrameStack
{
    Frame* frame;
    struct FrameStack* next;
};

void initFrame(Frame* frame);

#endif // FRAMESTACK_H
