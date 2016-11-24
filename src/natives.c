#include "natives.h"
#include "memoryinspect.h"
#include "utf8.h"
#include "jvm.h"
#include <string.h>
#include <inttypes.h>

uint8_t native_println(JavaVirtualMachine* jvm, Frame* frame)
{
    union {
        int64_t longval;
        int32_t intval;
        float floatval;
        double doubleval;
    } val;

    int32_t operand;
    OperandType type;

    popOperand(&frame->operands, &operand, &type);

    val.intval = operand;

    if (type == OP_LONG || type == OP_DOUBLE)
    {
        popOperand(&frame->operands, &operand, NULL);
        val.longval = val.intval;
        val.longval = (val.longval << 32) | operand;
    }

    switch(type)
    {
        case OP_DOUBLE: printf("%g", val.doubleval); break;
        case OP_FLOAT: printf("%g", val.floatval); break;
        case OP_INTEGER: printf("%d", val.intval); break;
        case OP_LONG: printf("%" PRId64"", val.longval); break;
        case OP_REFERENCE:
        {
            Reference* obj = (Reference*)val.intval;

            if (obj->type == REFTYPE_STRING)
                printf("%.*s", obj->str.len, obj->str.utf8_bytes);
            else
                printf("0x%.8X", val.intval); break;
        }

        default:
            break;
    }

    printf("\n");

    // Pop out the "java/lang/System.out" static field
    popOperand(&frame->operands, NULL, NULL);

    return 1;
}

InstructionFunction getNative(const uint8_t* className, int32_t classLen, const uint8_t* methodName, int32_t methodLen)
{
    const struct {
        char* methodname;
        int32_t methodlen;
        char* classname;
        int32_t classlen;
        InstructionFunction func;
    } nativeMethods[] = {
        {"println", 7, "java/io/PrintStream", 19, native_println}
    };

    uint32_t index;

    for (index = 0; index < sizeof(nativeMethods)/sizeof(*nativeMethods); index++)
    {
        if (cmp_UTF8((uint8_t*)nativeMethods[index].methodname, nativeMethods[index].methodlen, methodName, methodLen) &&
            cmp_UTF8((uint8_t*)nativeMethods[index].classname, nativeMethods[index].classlen, className, classLen))
            return nativeMethods[index].func;
    }

    return NULL;
}
