#include "instructions.h"
#include "utf8.h"
#include "jvm.h"
#include "natives.h"
#include <math.h>

// TODO: replace all 'out of memory' status errors with
// exception OutOfMemory.

#define NEXT_BYTE (*(frame->code + frame->pc++))
#define HIWORD(x) ((int32_t)(x >> 32))
#define LOWORD(x) ((int32_t)(x & 0xFFFFFFFFll))

uint8_t instfunc_nop(JavaVirtualMachine* jvm, Frame* frame)
{
    return 1;
}

uint8_t instfunc_aconst_null(JavaVirtualMachine* jvm, Frame* frame)
{
    if (!pushOperand(&frame->operands, 0, OP_REFERENCE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

/// @brief Used to automatically generate instructions "iconst_<n>" and
/// fconst_<n>.
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

/// @brief Used to automatically generate instructions "lconst_<n>" and
/// dconst_<n>.
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
        {
            cpi = frame->jc->constantPool + cpi->String.string_index - 1;

            Reference* str = newString(jvm, cpi->Utf8.bytes, cpi->Utf8.length);

            if (!str)
            {
                jvm->status = JVM_STATUS_OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)str;
            type = OP_REFERENCE;
            break;
        }

        case CONSTANT_Class:
        {
            cpi = frame->jc->constantPool + cpi->Class.name_index - 1;

            LoadedClasses* loadedClass;

            if (!resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length, &loadedClass))
            {
                // TODO: throw a resolution exception
                // Could be LinkageError, NoClassDefFoundError or IllegalAccessError
                return 0;
            }

            Reference* obj = newClassInstance(jvm, loadedClass);

            if (!obj)
            {
                jvm->status = JVM_STATUS_OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)obj;
            type = OP_REFERENCE;
            break;
        }

        case CONSTANT_Methodref:

            // This isn't supported, because it requires an instance of
            // MethodType or MethodHandle, which are Java 8 features.
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;

        default:
            DEBUG_REPORT_INSTRUCTION_ERROR
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
        {
            cpi = frame->jc->constantPool + cpi->String.string_index - 1;

            Reference* str = newString(jvm, cpi->Utf8.bytes, cpi->Utf8.length);

            if (!str)
            {
                jvm->status = JVM_STATUS_OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)str;
            type = OP_REFERENCE;
            break;
        }

        case CONSTANT_Class:
        {
            cpi = frame->jc->constantPool + cpi->Class.name_index - 1;

            LoadedClasses* loadedClass;

            if (!resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length, &loadedClass))
            {
                // TODO: throw a resolution exception
                // Could be LinkageError, NoClassDefFoundError or IllegalAccessError
                return 0;
            }

            Reference* obj = newClassInstance(jvm, loadedClass);

            if (!obj)
            {
                jvm->status = JVM_STATUS_OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)obj;
            type = OP_REFERENCE;
            break;
        }

        case CONSTANT_Methodref:

            // This isn't supported, because it requires an instance of
            // MethodType or MethodHandle, which are Java 8 features.
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;

        default:
            DEBUG_REPORT_INSTRUCTION_ERROR
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

/// @brief Used to automatically generate instructions "iload",
/// "fload" and "aload".
#define DECLR_LOAD_CAT_1_FAMILY(instructionprefix, type) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        if (!pushOperand(&frame->operands, *(frame->localVariables + NEXT_BYTE), type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

/// @brief Used to automatically generate instructions "lload",
/// and "dload".
#define DECLR_LOAD_CAT_2_FAMILY(instructionprefix, type) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint8_t index = NEXT_BYTE; \
        if (!pushOperand(&frame->operands, *(frame->localVariables + index), type) || \
            !pushOperand(&frame->operands, *(frame->localVariables + index + 1), type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

/// @brief Used to automatically generate instructions "iload",
/// "fload" and "aload" that are widened.
#define DECLR_WIDE_LOAD_CAT_1_FAMILY(instructionprefix, type) \
    uint8_t instfunc_wide_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        if (!pushOperand(&frame->operands, *(frame->localVariables + index), type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

/// @brief Used to automatically generate instructions "lload",
/// and "dload" that are widened.
#define DECLR_WIDE_LOAD_CAT_2_FAMILY(instructionprefix, type) \
    uint8_t instfunc_wide_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        if (!pushOperand(&frame->operands, *(frame->localVariables + index), type) || \
            !pushOperand(&frame->operands, *(frame->localVariables + index + 1), type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_LOAD_CAT_1_FAMILY(iload, OP_INTEGER)
DECLR_LOAD_CAT_2_FAMILY(lload, OP_LONG)
DECLR_LOAD_CAT_1_FAMILY(fload, OP_FLOAT)
DECLR_LOAD_CAT_2_FAMILY(dload, OP_DOUBLE)
DECLR_LOAD_CAT_1_FAMILY(aload, OP_REFERENCE)

DECLR_WIDE_LOAD_CAT_1_FAMILY(iload, OP_INTEGER)
DECLR_WIDE_LOAD_CAT_2_FAMILY(lload, OP_LONG)
DECLR_WIDE_LOAD_CAT_1_FAMILY(fload, OP_FLOAT)
DECLR_WIDE_LOAD_CAT_2_FAMILY(dload, OP_DOUBLE)
DECLR_WIDE_LOAD_CAT_1_FAMILY(aload, OP_REFERENCE)

/// @brief Used to automatically generate instructions "iload_<n>",
/// "fload_<n>" and "aload_<n>".
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

/// @brief Used to automatically generate instructions "dload_<n>"
/// and "lload_<n>".
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

DECLR_CAT_1_LOAD_N_FAMILY(aload, 0, OP_REFERENCE)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 1, OP_REFERENCE)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 2, OP_REFERENCE)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 3, OP_REFERENCE)

/// @brief Used to automatically generate instructions "iaload", "faload",
/// "baload", "saload" and "caload".
#define DECLR_ALOAD_CAT_1_FAMILY(instructionname, type, op_type) \
    uint8_t instfunc_##instructionname(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t index; \
        int32_t arrayref; \
        Reference* obj; \
        popOperand(&frame->operands, &index, NULL); \
        popOperand(&frame->operands, &arrayref, NULL); \
        obj = (Reference*)arrayref; \
        if (obj == NULL) \
        { \
            /* TODO: throw NullPointerException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            /* TODO: throw ArrayIndexOutOfBoundsException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        type* ptr = (type*)obj->arr.data; \
        if (!pushOperand(&frame->operands, ptr[index], op_type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

/// @brief Used to automatically generate instructions "laload" and
/// "daload".
#define DECLR_ALOAD_CAT_2_FAMILY(instructionname, type, op_type) \
    uint8_t instfunc_##instructionname(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t index; \
        int32_t arrayref; \
        Reference* obj; \
        popOperand(&frame->operands, &index, NULL); \
        popOperand(&frame->operands, &arrayref, NULL); \
        obj = (Reference*)arrayref; \
        if (obj == NULL) \
        { \
            /* TODO: throw NullPointerException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            /* TODO: throw ArrayIndexOutOfBoundsException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        type* ptr = (type*)obj->arr.data; \
        if (!pushOperand(&frame->operands, HIWORD(ptr[index]), op_type) || \
            !pushOperand(&frame->operands, LOWORD(ptr[index]), op_type)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_ALOAD_CAT_1_FAMILY(iaload, int32_t, OP_INTEGER)
DECLR_ALOAD_CAT_2_FAMILY(laload, int64_t, OP_INTEGER)
DECLR_ALOAD_CAT_1_FAMILY(faload, int32_t, OP_FLOAT)
DECLR_ALOAD_CAT_2_FAMILY(daload, int64_t, OP_INTEGER)
DECLR_ALOAD_CAT_1_FAMILY(baload, int8_t, OP_INTEGER)
DECLR_ALOAD_CAT_1_FAMILY(saload, int16_t, OP_INTEGER)
DECLR_ALOAD_CAT_1_FAMILY(caload, int16_t, OP_INTEGER)

uint8_t instfunc_aaload(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t index;
    int32_t arrayref;
    Reference* obj;

    popOperand(&frame->operands, &index, NULL);
    popOperand(&frame->operands, &arrayref, NULL);

    obj = (Reference*)arrayref;

    if (obj == NULL)
    {
        /* TODO: throw NullPointerException*/
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    if (index < 0 || (uint32_t)index >= obj->oar.length)
    {
        /* TODO: throw ArrayIndexOutOfBoundsException*/
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    Reference** ptr = (Reference**)obj->oar.elements;

    if (!pushOperand(&frame->operands, (int32_t)ptr[index], OP_REFERENCE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

/// @brief Used to automatically generate instructions "istore", "fstore"
/// and "astore".
#define DECLR_STORE_CAT_1_FAMILY(instructionprefix) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t operand; \
        popOperand(&frame->operands, &operand, NULL); \
        *(frame->localVariables + NEXT_BYTE) = operand; \
        return 1; \
    }

/// @brief Used to automatically generate instructions "istore", "fstore"
/// and "astore" that are widened.
#define DECLR_WIDE_STORE_CAT_1_FAMILY(instructionprefix) \
    uint8_t instfunc_wide_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        int32_t operand; \
        popOperand(&frame->operands, &operand, NULL); \
        *(frame->localVariables + NEXT_BYTE) = operand; \
        return 1; \
    }

/// @brief Used to automatically generate instructions "l" and
/// "dstore".
#define DECLR_STORE_CAT_2_FAMILY(instructionprefix) \
    uint8_t instfunc_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint8_t index = NEXT_BYTE; \
        int32_t highoperand; \
        int32_t lowoperand; \
        popOperand(&frame->operands, &lowoperand, NULL); \
        popOperand(&frame->operands, &highoperand, NULL); \
        *(frame->localVariables + index) = highoperand; \
        *(frame->localVariables + index + 1) = lowoperand; \
        return 1; \
    }

/// @brief Used to automatically generate instructions "l" and
/// "dstore" that are widened.
#define DECLR_WIDE_STORE_CAT_2_FAMILY(instructionprefix) \
    uint8_t instfunc_wide_##instructionprefix(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        int32_t highoperand; \
        int32_t lowoperand; \
        popOperand(&frame->operands, &lowoperand, NULL); \
        popOperand(&frame->operands, &highoperand, NULL); \
        *(frame->localVariables + index) = highoperand; \
        *(frame->localVariables + index + 1) = lowoperand; \
        return 1; \
    }

DECLR_STORE_CAT_1_FAMILY(istore)
DECLR_STORE_CAT_2_FAMILY(lstore)
DECLR_STORE_CAT_1_FAMILY(fstore)
DECLR_STORE_CAT_2_FAMILY(dstore)
DECLR_STORE_CAT_1_FAMILY(astore)

DECLR_WIDE_STORE_CAT_1_FAMILY(istore)
DECLR_WIDE_STORE_CAT_2_FAMILY(lstore)
DECLR_WIDE_STORE_CAT_1_FAMILY(fstore)
DECLR_WIDE_STORE_CAT_2_FAMILY(dstore)
DECLR_WIDE_STORE_CAT_1_FAMILY(astore)

/// @brief Used to automatically generate instructions "istore_<n>", "fstore_<n>"
/// and "astore_<n>".
#define DECLR_STORE_N_CAT_1_FAMILY(instructionprefix, N) \
    uint8_t instfunc_##instructionprefix##_##N(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t operand; \
        popOperand(&frame->operands, &operand, NULL); \
        *(frame->localVariables + N) = operand; \
        return 1; \
    }

/// @brief Used to automatically generate instructions "lstore_<n>" and
/// "dstore_<n>".
#define DECLR_STORE_N_CAT_2_FAMILY(instructionprefix, N) \
    uint8_t instfunc_##instructionprefix##_##N(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t highoperand; \
        int32_t lowoperand; \
        popOperand(&frame->operands, &lowoperand, NULL); \
        popOperand(&frame->operands, &highoperand, NULL); \
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

/// @brief Used to automatically generate instructions "bastore", "castore",
/// "sastore", "iastore" and "fastore".
#define DECLR_ASTORE_CAT_1_FAMILY(instructionname, type) \
    uint8_t instfunc_##instructionname(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t operand; \
        int32_t index; \
        int32_t arrayref; \
        Reference* obj; \
        popOperand(&frame->operands, &operand, NULL); \
        popOperand(&frame->operands, &index, NULL); \
        popOperand(&frame->operands, &arrayref, NULL); \
        obj = (Reference*)arrayref; \
        if (obj == NULL) \
        { \
            /* TODO: throw NullPointerException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            /* TODO: throw ArrayIndexOutOfBoundsException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        type* ptr = (type*)obj->arr.data; \
        ptr[index] = (type)operand; \
        return 1; \
    }

DECLR_ASTORE_CAT_1_FAMILY(bastore, int8_t)
DECLR_ASTORE_CAT_1_FAMILY(sastore, int16_t)
DECLR_ASTORE_CAT_1_FAMILY(castore, int16_t)
DECLR_ASTORE_CAT_1_FAMILY(iastore, int32_t)
DECLR_ASTORE_CAT_1_FAMILY(fastore, int32_t)

/// @brief Used to automatically generate instructions "dastore" and
/// "lastore".
#define DECLR_ASTORE_CAT_2_FAMILY(instructionname) \
    uint8_t instfunc_##instructionname(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t highoperand; \
        int32_t lowoperand; \
        int32_t index; \
        int32_t arrayref; \
        Reference* obj; \
        popOperand(&frame->operands, &lowoperand, NULL); \
        popOperand(&frame->operands, &highoperand, NULL); \
        popOperand(&frame->operands, &index, NULL); \
        popOperand(&frame->operands, &arrayref, NULL); \
        obj = (Reference*)arrayref; \
        if (obj == NULL) \
        { \
            /* TODO: throw NullPointerException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            /* TODO: throw ArrayIndexOutOfBoundsException*/ \
            DEBUG_REPORT_INSTRUCTION_ERROR \
            return 0; \
        } \
        int64_t* ptr = (int64_t*)obj->arr.data; \
        ptr[index] = ((int64_t)highoperand << 32) | (uint32_t)lowoperand; \
        return 1; \
    }

DECLR_ASTORE_CAT_2_FAMILY(dastore)
DECLR_ASTORE_CAT_2_FAMILY(lastore)

uint8_t instfunc_aastore(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t operand;
    int32_t index;
    int32_t arrayref;

    Reference* arrayobj;
    Reference* element;

    popOperand(&frame->operands, &operand, NULL);
    popOperand(&frame->operands, &index, NULL);
    popOperand(&frame->operands, &arrayref, NULL);

    arrayobj = (Reference*)arrayref;
    element = (Reference*)operand;

    if (arrayobj == NULL)
    {
        // TODO: throw NullPointerException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0; \
    }

    if (index < 0 || (uint32_t)index >= arrayobj->oar.length)
    {
        // TODO: throw ArrayIndexOutOfBoundsException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // TODO: throw ArrayStoreException in case of incompatible
    // element/array type.

    arrayobj->oar.elements[index] = element;
    return 1;
}

uint8_t instfunc_pop(JavaVirtualMachine* jvm, Frame* frame)
{
    popOperand(&frame->operands, NULL, NULL);
    return 1;
}

uint8_t instfunc_pop2(JavaVirtualMachine* jvm, Frame* frame)
{
    popOperand(&frame->operands, NULL, NULL);
    popOperand(&frame->operands, NULL, NULL);
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
    // The stack should look like: A -> B -> C -> A -> ...


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

uint8_t instfunc_dup2_x1(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t operand1, operand2, operand3;
    OperandType type1, type2, type3;

    // Pop the operand and then push them again.
    // This method is easier to implement, but it is
    // less efficient, since there is the need to free
    // and malloc memory again.
    popOperand(&frame->operands, &operand1, &type1);
    popOperand(&frame->operands, &operand2, &type2);
    popOperand(&frame->operands, &operand3, &type3);

    if (!pushOperand(&frame->operands, operand2, type2) ||
        !pushOperand(&frame->operands, operand1, type1) ||
        !pushOperand(&frame->operands, operand3, type3) ||
        !pushOperand(&frame->operands, operand2, type2) ||
        !pushOperand(&frame->operands, operand1, type1))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dup2_x2(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t operand1, operand2, operand3, operand4;
    OperandType type1, type2, type3, type4;

    // Pop the operand and then push them again.
    // This method is easier to implement, but it is
    // less efficient, since there is the need to free
    // and malloc memory again.
    popOperand(&frame->operands, &operand1, &type1);
    popOperand(&frame->operands, &operand2, &type2);
    popOperand(&frame->operands, &operand3, &type3);
    popOperand(&frame->operands, &operand4, &type4);

    if (!pushOperand(&frame->operands, operand2, type2) ||
        !pushOperand(&frame->operands, operand1, type1) ||
        !pushOperand(&frame->operands, operand4, type4) ||
        !pushOperand(&frame->operands, operand3, type3) ||
        !pushOperand(&frame->operands, operand2, type2) ||
        !pushOperand(&frame->operands, operand1, type1))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_swap(JavaVirtualMachine* jvm, Frame* frame)
{
    OperandStack* node1 = frame->operands;
    OperandStack* node2 = node1->next;

    frame->operands = node2;
    node1->next = node2->next;
    node2->next = node1;
    return 1;
}

/// @brief Used to automatically generate instructions "iadd", "isub",
/// "imul", "idiv", "irem", "iand", "ior" and "ixor".
#define DECLR_INTEGER_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t value1, value2; \
        popOperand(&frame->operands, &value2, NULL); \
        popOperand(&frame->operands, &value1, NULL); \
        if (!pushOperand(&frame->operands, value1 op value2, OP_INTEGER)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_INTEGER_MATH_OP(iadd, +)
DECLR_INTEGER_MATH_OP(isub, -)
DECLR_INTEGER_MATH_OP(imul, *)
DECLR_INTEGER_MATH_OP(idiv, /)
DECLR_INTEGER_MATH_OP(irem, %)
DECLR_INTEGER_MATH_OP(iand, &)
DECLR_INTEGER_MATH_OP(ior, |)
DECLR_INTEGER_MATH_OP(ixor, ^)

uint8_t instfunc_ishl(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value1, value2;

    popOperand(&frame->operands, &value2, NULL);
    popOperand(&frame->operands, &value1, NULL);

    if (!pushOperand(&frame->operands, value1 << (value2 & 0x1F), OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ishr(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value1, value2;

    popOperand(&frame->operands, &value2, NULL);
    popOperand(&frame->operands, &value1, NULL);

    if (!pushOperand(&frame->operands, value1 >> (value2 & 0x1F), OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_iushr(JavaVirtualMachine* jvm, Frame* frame)
{
    uint32_t value1, value2;

    popOperand(&frame->operands, (int32_t*)&value2, NULL);
    popOperand(&frame->operands, (int32_t*)&value1, NULL);

    if (!pushOperand(&frame->operands, value1 >> (value2 & 0x1F), OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

/// @brief Used to automatically generate instructions "ladd", "lsub",
/// "lmul", "ldiv", "lrem", "land", "lor" and "lxor".
#define DECLR_LONG_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int64_t value1, value2; \
        int32_t high, low; \
        popOperand(&frame->operands, &low, NULL); \
        popOperand(&frame->operands, &high, NULL); \
        value2 = high; \
        value2 = value2 << 32 | (uint32_t)low; \
        popOperand(&frame->operands, &low, NULL); \
        popOperand(&frame->operands, &high, NULL); \
        value1 = high; \
        value1 = value1 << 32 | (uint32_t)low; \
        value1 = value1 op value2; \
        if (!pushOperand(&frame->operands, HIWORD(value1), OP_LONG) || \
            !pushOperand(&frame->operands, LOWORD(value1), OP_LONG)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_LONG_MATH_OP(ladd, +)
DECLR_LONG_MATH_OP(lsub, -)
DECLR_LONG_MATH_OP(lmul, *)
DECLR_LONG_MATH_OP(ldiv, /)
DECLR_LONG_MATH_OP(lrem, %)
DECLR_LONG_MATH_OP(land, &)
DECLR_LONG_MATH_OP(lor, |)
DECLR_LONG_MATH_OP(lxor, ^)

uint8_t instfunc_lshl(JavaVirtualMachine* jvm, Frame* frame)
{
    int64_t value1;
    int32_t value2;
    int32_t high, low;

    popOperand(&frame->operands, &value2, NULL);
    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    value1 = value1 << (value2 & 0x3F);

    if (!pushOperand(&frame->operands, HIWORD(value1), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(value1), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

uint8_t instfunc_lshr(JavaVirtualMachine* jvm, Frame* frame)
{
    int64_t value1;
    int32_t value2;
    int32_t high, low;

    popOperand(&frame->operands, &value2, NULL);
    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    value1 = value1 >> (value2 & 0x3F);

    if (!pushOperand(&frame->operands, HIWORD(value1), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(value1), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

uint8_t instfunc_lushr(JavaVirtualMachine* jvm, Frame* frame)
{
    uint64_t value1;
    uint32_t value2;
    uint32_t high, low;

    popOperand(&frame->operands, (int32_t*)&value2, NULL);
    popOperand(&frame->operands, (int32_t*)&low, NULL);
    popOperand(&frame->operands, (int32_t*)&high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    value1 = value1 >> (value2 & 0x3F);

    if (!pushOperand(&frame->operands, HIWORD(value1), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(value1), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

/// @brief Used to automatically generate instructions "fadd", "fsub",
/// "fmul" and "fdiv".
#define DECLR_FLOAT_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        union { \
            float f; \
            int32_t i; \
        } value1, value2; \
        popOperand(&frame->operands, &value2.i, NULL); \
        popOperand(&frame->operands, &value1.i, NULL); \
        value1.f = value1.f op value2.f; \
        if (!pushOperand(&frame->operands, value1.i, OP_FLOAT)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_FLOAT_MATH_OP(fadd, +)
DECLR_FLOAT_MATH_OP(fsub, -)
DECLR_FLOAT_MATH_OP(fmul, *)
DECLR_FLOAT_MATH_OP(fdiv, /)

/// @brief Used to automatically generate instructions "dadd", "dsub",
/// "dmul" and "ddiv".
#define DECLR_DOUBLE_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        union { \
            double d; \
            int64_t i; \
        } value1, value2; \
        int32_t high, low; \
        popOperand(&frame->operands, &low, NULL); \
        popOperand(&frame->operands, &high, NULL); \
        value2.i = high; \
        value2.i = (value2.i << 32) | (uint32_t)low; \
        popOperand(&frame->operands, &low, NULL); \
        popOperand(&frame->operands, &high, NULL); \
        value1.i = high; \
        value1.i = (value1.i << 32) | (uint32_t)low; \
        value1.d = value1.d op value2.d; \
        if (!pushOperand(&frame->operands, HIWORD(value1.i), OP_DOUBLE) || \
            !pushOperand(&frame->operands, LOWORD(value1.i), OP_DOUBLE)) \
        { \
            jvm->status = JVM_STATUS_OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_DOUBLE_MATH_OP(dadd, +)
DECLR_DOUBLE_MATH_OP(dsub, -)
DECLR_DOUBLE_MATH_OP(dmul, *)
DECLR_DOUBLE_MATH_OP(ddiv, /)

uint8_t instfunc_frem(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        float f;
        int32_t i;
    } value1, value2;

    popOperand(&frame->operands, &value2.i, NULL);
    popOperand(&frame->operands, &value1.i, NULL);

    // When the dividend is finite and the divisor is infinity, the result should
    // be equal to the dividend. So we do nothing to 'a'.
    // Otherwise, we calculate the fmod. We have to make this check because
    // fmod fails when the divisor is +-infinity, returning NAN.
    if (!(value1.f != INFINITY && value1.f != -INFINITY && (value2.f == INFINITY || value2.f == -INFINITY)))
        value1.f = fmodf(value1.f, value2.f);

    if (!pushOperand(&frame->operands, value1.i, OP_FLOAT))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_drem(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } value1, value2;

    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value2.i = high;
    value2.i = (value2.i << 32) | (uint32_t)low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value1.i = high;
    value1.i = (value1.i << 32) | (uint32_t)low;

    // When the dividend is finite and the divisor is infinity, the result should
    // be equal to the dividend. So we do nothing to 'a'.
    // Otherwise, we calculate the fmod. We have to make this check because
    // fmod fails when the divisor is +-infinity, returning NAN.
    if (!(value1.d != INFINITY && value1.d != -INFINITY && (value2.d == INFINITY || value2.d == -INFINITY)))
        value1.d = fmod(value1.d, value2.d);

    if (!pushOperand(&frame->operands, HIWORD(value1.i), OP_DOUBLE) ||
        !pushOperand(&frame->operands, LOWORD(value1.i), OP_DOUBLE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ineg(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value;
    popOperand(&frame->operands, &value, NULL);
    value = -value;

    if (!pushOperand(&frame->operands, value, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_lneg(JavaVirtualMachine* jvm, Frame* frame)
{
    int64_t value;
    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value = high;
    value = (value << 32) | (uint32_t)low;
    value = -value;

    if (!pushOperand(&frame->operands, HIWORD(value), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(value), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fneg(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        float f;
        int32_t i;
    } value;

    popOperand(&frame->operands, &value.i, NULL);

    value.f = -value.f;

    if (!pushOperand(&frame->operands, value.i, OP_FLOAT))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dneg(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } value;

    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value.i = high;
    value.i = (value.i << 32) | (uint32_t)low;
    value.d = -value.d;

    if (!pushOperand(&frame->operands, HIWORD(value.i), OP_DOUBLE) ||
        !pushOperand(&frame->operands, LOWORD(value.i), OP_DOUBLE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_iinc(JavaVirtualMachine* jvm, Frame* frame)
{
    uint8_t index = NEXT_BYTE;
    int8_t immediate = (int8_t)NEXT_BYTE;
    frame->localVariables[index] += (int32_t)immediate;
    return 1;
}

uint8_t instfunc_wide_iinc(JavaVirtualMachine* jvm, Frame* frame)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;
    int16_t immediate = NEXT_BYTE;
    immediate = (immediate << 8) | NEXT_BYTE;
    frame->localVariables[index] += (int32_t)immediate;
    return 1;
}

uint8_t instfunc_i2l(JavaVirtualMachine* jvm, Frame* frame)
{
    int64_t value;
    int32_t temp;

    popOperand(&frame->operands, &temp, NULL);

    value = temp;

    if (!pushOperand(&frame->operands, HIWORD(value), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(value), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2f(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        float f;
        int32_t i;
    } value;

    popOperand(&frame->operands, &value.i, NULL);
    value.f = (float)value.i;

    if (!pushOperand(&frame->operands, value.i, OP_FLOAT))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2d(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } value;

    int32_t temp;

    popOperand(&frame->operands, &temp, NULL);
    value.d = (double)temp;

    if (!pushOperand(&frame->operands, HIWORD(value.i), OP_DOUBLE) ||
        !pushOperand(&frame->operands, LOWORD(value.i), OP_DOUBLE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_l2i(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t temp;

    popOperand(&frame->operands, &temp, NULL);
    popOperand(&frame->operands, NULL, NULL);

    if (!pushOperand(&frame->operands, temp, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_l2f(JavaVirtualMachine* jvm, Frame* frame)
{
    int64_t lval;

    union {
        float f;
        int32_t i;
    } temp;

    int32_t low, high;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    lval = high;
    lval = (lval << 32) | (uint32_t)low;
    temp.f = (float)lval;

    if (!pushOperand(&frame->operands, temp.i, OP_FLOAT))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_l2d(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } val;

    int32_t temp;

    popOperand(&frame->operands, &temp, NULL);

    val.i = temp;

    popOperand(&frame->operands, &temp, NULL);

    val.i = (val.i << 32) | (uint32_t)temp;
    val.d = (double)val.i;

    if (!pushOperand(&frame->operands, HIWORD(val.i), OP_DOUBLE) ||
        !pushOperand(&frame->operands, LOWORD(val.i), OP_DOUBLE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_f2i(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        float f;
        int32_t i;
    } value;

    popOperand(&frame->operands, &value.i, NULL);
    value.i = (int32_t)value.f;

    if (!pushOperand(&frame->operands, value.i, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_f2l(JavaVirtualMachine* jvm, Frame* frame)
{
    int64_t lval;

    union {
        float f;
        int32_t i;
    } temp;

    popOperand(&frame->operands, &temp.i, NULL);

    lval = (int64_t)temp.f;

    if (!pushOperand(&frame->operands, HIWORD(lval), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(lval), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_f2d(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } dval;

    union {
        float f;
        int32_t i;
    } temp;

    popOperand(&frame->operands, &temp.i, NULL);

    dval.d = (double)temp.f;

    if (!pushOperand(&frame->operands, HIWORD(dval.i), OP_DOUBLE) ||
        !pushOperand(&frame->operands, LOWORD(dval.i), OP_DOUBLE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_d2i(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } dval;

    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    dval.i = high;
    dval.i = (dval.i << 32) | (uint32_t)low;

    low = (int32_t)dval.d;

    if (!pushOperand(&frame->operands, low, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_d2l(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } dval;

    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    dval.i = high;
    dval.i = (dval.i << 32) | (uint32_t)low;
    dval.i = (int64_t)dval.d;

    if (!pushOperand(&frame->operands, HIWORD(dval.i), OP_LONG) ||
        !pushOperand(&frame->operands, LOWORD(dval.i), OP_LONG))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_d2f(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        double d;
        int64_t i;
    } dval;

    union {
        float f;
        int32_t i;
    } temp;

    int32_t low, high;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    dval.i = high;
    dval.i = (dval.i << 32) | (uint32_t)low;

    temp.f = (float)dval.d;

    if (!pushOperand(&frame->operands, temp.i, OP_FLOAT))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2b(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value;
    int8_t byte;

    popOperand(&frame->operands, &value, NULL);

    byte = (int8_t)value;

    if (!pushOperand(&frame->operands, (int32_t)byte, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2c(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value;
    uint16_t character;

    popOperand(&frame->operands, &value, NULL);

    character = (uint16_t)value;

    if (!pushOperand(&frame->operands, (int32_t)character, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2s(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t value;
    int16_t sval;

    popOperand(&frame->operands, &value, NULL);

    sval = (int16_t)value;

    if (!pushOperand(&frame->operands, (int32_t)sval, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_lcmp(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t high, low;
    int64_t value1, value2;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value2 = high;
    value2 = (value2 << 32) | (uint32_t)low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    if (value1 > value2)
        high = 1;
    else if (value1 == value2)
        high = 0;
    else
        high = -1;

    if (!pushOperand(&frame->operands, high, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fcmpl(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        int32_t i;
        float f;
    } value1, value2;

    popOperand(&frame->operands, &value2.i, NULL);
    popOperand(&frame->operands, &value1.i, NULL);

    if (value1.f < value2.f || value1.f == NAN || value2.f == NAN)
        value1.i = -1;
    else if (value1.f == value2.f)
        value1.i = 0;
    else
        value1.i = 1;

    if (!pushOperand(&frame->operands, value1.i, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fcmpg(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        int32_t i;
        float f;
    } value1, value2;

    popOperand(&frame->operands, &value2.i, NULL);
    popOperand(&frame->operands, &value1.i, NULL);

    if (value1.f > value2.f || value1.f == NAN || value2.f == NAN)
        value1.i = 1;
    else if (value1.f == value2.f)
        value1.i = 0;
    else
        value1.i = -1;

    if (!pushOperand(&frame->operands, value1.i, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dcmpl(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        int64_t i;
        double d;
    } value1, value2;

    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value2.i = high;
    value2.i = (value2.i << 32) | (uint32_t)low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value1.i = high;
    value1.i = (value1.i << 32) | (uint32_t)low;

    if (value1.d < value2.d || value1.d == NAN || value2.d == NAN)
        value1.i = -1;
    else if (value1.d == value2.d)
        value1.i = 0;
    else
        value1.i = 1;

    if (!pushOperand(&frame->operands, value1.i, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dcmpg(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        int64_t i;
        double d;
    } value1, value2;

    int32_t high, low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value2.i = high;
    value2.i = (value2.i << 32) | (uint32_t)low;

    popOperand(&frame->operands, &low, NULL);
    popOperand(&frame->operands, &high, NULL);

    value1.i = high;
    value1.i = (value1.i << 32) | (uint32_t)low;

    if (value1.d > value2.d || value1.d == NAN || value2.d == NAN)
        value1.i = 1;
    else if (value1.d == value2.d)
        value1.i = 0;
    else
        value1.i = -1;

    if (!pushOperand(&frame->operands, value1.i, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

/// @brief Used to automatically generate instructions "ifeq", "ifne",
/// "iflt", "ifle", "ifgt" and "ifge".
#define DECLR_IF_FAMILY(inst, op) \
    uint8_t instfunc_##inst(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t value; \
        int16_t offset = NEXT_BYTE; \
        offset = (offset << 8) | NEXT_BYTE; \
        popOperand(&frame->operands, &value, NULL); \
        if (value op 0) \
            frame->pc += offset - 3; \
        return 1; \
    }

DECLR_IF_FAMILY(ifeq, ==)
DECLR_IF_FAMILY(ifne, !=)
DECLR_IF_FAMILY(iflt, <)
DECLR_IF_FAMILY(ifle, <=)
DECLR_IF_FAMILY(ifgt, >)
DECLR_IF_FAMILY(ifge, >=)

/// @brief Used to automatically generate instructions "if_icmpeq", "if_icmpne",
/// "if_icmplt", "if_icmple", "if_icmpgt", "if_icmpge", "if_acmpeq" and "if_acmpne".
#define DECLR_IF_ICMP_FAMILY(inst, op) \
    uint8_t instfunc_##inst(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        int32_t value1, value2; \
        int16_t offset = NEXT_BYTE; \
        offset = (offset << 8) | NEXT_BYTE; \
        popOperand(&frame->operands, &value2, NULL); \
        popOperand(&frame->operands, &value1, NULL); \
        if (value1 op value2) \
            frame->pc += offset - 3; \
        return 1; \
    }

DECLR_IF_ICMP_FAMILY(if_icmpeq, ==)
DECLR_IF_ICMP_FAMILY(if_icmpne, !=)
DECLR_IF_ICMP_FAMILY(if_icmplt, <)
DECLR_IF_ICMP_FAMILY(if_icmple, <=)
DECLR_IF_ICMP_FAMILY(if_icmpgt, >)
DECLR_IF_ICMP_FAMILY(if_icmpge, >=)

DECLR_IF_ICMP_FAMILY(if_acmpeq, ==)
DECLR_IF_ICMP_FAMILY(if_acmpne, !=)

uint8_t instfunc_goto(JavaVirtualMachine* jvm, Frame* frame)
{
    int16_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    frame->pc += offset - 3;
    return 1;
}

uint8_t instfunc_jsr(JavaVirtualMachine* jvm, Frame* frame)
{
    int16_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;

    if (!pushOperand(&frame->operands, (int32_t)frame->pc, OP_RETURNADDRESS))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    frame->pc += offset - 3;
    return 1;
}

uint8_t instfunc_ret(JavaVirtualMachine* jvm, Frame* frame)
{
    uint8_t index = NEXT_BYTE;
    frame->pc = (uint32_t)frame->localVariables[index];
    return 1;
}

uint8_t instfunc_wide_ret(JavaVirtualMachine* jvm, Frame* frame)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;
    frame->pc = (uint32_t)frame->localVariables[index];
    return 1;
}

uint8_t instfunc_tableswitch(JavaVirtualMachine* jvm, Frame* frame)
{
    uint32_t base = frame->pc - 1;

    // Skip padding bytes
    frame->pc += 4 - (frame->pc % 4);

    int32_t defaultValue = NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;

    int32_t lowValue = NEXT_BYTE;
    lowValue = (lowValue << 8) | NEXT_BYTE;
    lowValue = (lowValue << 8) | NEXT_BYTE;
    lowValue = (lowValue << 8) | NEXT_BYTE;

    int32_t highValue = NEXT_BYTE;
    highValue = (highValue << 8) | NEXT_BYTE;
    highValue = (highValue << 8) | NEXT_BYTE;
    highValue = (highValue << 8) | NEXT_BYTE;

    int32_t index;
    popOperand(&frame->operands, &index, NULL);

    if (index >= lowValue && index <= highValue)
    {
        frame->pc += 4 * (index - lowValue);
        defaultValue = NEXT_BYTE;
        defaultValue = (defaultValue << 8) | NEXT_BYTE;
        defaultValue = (defaultValue << 8) | NEXT_BYTE;
        defaultValue = (defaultValue << 8) | NEXT_BYTE;
    }

    frame->pc = base + defaultValue;

    return 1;
}

uint8_t instfunc_lookupswitch(JavaVirtualMachine* jvm, Frame* frame)
{
    uint32_t base = frame->pc - 1;

    // Skip padding bytes
    frame->pc += 4 - (frame->pc % 4);

    int32_t defaultValue = NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;

    int32_t npairs = NEXT_BYTE;
    npairs = (npairs << 8) | NEXT_BYTE;
    npairs = (npairs << 8) | NEXT_BYTE;
    npairs = (npairs << 8) | NEXT_BYTE;

    int32_t key, match;
    popOperand(&frame->operands, &key, NULL);

    while (npairs-- > 0)
    {
        match = NEXT_BYTE;
        match = (match << 8) | NEXT_BYTE;
        match = (match << 8) | NEXT_BYTE;
        match = (match << 8) | NEXT_BYTE;

        if (key == match)
        {
            defaultValue = NEXT_BYTE;
            defaultValue = (defaultValue << 8) | NEXT_BYTE;
            defaultValue = (defaultValue << 8) | NEXT_BYTE;
            defaultValue = (defaultValue << 8) | NEXT_BYTE;
            break;
        }
        else
        {
            frame->pc += 4;
        }
    }

    frame->pc = base + defaultValue;

    return 1;
}

/// @brief Used to automatically generate instructions "ireturn", "lreturn",
/// "freturn", "dreturn", "areturn" and "return".
#define DECLR_RETURN_FAMILY(instname, retcount) \
    uint8_t instfunc_##instname(JavaVirtualMachine* jvm, Frame* frame) \
    { \
        /* This will finish the frame, forcing it to return */ \
        frame->pc = frame->code_length; \
        /*
         Set the returnCount to 1, so the caller frame will know \
         that one operand (the top one obviously) must be popped \
         from this soon-to-end frame and pushed again to the \
         caller frame.
        */ \
        frame->returnCount = retcount; \
        return 1;\
    }

DECLR_RETURN_FAMILY(ireturn, 1)
DECLR_RETURN_FAMILY(lreturn, 2)
DECLR_RETURN_FAMILY(freturn, 1)
DECLR_RETURN_FAMILY(dreturn, 2)
DECLR_RETURN_FAMILY(areturn, 1)
DECLR_RETURN_FAMILY(return, 0)

uint8_t instfunc_getstatic(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    // Get the Fieldref CP entry
    cp_info* field = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2;

    if (jvm->simulatingSystemAndStringClasses)
    {
        cpi1 = frame->jc->constantPool + field->Fieldref.class_index - 1;
        cpi1 = frame->jc->constantPool + cpi1->Class.name_index - 1;

        // All static fields from java/lang/System will be replaced
        // with null object in simulation mode.
        if (cmp_UTF8(cpi1->Utf8.bytes, cpi1->Utf8.length, (const uint8_t*)"java/lang/System", 16))
        {
            if (!pushOperand(&frame->operands, 0, OP_NULL))
            {
                jvm->status = JVM_STATUS_OUT_OF_MEMORY;
                return 0;
            }

            return 1;
        }
    }

    LoadedClasses* fieldLoadedClass;

    // Resolve the field, i.e, load the class that field belongs to
    if (!resolveField(jvm, frame->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initClass(jvm, fieldLoadedClass))
    {
        // TODO: throw NoSuchFieldError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the field and its descriptor
    cpi2 = frame->jc->constantPool + field->Fieldref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    // Find in the field's class the field_info that matches the name and the descriptor
    field_info* fi = getFieldMatching(fieldLoadedClass->jc, cpi1->Utf8.bytes, cpi1->Utf8.length,
                                      cpi2->Utf8.bytes, cpi2->Utf8.length, 0);

    if (!fi)
    {
        // TODO: throw NoSuchFieldError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    OperandType type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = OP_LONG; break;
        case 'D': type = OP_DOUBLE; break;
        case 'F': type = OP_FLOAT; break;

        case 'L':
        case '[':
            type = OP_REFERENCE;
            break;

        case 'B': // byte
        case 'C': // char
        case 'I': // int
        case 'S': // short
        case 'Z': // boolean
            type = OP_INTEGER;
            break;

        default:
            // todo: throw exception maybe?
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;
    }

    if (!pushOperand(&frame->operands, fieldLoadedClass->staticFieldsData[fi->offset], type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    // If the field is category 2, push the second part operand from the static data
    if (type == OP_LONG || type == OP_DOUBLE)
    {
        if (!pushOperand(&frame->operands, fieldLoadedClass->staticFieldsData[fi->offset + 1], type))
        {
            jvm->status = JVM_STATUS_OUT_OF_MEMORY;
            return 0;
        }
    }

    // TODO: check if the field isn't static and isn't in an interface,
    // throwing IncompatibleClassChangeError

    // TODO: check if this class can access the field, i.e:
    //   1) If the field is private, then throw IllegalAccessError.
    //   2) If the field is protected and this class isn't a subclass
    //      of the field's class, throw IllegalAccessError.

    return 1;
}

uint8_t instfunc_putstatic(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    // Get the Fieldref CP entry
    cp_info* field = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2;

    LoadedClasses* fieldLoadedClass;

    // Resolve the field, i.e, load the class that field belongs to
    if (!resolveField(jvm, frame->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initClass(jvm, fieldLoadedClass))
    {
        // TODO: throw NoSuchFieldError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the field and its descriptor
    cpi2 = frame->jc->constantPool + field->Fieldref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    // Find in the field's class the field_info that matches the name and the descriptor
    field_info* fi = getFieldMatching(fieldLoadedClass->jc, cpi1->Utf8.bytes, cpi1->Utf8.length,
                                      cpi2->Utf8.bytes, cpi2->Utf8.length, 0);

    if (!fi)
    {
        // TODO: throw NoSuchFieldError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    OperandType type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = OP_LONG; break;
        case 'D': type = OP_DOUBLE; break;
        case 'F': type = OP_FLOAT; break;

        case 'L':
        case '[':
            type = OP_REFERENCE;
            break;

        case 'B': // byte
        case 'C': // char
        case 'I': // int
        case 'S': // short
        case 'Z': // boolean
            type = OP_INTEGER;
            break;

        default:
            // todo: throw exception maybe?
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;
    }

    int32_t operand;

    popOperand(&frame->operands, &operand, NULL);

    // If the field is category 2, set the following index too
    if (type == OP_LONG || type == OP_DOUBLE)
    {
        fieldLoadedClass->staticFieldsData[fi->offset + 1] = operand;
        popOperand(&frame->operands, &operand, NULL);
        fieldLoadedClass->staticFieldsData[fi->offset] = operand;
    }
    else
    {
        fieldLoadedClass->staticFieldsData[fi->offset] = operand;
    }


    // TODO: check if the field isn't static and isn't in an interface,
    // throwing IncompatibleClassChangeError

    // TODO: check if this class can access the field, i.e:
    //   1) If the field is private, then throw IllegalAccessError.
    //   2) If the field is protected and this class isn't a subclass
    //      of the field's class, throw IllegalAccessError.

    // TODO: check if the field can be written, ie:
    //   1) If the field is final and this instruction isn't
    //      being executed in the method '<clinit>', then
    //      throw IllegalAccessError

    return 1;
}

uint8_t instfunc_getfield(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    // Get the Fieldref CP entry
    cp_info* field = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2;

    LoadedClasses* fieldLoadedClass;

    // Resolve the field, i.e, load the class that field belongs to and
    // the class of the that field, if its type is class.
    if (!resolveField(jvm, frame->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initClass(jvm, fieldLoadedClass))
    {
        // TODO: throw Error or other linkage exception
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the field and its descriptor
    cpi2 = frame->jc->constantPool + field->Fieldref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    field_info* fi = getFieldMatching(fieldLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!fi)
    {
        // maybe it is located in the super class
        JavaClass* super = getSuperClass(jvm, fieldLoadedClass->jc);

        while (super)
        {
            fi = getFieldMatching(super, UTF8(cpi1), UTF8(cpi2), 0);

            if (fi)
                break;

            super = getSuperClass(jvm, super);
        }
    }

    if (!fi)
    {
        // TODO: throw NoSuchFieldError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    OperandType type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = OP_LONG; break;
        case 'D': type = OP_DOUBLE; break;
        case 'F': type = OP_FLOAT; break;

        case 'L':
        case '[':
            type = OP_REFERENCE;
            break;

        case 'B': // byte
        case 'C': // char
        case 'I': // int
        case 'S': // short
        case 'Z': // boolean
            type = OP_INTEGER;
            break;

        default:
            // todo: throw exception maybe?
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;
    }

    Reference* object;
    int32_t object_address;

    // Get the objectref
    popOperand(&frame->operands, &object_address, NULL);
    object = (Reference*)object_address;

    if (!object)
    {
        // TODO: throw NullPointerException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    //if (!pushOperand(&frame->operands, *(int32_t*)(object->ci.data + sizeof(int32_t) * fi->offset), type))
    if (!pushOperand(&frame->operands, object->ci.data[fi->offset], type))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    if (type == OP_LONG || type == OP_DOUBLE)
    {
        //if (!pushOperand(&frame->operands, *(int32_t*)(object->ci.data + sizeof(int32_t) * (fi->offset + 1)), type))
        if (!pushOperand(&frame->operands, object->ci.data[fi->offset + 1], type))
        {
            jvm->status = JVM_STATUS_OUT_OF_MEMORY;
            return 0;
        }
    }

    return 1;
}

uint8_t instfunc_putfield(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    // Get the Fieldref CP entry
    cp_info* field = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2;

    LoadedClasses* fieldLoadedClass;

    // Resolve the field, i.e, load the class that field belongs to and
    // the class of the that field, if its type is class.
    if (!resolveField(jvm, frame->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initClass(jvm, fieldLoadedClass))
    {
        // TODO: throw Error
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the field and its descriptor
    cpi2 = frame->jc->constantPool + field->Methodref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    field_info* fi = getFieldMatching(fieldLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!fi)
    {
        // maybe it is located in the super class
        JavaClass* super = getSuperClass(jvm, fieldLoadedClass->jc);

        while (super)
        {
            fi = getFieldMatching(super, UTF8(cpi1), UTF8(cpi2), 0);

            if (fi)
                break;

            super = getSuperClass(jvm, super);
        }
    }

    if (!fi)
    {
        // TODO: throw NoSuchFieldError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    OperandType type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = OP_LONG; break;
        case 'D': type = OP_DOUBLE; break;
        case 'F': type = OP_FLOAT; break;

        case 'L':
        case '[':
            type = OP_REFERENCE;
            break;

        case 'B': // byte
        case 'C': // char
        case 'I': // int
        case 'S': // short
        case 'Z': // boolean
            type = OP_INTEGER;
            break;

        default:
            // todo: throw exception maybe?
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;
    }

    Reference* object;
    int32_t lo_operand;
    int32_t hi_operand;
    int32_t object_address;

    popOperand(&frame->operands, &lo_operand, NULL);

    // If the field is category 2, pop the other operand too
    if (type == OP_LONG || type == OP_DOUBLE)
        popOperand(&frame->operands, &hi_operand, NULL);

    // Get the objectref
    popOperand(&frame->operands, &object_address, NULL);
    object = (Reference*)object_address;

    if (!object)
    {
        // TODO: throw NullPointerException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    if (type == OP_LONG || type == OP_DOUBLE)
    {
        //*(int32_t*)(object->ci.data + sizeof(int32_t) * fi->offset) = hi_operand;
        //*(int32_t*)(object->ci.data + sizeof(int32_t) * (fi->offset + 1)) = lo_operand;
        object->ci.data[fi->offset] = hi_operand;
        object->ci.data[fi->offset + 1] = lo_operand;
    }
    else
    {
        //*(int32_t*)(object->ci.data + sizeof(int32_t) * fi->offset) = lo_operand;
        object->ci.data[fi->offset] = lo_operand;
   }

    return 1;
}

uint8_t instfunc_invokevirtual(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

     // Get the Methodref CP entry
    cp_info* method = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2, *cpi3;
    method_info* mi = NULL;

    if (jvm->simulatingSystemAndStringClasses)
    {
        cpi1 = frame->jc->constantPool + method->Methodref.class_index - 1;
        cpi1 = frame->jc->constantPool + cpi1->Class.name_index - 1;

        cpi2 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
        cpi2 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;

        cpi3 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
        cpi3 = frame->jc->constantPool + cpi3->NameAndType.descriptor_index - 1;

        NativeFunction nativeFunc = getNative(cpi1->Utf8.bytes, cpi1->Utf8.length,
                                              cpi2->Utf8.bytes, cpi2->Utf8.length,
                                              cpi3->Utf8.bytes, cpi3->Utf8.length);

        if (nativeFunc)
            return nativeFunc(jvm, frame, cpi3->Utf8.bytes, cpi3->Utf8.length);
    }

    LoadedClasses* methodLoadedClass;

    // Resolve the method, i.e, load the class that method belongs to
    if (!resolveMethod(jvm, frame->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initClass(jvm, methodLoadedClass))
    {
        // TODO: throw Error
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the method and its descriptor
    cpi2 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    uint8_t operandIndex;
    uint8_t parameterCount = getMethodDescriptorParameterCount(cpi2->Utf8.bytes, cpi2->Utf8.length);
    OperandStack* node = frame->operands;

    for (operandIndex = 0; operandIndex < parameterCount; operandIndex++)
        node = node->next;

    Reference* object = (Reference*)node->value;

    if (object)
    {
        JavaClass* jc = object->ci.c;

        while (jc)
        {
            mi = getMethodMatching(jc, cpi1->Utf8.bytes, cpi1->Utf8.length,
                                   cpi2->Utf8.bytes, cpi2->Utf8.length, 0);

            if (mi)
                break;

            jc = getSuperClass(jvm, jc);
        }
    }
    else
    {
        // TODO: throw NullPointerException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    if (!mi)
    {
        // TODO: throw AbstractMethodError
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }


    // TODO: if the method is static, throw IncompatibleClassChangeError

    return runMethod(jvm, methodLoadedClass->jc, mi, 1 + parameterCount);
}

uint8_t instfunc_invokespecial(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

     // Get the Methodref CP entry
    cp_info* method = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2;
    method_info* mi = NULL;

    LoadedClasses* methodLoadedClass;

    // Resolve the method, i.e, load the class that method belongs to
    if (!resolveMethod(jvm, frame->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initClass(jvm, methodLoadedClass))
    {
        // TODO: throw Error
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the method and its descriptor
    cpi2 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    // If the method being invoked isn't <init> and this class has a super class and
    // the resolved method belongs to class that is a super class of this class,
    // then it is necessary to lookup the super classes for that method
    if (!cmp_UTF8(cpi1->Utf8.bytes, cpi1->Utf8.length, (const uint8_t*)"<init>", 6) &&
        (frame->jc->accessFlags & ACC_SUPER) && isClassSuperOf(jvm, methodLoadedClass->jc, frame->jc))
    {
        JavaClass* super = getSuperClass(jvm, frame->jc);

        while (super)
        {
            mi = getMethodMatching(super, cpi1->Utf8.bytes, cpi1->Utf8.length,
                                   cpi2->Utf8.bytes, cpi2->Utf8.length, 0);

            if (mi)
                break;

            super = getSuperClass(jvm, super);
        }

        if (!mi)
        {
            // TODO: throw AbstractMethodError
            DEBUG_REPORT_INSTRUCTION_ERROR
            return 0;
        }
    }
    else
    {
        mi = getMethodMatching(methodLoadedClass->jc, cpi1->Utf8.bytes, cpi1->Utf8.length,
                               cpi2->Utf8.bytes, cpi2->Utf8.length, 0);
    }

    if (!mi)
    {
        // TODO: throw Error
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // We add one to the parameter count to pop the objectref at the stack as well.
    uint8_t parameterCount = 1 + getMethodDescriptorParameterCount(cpi2->Utf8.bytes, cpi2->Utf8.length);

    return runMethod(jvm, methodLoadedClass->jc, mi, parameterCount);
}

uint8_t instfunc_invokestatic(JavaVirtualMachine* jvm, Frame* frame)
{
    // Get the parameter of the instruction
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    // Get the Methodref CP entry
    cp_info* method = frame->jc->constantPool + index - 1;
    cp_info* cpi1, *cpi2, *cpi3;

    if (jvm->simulatingSystemAndStringClasses)
    {
        cpi1 = frame->jc->constantPool + method->Methodref.class_index - 1;
        cpi1 = frame->jc->constantPool + cpi1->Class.name_index - 1;

        cpi2 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
        cpi2 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;

        cpi3 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
        cpi3 = frame->jc->constantPool + cpi3->NameAndType.descriptor_index - 1;

        NativeFunction nativeFunc = getNative(cpi1->Utf8.bytes, cpi1->Utf8.length,
                                              cpi2->Utf8.bytes, cpi2->Utf8.length,
                                              cpi3->Utf8.bytes, cpi3->Utf8.length);

        if (nativeFunc)
            return nativeFunc(jvm, frame, cpi3->Utf8.bytes, cpi3->Utf8.length);
    }

    LoadedClasses* methodLoadedClass;

    // Resolve the method, i.e, load the class that method belongs to
    if (!resolveMethod(jvm, frame->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initClass(jvm, methodLoadedClass))
    {
        // TODO: throw Error
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // Get the name of the method and its descriptor
    cpi2 = frame->jc->constantPool + method->Methodref.name_and_type_index - 1;
    cpi1 = frame->jc->constantPool + cpi2->NameAndType.name_index - 1;          // name
    cpi2 = frame->jc->constantPool + cpi2->NameAndType.descriptor_index - 1;    // descriptor

    // Find in the method's class the field_info that matches the name and the descriptor
    method_info* mi = getMethodMatching(methodLoadedClass->jc, cpi1->Utf8.bytes, cpi1->Utf8.length,
                                        cpi2->Utf8.bytes, cpi2->Utf8.length, 0);

    if (!mi)
    {
        // TODO: throw Error
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    return runMethod(jvm, methodLoadedClass->jc, mi, getMethodDescriptorParameterCount(cpi2->Utf8.bytes, cpi2->Utf8.length));
}

uint8_t instfunc_invokeinterface(JavaVirtualMachine* jvm, Frame* frame)
{
    // invoke interface is currently implemented
    uint8_t result = instfunc_invokevirtual(jvm, frame);

    if (!result)
        return result;

    // Skip the count and the zero byte of the instruction
    frame->pc += 2;

    return 1;
}

uint8_t instfunc_invokedynamic(JavaVirtualMachine* jvm, Frame* frame)
{
    // This instruction isn't to be implemented
    DEBUG_REPORT_INSTRUCTION_ERROR
    return 0;
}

uint8_t instfunc_new(JavaVirtualMachine* jvm, Frame* frame)
{
    uint16_t index;
    cp_info* cp;
    LoadedClasses* instanceLoadedClass;

    index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    // Get the name of the class of the new instance
    cp = frame->jc->constantPool + index - 1;
    cp = frame->jc->constantPool + cp->Class.name_index - 1;

    if (!resolveClass(jvm, cp->Utf8.bytes, cp->Utf8.length, &instanceLoadedClass) || !instanceLoadedClass)
    {
        // TODO: throw a resolution exception
        // Could be LinkageError, NoClassDefFoundError or IllegalAccessError

        // NOTE: IT IS CURRENTLY NOT POSSIBLE TO CREATE AN INSTANCE OF CLASS String
        // USING THE KEYWORD 'new'.
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    // TODO: check if class is interface or abstract. If so, throw InstantiationError

    Reference* instance = newClassInstance(jvm, instanceLoadedClass);

    if (!instance || !pushOperand(&frame->operands, (int32_t)instance, OP_REFERENCE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_newarray(JavaVirtualMachine* jvm, Frame* frame)
{
    uint8_t type = NEXT_BYTE;
    int32_t count;

    popOperand(&frame->operands, &count, NULL);

    if (count < 0)
    {
        // TODO: throw NegativeArraySizeException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    Reference* arrayref = newArray(jvm, (uint32_t)count, (Opcode_newarray_type)type);

    if (!arrayref || !pushOperand(&frame->operands, (int32_t)arrayref, OP_REFERENCE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_anewarray(JavaVirtualMachine* jvm, Frame* frame)
{
    uint16_t index;
    int32_t count;
    cp_info* cp;

    index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    popOperand(&frame->operands, &count, NULL);

    if (count < 0)
    {
        // TODO: throw NegativeArraySizeException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    cp = frame->jc->constantPool + index - 1;
    cp = frame->jc->constantPool + cp->Class.name_index - 1;

    LoadedClasses* loadedClass;

    if (!resolveClass(jvm, cp->Utf8.bytes, cp->Utf8.length, &loadedClass))
    {
        // TODO: throw a resolution exception
        // Could be LinkageError, NoClassDefFoundError or IllegalAccessError
        return 0;
    }

    Reference* aarray = newObjectArray(jvm, count, cp->Utf8.bytes, cp->Utf8.length);

    if (!aarray || !pushOperand(&frame->operands, (int32_t)aarray, OP_REFERENCE))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_arraylength(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t operand;
    Reference* object;

    popOperand(&frame->operands, &operand, NULL);

    object = (Reference*)operand;

    if (!object)
    {
        // TODO: throw NullPointerException
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    if (object->type == REFTYPE_ARRAY)
    {
        operand = object->arr.length;
    }
    else if (object->type == REFTYPE_OBJARRAY)
    {
        operand = object->oar.length;
    }
    else
    {
        // TODO: maybe throw NullPointerException ?
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    if (!pushOperand(&frame->operands, operand, OP_INTEGER))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_athrow(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction function
    DEBUG_REPORT_INSTRUCTION_ERROR
    return 0;
}

uint8_t instfunc_checkcast(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction function
    DEBUG_REPORT_INSTRUCTION_ERROR
    return 0;
}

uint8_t instfunc_instanceof(JavaVirtualMachine* jvm, Frame* frame)
{
    // TODO: implement this instruction function
    DEBUG_REPORT_INSTRUCTION_ERROR
    return 0;
}

uint8_t instfunc_monitorenter(JavaVirtualMachine* jvm, Frame* frame)
{
    // This instruction isn't to be implemented
    return 1;
}

uint8_t instfunc_monitorexit(JavaVirtualMachine* jvm, Frame* frame)
{
    // This instruction isn't to be implemented
    return 1;
}

uint8_t instfunc_wide(JavaVirtualMachine* jvm, Frame* frame)
{
    uint8_t opcode = NEXT_BYTE;
    uint8_t index;

    if (opcode >= opcode_iload && opcode <= opcode_lload)
        index = opcode - opcode_iload;
    else if (opcode >= opcode_istore || opcode <= opcode_astore)
        index = 5 + opcode - opcode_istore;
    else if (opcode == opcode_ret)
        index = 10;
    else if (opcode == opcode_iinc)
        index = 11;
    else
    {
        // trying to wide an instruction that can't
        // be widened.
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    const InstructionFunction funcs[] = {
        instfunc_wide_iload, instfunc_wide_lload, instfunc_wide_fload,
        instfunc_wide_dload, instfunc_wide_aload, instfunc_wide_istore,
        instfunc_wide_lstore, instfunc_wide_fstore, instfunc_wide_dstore,
        instfunc_wide_dstore, instfunc_wide_ret, instfunc_wide_iinc
    };

    return funcs[index](jvm, frame);
}

uint8_t instfunc_multianewarray(JavaVirtualMachine* jvm, Frame* frame)
{
    uint16_t index;
    uint8_t numberOfDimensions;
    int32_t* dimensions;
    cp_info* cp;

    index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    numberOfDimensions = NEXT_BYTE;
    dimensions = (int32_t*)malloc(sizeof(int32_t) * numberOfDimensions);

    if (!dimensions)
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    uint8_t dimensionIndex = numberOfDimensions;

    while (dimensionIndex--)
    {
        popOperand(&frame->operands, dimensions + dimensionIndex, NULL);

        if (dimensions[dimensionIndex] < 0)
        {
            // TODO: throw NegativeArraySizeException
            DEBUG_REPORT_INSTRUCTION_ERROR
            free(dimensions);
            return 0;
        }
        else if (dimensions[dimensionIndex] == 0)
        {
            // TODO: Java specifies that if the dimension size is equal to 0,
            // no subsequent dimensions should be allocated, but I have no
            // idea of what that means. What could a matrix with one of its
            // dimensions with size zero could possibly mean? int matrix[4][6][0]?
            DEBUG_REPORT_INSTRUCTION_ERROR
            free(dimensions);
            return 0;

        }
    }

    cp = frame->jc->constantPool + index - 1;
    cp = frame->jc->constantPool + cp->Class.name_index - 1;

    if (!resolveClass(jvm, cp->Utf8.bytes, cp->Utf8.length, NULL))
    {
        // TODO: throw a resolution exception
        // Could be LinkageError, NoClassDefFoundError or IllegalAccessError
        free(dimensions);
        return 0;
    }

    Reference* aarray = newObjectMultiArray(jvm, dimensions, numberOfDimensions, cp->Utf8.bytes, cp->Utf8.length);

    if (!aarray || !pushOperand(&frame->operands, (int32_t)aarray, OP_REFERENCE))
    {
        free(dimensions);
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    free(dimensions);
    return 1;
}

uint8_t instfunc_ifnull(JavaVirtualMachine* jvm, Frame* frame)
{
    int16_t branch = NEXT_BYTE;
    branch = (branch << 8) | NEXT_BYTE;

    int32_t address;

    popOperand(&frame->operands, &address, NULL);

    if (!address)
        frame->pc += branch - 3;

    return 1;
}

uint8_t instfunc_ifnonnull(JavaVirtualMachine* jvm, Frame* frame)
{
    int16_t branch = NEXT_BYTE;
    branch = (branch << 8) | NEXT_BYTE;

    int32_t address;

    popOperand(&frame->operands, &address, NULL);

    if (address)
        frame->pc += branch - 3;

    return 1;
}

uint8_t instfunc_goto_w(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    frame->pc += offset - 5;
    return 1;
}

uint8_t instfunc_jsr_w(JavaVirtualMachine* jvm, Frame* frame)
{
    int32_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;

    if (!pushOperand(&frame->operands, (int32_t)frame->pc, OP_RETURNADDRESS))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    frame->pc += offset - 5;
    return 1;
}

/// @brief Retrieves the instruction function for a given
/// instruction opcode.
/// @return The function that needs to be called for the
/// instruction to be executed if there is one for the given
/// opcode. Otherwise, returns NULL.
/// @see Opcodes, getOpcodeMnemonic()
InstructionFunction fetchOpcodeFunction(uint8_t opcode)
{
    const InstructionFunction opcodeFunctions[202] = {
        instfunc_nop, instfunc_aconst_null, instfunc_iconst_m1,
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
        instfunc_dup_x1, instfunc_dup_x2, instfunc_dup2,
        instfunc_dup2_x1, instfunc_dup2_x2, instfunc_swap,
        instfunc_iadd, instfunc_ladd, instfunc_fadd,
        instfunc_dadd, instfunc_isub, instfunc_lsub,
        instfunc_fsub, instfunc_dsub, instfunc_imul,
        instfunc_lmul, instfunc_fmul, instfunc_dmul,
        instfunc_idiv, instfunc_ldiv, instfunc_fdiv,
        instfunc_ddiv, instfunc_irem, instfunc_lrem,
        instfunc_frem, instfunc_drem, instfunc_ineg,
        instfunc_lneg, instfunc_fneg, instfunc_dneg,
        instfunc_ishl, instfunc_lshl, instfunc_ishr,
        instfunc_lshr, instfunc_iushr, instfunc_lushr,
        instfunc_iand, instfunc_land, instfunc_ior,
        instfunc_lor, instfunc_ixor, instfunc_lxor,
        instfunc_iinc, instfunc_i2l, instfunc_i2f,
        instfunc_i2d, instfunc_l2i, instfunc_l2f,
        instfunc_l2d, instfunc_f2i, instfunc_f2l,
        instfunc_f2d, instfunc_d2i, instfunc_d2l,
        instfunc_d2f, instfunc_i2b, instfunc_i2c,
        instfunc_i2s, instfunc_lcmp, instfunc_fcmpl,
        instfunc_fcmpg, instfunc_dcmpl, instfunc_dcmpg,
        instfunc_ifeq, instfunc_ifne, instfunc_iflt,
        instfunc_ifge, instfunc_ifgt, instfunc_ifle,
        instfunc_if_icmpeq, instfunc_if_icmpne, instfunc_if_icmplt,
        instfunc_if_icmpge, instfunc_if_icmpgt, instfunc_if_icmple,
        instfunc_if_acmpeq, instfunc_if_acmpne, instfunc_goto,
        instfunc_jsr, instfunc_ret, instfunc_tableswitch,
        instfunc_lookupswitch, instfunc_ireturn, instfunc_lreturn,
        instfunc_freturn, instfunc_dreturn, instfunc_areturn,
        instfunc_return, instfunc_getstatic, instfunc_putstatic,
        instfunc_getfield, instfunc_putfield, instfunc_invokevirtual,
        instfunc_invokespecial, instfunc_invokestatic, instfunc_invokeinterface,
        instfunc_invokedynamic, instfunc_new, instfunc_newarray,
        instfunc_anewarray, instfunc_arraylength, instfunc_athrow,
        instfunc_checkcast, instfunc_instanceof, instfunc_monitorenter,
        instfunc_monitorexit, instfunc_wide, instfunc_multianewarray,
        instfunc_ifnull, instfunc_ifnonnull, instfunc_goto_w,
        instfunc_jsr_w
    };

    if (opcode > 201)
        return NULL;

    return opcodeFunctions[opcode];
}
