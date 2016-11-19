#include "operandstack.h"
#include <stdlib.h>

uint8_t pushOperand(OperandStack** os, int32_t value)
{
    OperandStack* node = (OperandStack*)malloc(sizeof(OperandStack));

    if (node)
    {
        node->value = value;
        node->next = *os;
        *os = node;
    }

    return node != NULL;
}

uint8_t popOperand(OperandStack** os, int32_t* outPtr)
{
    OperandStack* node = *os;

    if (node)
    {
        if (outPtr)
            *outPtr = node->value;

        *os = node->next;
        free(node);
    }

    return node != NULL;
}
