#include "operandstack.h"
#include "memoryinspect.h"

uint8_t pushOperand(OperandStack** os, int32_t value, enum OperandType type)
{
    OperandStack* node = (OperandStack*)malloc(sizeof(OperandStack));

    if (node)
    {
        node->value = value;
        node->type = type;
        node->next = *os;
        *os = node;
    }

    return node != NULL;
}

uint8_t popOperand(OperandStack** os, int32_t* outPtr, enum OperandType* outType)
{
    OperandStack* node = *os;

    if (node)
    {
        if (outPtr)
            *outPtr = node->value;

        if (outType)
            *outType = node->type;

        *os = node->next;
        free(node);
    }

    return node != NULL;
}

void freeOperandStack(OperandStack** os)
{
    OperandStack* node = *os;
    OperandStack* tmp;

    while (node)
    {
        tmp = node;
        node = node->next;
        free(tmp);
    }

    *os = NULL;
}
