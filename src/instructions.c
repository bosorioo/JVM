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
        instfunc_aload_3
    };

    if (opcode > 45)
        return NULL;

    // TODO: fill instructions that aren't currently implemented
    // with NULL so the code will properly work

    return opcodeFunctions[opcode];
}
