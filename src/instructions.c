#include "instructions.h"

#define NEXT_BYTE (*(frame->code + frame->pc++))

uint8_t instfunc_nop(JavaVirtualMachine* jvm, Frame* frame)
{
    return 1;
}

uint8_t instfunc_aconstnull(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, 0, OP_NULL))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_CONST_CAT_1_FAMILY(instructionprefix, value, type) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, value, type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_CONST_CAT_2_FAMILY(instructionprefix, highvalue, lowvalue, type) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, highvalue, type) || \
            !pushOperand(&frame->operands, lowvalue,  type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_CONST_CAT_1_FAMILY(iconst_m1, -1, OP_INTEGER)
DECLR_CONST_CAT_1_FAMILY(iconst_0, 0, OP_INTEGER)
DECLR_CONST_CAT_1_FAMILY(iconst_1, 1, OP_INTEGER)
DECLR_CONST_CAT_1_FAMILY(iconst_2, 2, OP_INTEGER)
DECLR_CONST_CAT_1_FAMILY(iconst_3, 3, OP_INTEGER)
DECLR_CONST_CAT_1_FAMILY(iconst_4, 4, OP_INTEGER)
DECLR_CONST_CAT_1_FAMILY(iconst_5, 5, OP_INTEGER)

DECLR_CONST_CAT_2_FAMILY(lconst_0, 0, 0, OP_LONG)
DECLR_CONST_CAT_2_FAMILY(lconst_1, 0, 1, OP_LONG)

DECLR_CONST_CAT_1_FAMILY(fconst_0, 0x00000000, OP_FLOAT)
DECLR_CONST_CAT_1_FAMILY(fconst_1, 0x3F800000, OP_FLOAT)
DECLR_CONST_CAT_1_FAMILY(fconst_2, 0x40000000, OP_FLOAT)

DECLR_CONST_CAT_2_FAMILY(dconst_0, 0x00000000, 0x00000000, OP_DOUBLE)
DECLR_CONST_CAT_2_FAMILY(dconst_1, 0x3FF00000, 0x00000000, OP_DOUBLE)


uint8_t instfunc_bipush(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, (int8_t)NEXT_BYTE, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_sipush(JavaVirtualMachine* jvm, Frame* frame)
{
    int16_t immediate = (int16_t)NEXT_BYTE;
    immediate <<= 8;
    immediate |= NEXT_BYTE;

    if (!pushOperand(&frame->operands, immediate, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc(JavaVirtualMachine* jvm, Frame* frame)
{
    uint32_t value = (uint32_t)NEXT_BYTE;

    enum OperandType type;

    cp_info* cpi = frame->jc->constantPool + value - 1;

    switch (cpi->tag)
    {
        case CONSTANT_Float:
            value = cpi->Float.bytes;
            type = OP_FLOAT;
            break;

        case CONSTANT_Integer:
            value = cpi->Integer.value;
            type = OP_INTEGER;
            break;

        case CONSTANT_String:
            type = OP_STRINGREF;
            break;

        case CONSTANT_Class:

            cpi = frame->jc->constantPool + cpi->Class.name_index - 1;

            if (!resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length))
                return 0;

            type = OP_CLASSREF;
            break;

        case CONSTANT_Methodref:
            // TODO: Resolve method
            type = OP_METHODREF;
            break;

        default:
            // Shouldn't happen
            return 0;
    }

    if (!pushOperand(&frame->operands, value, type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc_w(JavaVirtualMachine* jvm, Frame* frame)
{
    uint32_t value = (uint32_t)NEXT_BYTE;
    value <<= 8;
    value |= NEXT_BYTE;

    enum OperandType type;

    cp_info* cpi = frame->jc->constantPool + value - 1;

    switch (cpi->tag)
    {
        case CONSTANT_Float:
            value = cpi->Float.bytes;
            type = OP_FLOAT;
            break;

        case CONSTANT_Integer:
            value = cpi->Integer.value;
            type = OP_INTEGER;
            break;

        case CONSTANT_String:
            type = OP_STRINGREF;
            break;

        case CONSTANT_Class:

            cpi = frame->jc->constantPool + cpi->Class.name_index - 1;

            if (!resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length))
                return 0;

            type = OP_CLASSREF;
            break;

        case CONSTANT_Methodref:
            // TODO: Resolve method
            type = OP_METHODREF;
            break;

        default:
            // Shouldn't happen
            return 0;
    }

    if (!pushOperand(&frame->operands, (int32_t)value, type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc2_w(JavaVirtualMachine* jvm, Frame* frame)
{
    uint32_t highvalue;
    uint32_t lowvalue = (uint32_t)NEXT_BYTE;
    lowvalue <<= 8;
    lowvalue |= NEXT_BYTE;

    enum OperandType type;

    cp_info* cpi = frame->jc->constantPool + lowvalue - 1;

    switch (cpi->tag)
    {
        case CONSTANT_Long:
            highvalue = cpi->Long.high;
            lowvalue = cpi->Long.low;
            type = OP_LONG;
            break;

        case CONSTANT_Double:
            highvalue = cpi->Double.high;
            lowvalue = cpi->Double.low;
            type = OP_DOUBLE;
            break;

        default:
            // Shouldn't happen
            return 0;
    }

    if (!pushOperand(&frame->operands, highvalue, type) ||
        !pushOperand(&frame->operands, lowvalue,  type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_iload(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, *(frame->localVariables + NEXT_BYTE), OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_lload(JavaVirtualMachine* jvm, Frame* frame)
{
    uint8_t index = NEXT_BYTE;

    if (!pushOperand(&frame->operands, *(frame->localVariables + index), OP_LONG) ||
        !pushOperand(&frame->operands, *(frame->localVariables + index + 1), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fload(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, *(frame->localVariables + NEXT_BYTE), OP_FLOAT))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dload(JavaVirtualMachine* jvm, Frame* frame)
{
    uint8_t index = NEXT_BYTE;

    if (!pushOperand(&frame->operands, *(frame->localVariables + index), OP_DOUBLE) ||
        !pushOperand(&frame->operands, *(frame->localVariables + index + 1), OP_DOUBLE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_aload(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, *(frame->localVariables + NEXT_BYTE), OP_OBJECTREF))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_CAT_1_LOAD_N_FAMILY(instructionprefix, value, type) \
    uint8_t instfunc_##instructionprefix##_##value(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, *(frame->localVariables + value), type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_CAT_2_LOAD_N_FAMILY(instructionprefix, value, type) \
    uint8_t instfunc_##instructionprefix##_##value(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, *(frame->localVariables + value), type) || \
            !pushOperand(&frame->operands, *(frame->localVariables + value + 1), type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_CAT_1_LOAD_N_FAMILY(iload, 0, OP_INTEGER)
DECLR_CAT_1_LOAD_N_FAMILY(iload, 1, OP_INTEGER)
DECLR_CAT_1_LOAD_N_FAMILY(iload, 2, OP_INTEGER)
DECLR_CAT_1_LOAD_N_FAMILY(iload, 3, OP_INTEGER)

DECLR_CAT_2_LOAD_N_FAMILY(lload, 0, OP_LONG)
DECLR_CAT_2_LOAD_N_FAMILY(lload, 1, OP_LONG)
DECLR_CAT_2_LOAD_N_FAMILY(lload, 2, OP_LONG)
DECLR_CAT_2_LOAD_N_FAMILY(lload, 3, OP_LONG)

DECLR_CAT_1_LOAD_N_FAMILY(fload, 0, OP_FLOAT)
DECLR_CAT_1_LOAD_N_FAMILY(fload, 1, OP_FLOAT)
DECLR_CAT_1_LOAD_N_FAMILY(fload, 2, OP_FLOAT)
DECLR_CAT_1_LOAD_N_FAMILY(fload, 3, OP_FLOAT)

DECLR_CAT_2_LOAD_N_FAMILY(dload, 0, OP_DOUBLE)
DECLR_CAT_2_LOAD_N_FAMILY(dload, 1, OP_DOUBLE)
DECLR_CAT_2_LOAD_N_FAMILY(dload, 2, OP_DOUBLE)
DECLR_CAT_2_LOAD_N_FAMILY(dload, 3, OP_DOUBLE)

DECLR_CAT_1_LOAD_N_FAMILY(aload, 0, OP_OBJECTREF)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 1, OP_OBJECTREF)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 2, OP_OBJECTREF)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 3, OP_OBJECTREF)

uint8_t instfunc_iaload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_laload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_faload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_daload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_aaload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_baload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_caload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_saload(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

#define DECLR_STORE_CAT_1_FAMILY(instructionprefix) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t operand; \
        popOperand(&frame->operands, &operand); \
        *(frame->localVariables + NEXT_BYTE) = operand; \
        return 1; \
    }

#define DECLR_STORE_CAT_2_FAMILY(instructionprefix) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint8_t index = NEXT_BYTE; \
        int32_t highoperand; \
        int32_t lowoperand; \
        popOperand(&frame->operands, &lowoperand); \
        popOperand(&frame->operands, &highoperand); \
        *(frame->localVariables + index) = highoperand; \
        *(frame->localVariables + index + 1) = lowoperand; \
        return 1; \
    }

DECLR_STORE_CAT_1_FAMILY(istore)
DECLR_STORE_CAT_2_FAMILY(lstore)
DECLR_STORE_CAT_1_FAMILY(fstore)
DECLR_STORE_CAT_2_FAMILY(dstore)
DECLR_STORE_CAT_1_FAMILY(astore)

#define DECLR_STORE_N_CAT_1_FAMILY(instructionprefix, N) \
    uint8_t instfunc_##instructionprefix##_##N(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t operand; \
        popOperand(&frame->operands, &operand); \
        *(frame->localVariables + N) = operand; \
        return 1; \
    }

#define DECLR_STORE_N_CAT_2_FAMILY(instructionprefix, N) \
    uint8_t instfunc_##instructionprefix##_##N(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t highoperand; \
        int32_t lowoperand; \
        popOperand(&frame->operands, &lowoperand); \
        popOperand(&frame->operands, &highoperand); \
        *(frame->localVariables + N) = highoperand; \
        *(frame->localVariables + N + 1) = lowoperand; \
        return 1; \
    }

DECLR_STORE_N_CAT_1_FAMILY(istore, 0)
DECLR_STORE_N_CAT_1_FAMILY(istore, 1)
DECLR_STORE_N_CAT_1_FAMILY(istore, 2)
DECLR_STORE_N_CAT_1_FAMILY(istore, 3)

DECLR_STORE_N_CAT_2_FAMILY(lstore, 0)
DECLR_STORE_N_CAT_2_FAMILY(lstore, 1)
DECLR_STORE_N_CAT_2_FAMILY(lstore, 2)
DECLR_STORE_N_CAT_2_FAMILY(lstore, 3)

DECLR_STORE_N_CAT_1_FAMILY(fstore, 0)
DECLR_STORE_N_CAT_1_FAMILY(fstore, 1)
DECLR_STORE_N_CAT_1_FAMILY(fstore, 2)
DECLR_STORE_N_CAT_1_FAMILY(fstore, 3)

DECLR_STORE_N_CAT_2_FAMILY(dstore, 0)
DECLR_STORE_N_CAT_2_FAMILY(dstore, 1)
DECLR_STORE_N_CAT_2_FAMILY(dstore, 2)
DECLR_STORE_N_CAT_2_FAMILY(dstore, 3)

DECLR_STORE_N_CAT_1_FAMILY(astore, 0)
DECLR_STORE_N_CAT_1_FAMILY(astore, 1)
DECLR_STORE_N_CAT_1_FAMILY(astore, 2)
DECLR_STORE_N_CAT_1_FAMILY(astore, 3)

uint8_t instfunc_iastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_lastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_fastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_dastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_aastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_bastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_castore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_sastore(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction
    return 0;
}

uint8_t instfunc_pop(JavaVirtualMachine* jvm, Frame* frame)
{
    popOperand(&frame->operands, NULL);
    return 1;
}

uint8_t instfunc_pop2(JavaVirtualMachine* jvm, Frame* frame)
{
    popOperand(&frame->operands, NULL);
    popOperand(&frame->operands, NULL);
    return 1;
}

uint8_t instfunc_dup(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, frame->operands->value, frame->operands->type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

uint8_t instfunc_dup_x1(JavaVirtualMachine* jvm, Frame* frame)
{
    // Suppose our stack is: A -> B -> ...
    // We want to duplicate A (top node), but moving it down 2 slots
    // The stack should look like: A -> B -> A -> ...


    // We duplicate the top operand, so the stack will be: A -> A -> B -> ...
    // Calling one of the duplicated A as C, the stack is: A -> C -> B -> ...
    // And our objective (using node C terminology) is:    A -> B -> C -> ...
    if (!pushOperand(&frame->operands, frame->operands->value, frame->operands->type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    // Current stack is: A -> C -> B -> ...
    OperandStack* nodeA = frame->operands;
    OperandStack* nodeC = nodeA->next;
    OperandStack* nodeB = nodeC->next;

    // Switching node C and node B:
    nodeA->next = nodeB;        // Stack: A -> B -> ...  |  C -> B -> ...
    nodeC->next = nodeB->next;  // Stack: A -> B -> ...  |  C -> ...
    nodeB->next = nodeC;        // Stack: A -> B -> C -> ...

    // Another implementation (changing content instead of pointers):
    /*
    OperandStack temp = *nodeC;
    *nodeC = *nodeB;
    *nodeB = temp;
    */

    return 1;
}

uint8_t instfunc_dup_x2(JavaVirtualMachine* jvm, Frame* frame)
{
    // Suppose our stack is: A -> B -> C -> ...
    // We want to duplicate A (top node), but moving it down 3 slots
    // The stack should look like: A -> B -> C-> A -> ...


    // We duplicate the top operand, so the stack will be: A -> A -> B -> C -> ...
    // Calling one of the duplicated A as D, the stack is: A -> D -> B -> C -> ...
    // And our objective (using node D terminology) is:    A -> B -> C -> D -> ...
    if (!pushOperand(&frame->operands, frame->operands->value, frame->operands->type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    // Current stack is: A -> D -> B -> C -> ...
    OperandStack* nodeA = frame->operands;
    OperandStack* nodeD = nodeA->next;
    OperandStack* nodeB = nodeD->next;
    OperandStack* nodeC = nodeB->next;

    // Switching node D and node C:
    nodeA->next = nodeB;        // Stack: A -> B -> C -> ...  |  D -> B -> C -> ...
    nodeD->next = nodeC->next;  // Stack: A -> B -> C -> ...  |  D -> ...
    nodeC->next = nodeD;        // Stack: A -> B -> C -> D -> ...

    return 1;
}

uint8_t instfunc_dup2(JavaVirtualMachine* jvm, Frame* frame)
{
    OperandStack* node1 = frame->operands;
    OperandStack* node2 = node1->next;

    if (!pushOperand(&frame->operands, node2->value, node2->type) ||
        !pushOperand(&frame->operands, node1->value, node1->type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

InstructionFunction fetchOpcodeFunction(uint8_t opcode)
{
    const InstructionFunction opcodeFunctions[255] = {
        instfunc_nop, instfunc_aconstnull, instfunc_iconst_m1,
        instfunc_iconst_0, instfunc_iconst_1, instfunc_iconst_2,
        instfunc_iconst_3, instfunc_iconst_4, instfunc_iconst_5,
        instfunc_lconst_0, instfunc_lconst_1, instfunc_fconst_0,
        instfunc_fconst_1, instfunc_fconst_2, instfunc_dconst_0,
        instfunc_dconst_1, instfunc_bipush, instfunc_sipush,
        instfunc_ldc, instfunc_ldc_w, instfunc_ldc2_w,
        instfunc_iload, instfunc_lload, instfunc_fload,
        instfunc_dload, instfunc_aload, instfunc_iload_0,
        instfunc_iload_1, instfunc_iload_2, instfunc_iload_3,
        instfunc_lload_0, instfunc_lload_1, instfunc_lload_2,
        instfunc_lload_3, instfunc_fload_0, instfunc_fload_1,
        instfunc_fload_2, instfunc_fload_3, instfunc_dload_0,
        instfunc_dload_1, instfunc_dload_2, instfunc_dload_3,
        instfunc_aload_0, instfunc_aload_1, instfunc_aload_2,
        instfunc_aload_3, instfunc_iaload, instfunc_laload,
        instfunc_faload, instfunc_daload, instfunc_aaload,
        instfunc_baload, instfunc_caload, instfunc_saload,
        instfunc_istore, instfunc_lstore, instfunc_fstore,
        instfunc_dstore, instfunc_astore, instfunc_istore_0,
        instfunc_istore_1, instfunc_istore_2, instfunc_istore_3,
        instfunc_lstore_0, instfunc_lstore_1, instfunc_lstore_2,
        instfunc_lstore_3, instfunc_fstore_0, instfunc_fstore_1,
        instfunc_fstore_2, instfunc_fstore_3, instfunc_dstore_0,
        instfunc_dstore_1, instfunc_dstore_2, instfunc_dstore_3,
        instfunc_astore_0, instfunc_astore_1, instfunc_astore_2,
        instfunc_astore_3, instfunc_iastore, instfunc_lastore,
        instfunc_fastore, instfunc_dastore, instfunc_aastore,
        instfunc_bastore, instfunc_castore, instfunc_sastore,
        instfunc_pop, instfunc_pop2, instfunc_dup,
        instfunc_dup_x1, instfunc_dup_x2, instfunc_dup2

    };

    if (opcode > 92)
        return NULL;

    // TODO: fill instructions that aren't currently implemented
    // with NULL so the code will properly work

    return opcodeFunctions[opcode];
}
