#ifndef OPERAND_STACK
#define OPERAND_STACK

typedef struct OperandStack OperandStack;

#include <stdint.h>

struct OperandStack
{
    int32_t value;
    struct OperandStack* next;
};

uint8_t pushOperand(OperandStack** os, int32_t value);
uint8_t popOperand(OperandStack** os, int32_t* outPtr);

#endif // OPERAND_STACK
