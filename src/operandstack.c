#include "operandstack.h"
#include "debugging.h"

///@brief Push the operand on the top of OperandStack passed as parameter by reference
///
///@param OperandStack** os - pointer to the OperandStack where the operand will be pushed.
///@param int32_t value - value of the operand.
///@param OperandType type - type of the operand that will be pushed
///
///@return 0 if OperandStack* node is NULL, in other words, if the push was not successful, 1 otherwise
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
///@brief Pop the operand out of the top of OperandStack passed as parameter by reference
///
///@param OperandStack** os - pointer to the OperandStack.
///@param int32_t* outPtr - value of the operand that will be popped.
///@param OperandType outType - type of the operand that will be popped.
///
///@return 0 if the pop operation was not successful, 1 otherwise.
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

///@brief Free all the elements of the OperandStack passed as parameter by reference
///
///@param OperandStack** os - pointer to the OperandStack.
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
