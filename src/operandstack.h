#ifndef OPERAND_STACK
#define OPERAND_STACK

typedef struct OperandStack OperandStack;

#include <stdint.h>

typedef enum OperandType {
    OP_INTEGER, OP_FLOAT, OP_LONG, OP_DOUBLE,
    OP_NULL, OP_ARRAYREF, OP_STRINGREF, OP_CLASSREF,
    OP_METHODREF, OP_OBJECTREF
} OperandType;

struct OperandStack
{
    int32_t value;
    OperandType type;
    OperandStack* next;
};

uint8_t pushOperand(OperandStack** os, int32_t value, OperandType type);
uint8_t popOperand(OperandStack** os, int32_t* outPtr, OperandType* outType);
void freeOperandStack(OperandStack** os);

#endif // OPERAND_STACK
