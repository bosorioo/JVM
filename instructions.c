#include "instructions.h"

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

#define DECLR_ICONST(name, value) \
    uint8_t instfunc_iconst_##name(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, value, OP_INTEGER)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_ICONST(m1, -1)
DECLR_ICONST(0, 0)
DECLR_ICONST(1, 1)
DECLR_ICONST(2, 2)
DECLR_ICONST(3, 3)
DECLR_ICONST(4, 4)
DECLR_ICONST(5, 5)

#define DECLR_LCONST(value) \
    uint8_t instfunc_lconst_##value(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, 0, OP_LONG)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        if (!pushOperand(&frame->operands, value, OP_LONG)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_LCONST(0)
DECLR_LCONST(1)

#define DECLR_FCONST(name, value) \
    uint8_t instfunc_fconst_##name(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, value, OP_FLOAT)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_FCONST(0, 0x0)
DECLR_FCONST(1, 0x3F800000)
DECLR_FCONST(2, 0x40000000)

#define DECLR_DCONST(name, high, low) \
    uint8_t instfunc_dconst_##name(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, high, OP_DOUBLE)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        if (!pushOperand(&frame->operands, low, OP_DOUBLE)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_DCONST(0, 0x0, 0x0)
DECLR_DCONST(1, 0x3FF00000, 0x0)

uint8_t instfunc_bipush(JavaVirtualMachine* jvm, Frame* frame)
{
    int8_t immediate = (int8_t)(*(frame->code + frame->pc));
    frame->pc++;

    if (!pushOperand(&frame->operands, immediate, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_sipush(JavaVirtualMachine* jvm, Frame* frame)
{
    int16_t immediate = (int16_t)*(frame->code + frame->pc);
    immediate <<= 8;
    immediate |= *(frame->code + frame->pc + 1);
    frame->pc += 2;

    if (!pushOperand(&frame->operands, immediate, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value = (int32_t)(*(frame->code + frame->pc));
    frame->pc++;

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
            // TODO: Resolve class (load it)
            type = OP_CLASSREF;
            break;

        case CONSTANT_Methodref:
            // TODO: Resolve method
            type = OP_METHODREF;
            break;

        default:
            // Shouldn't happen
            // OP_NULL is set so the compiler won't complain
            // about "type" being uninitialized
            type = OP_NULL;
            break;
    }

    if (!pushOperand(&frame->operands, value, type))
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
        instfunc_ldc,
    };

    // TODO: fill instructions that aren't currently implemented
    // with NULL so the code will properly work

    return opcodeFunctions[opcode];
}
