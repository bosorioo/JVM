#include "natives.h"
#include "readfunctions.h"
#include "memoryinspect.h"
#include "utf8.h"
#include "jvm.h"
#include <string.h>
#include <inttypes.h>

uint8_t native_println(JavaVirtualMachine* jvm, Frame* frame, const uint8_t* descriptor_utf8, int32_t utf8_len)
{
    int64_t longvalue = 0;
    int32_t high, low;

    if (utf8_len < 2)
    {
        DEBUG_REPORT_INSTRUCTION_ERROR
        return 0;
    }

    switch (descriptor_utf8[1])
    {
        case ')': break;

        case 'Z':
            popOperand(&frame->operands, &low, NULL);
            printf("%s", (int8_t)low ? "true" : "false");
            break;

        case 'B':
            popOperand(&frame->operands, &low, NULL);
            printf("%d", (int8_t)low);
            break;

        case 'C':
            popOperand(&frame->operands, &low, NULL);
            printf("%c%c", (int16_t)low >> 8, (int16_t)low & 0xFF);
            break;

        case 'D':
        case 'J':
            popOperand(&frame->operands, &low, NULL);
            popOperand(&frame->operands, &high, NULL);
            longvalue = high;
            longvalue = (longvalue << 32) | (uint32_t)low;

            if (descriptor_utf8[1] == 'D')
                printf("%#f", readDoubleFromUint64(longvalue));
            else
                printf("%" PRId64"", longvalue);

            break;

        case 'F':
            popOperand(&frame->operands, &low, NULL);
            printf("%#f", readFloatFromUint32(low));
            break;

        case 'I':
            popOperand(&frame->operands, &low, NULL);
            printf("%d", low);
            break;

        case 'L':
        {
            popOperand(&frame->operands, &low, NULL);
            Reference* obj = (Reference*)low;

            if (obj->type == REFTYPE_STRING)
                printf("%.*s", obj->str.len, obj->str.utf8_bytes);
            else
                printf("0x%.8X", low);

            break;
        }

        case '[':
            popOperand(&frame->operands, &low, NULL);
            printf("0x%X", low);
            break;

        default:
            break;
    }

    printf("\n");

    // Pop out the "java/lang/System.out" static field
    popOperand(&frame->operands, NULL, NULL);

    return 1;
}

NativeFunction getNative(const uint8_t* className, int32_t classLen,
                         const uint8_t* methodName, int32_t methodLen,
                         const uint8_t* descriptor, int32_t descrLen)
{
    const struct {
        char* classname; int32_t classlen;
        char* methodname; int32_t methodlen;
        char* descriptor; int32_t descrlen;
        NativeFunction func;
    } nativeMethods[] = {
        {"java/io/PrintStream", 19, "println", 7, NULL, 0, native_println}
    };

    uint32_t index;

    for (index = 0; index < sizeof(nativeMethods)/sizeof(*nativeMethods); index++)
    {
        if (cmp_UTF8((uint8_t*)nativeMethods[index].methodname, nativeMethods[index].methodlen, methodName, methodLen) &&
            cmp_UTF8((uint8_t*)nativeMethods[index].classname, nativeMethods[index].classlen, className, classLen))
        {
            if (!nativeMethods[index].descriptor ||
                cmp_UTF8((uint8_t*)nativeMethods[index].descriptor, nativeMethods[index].descrlen, descriptor, descrLen))
            {
                return nativeMethods[index].func;
            }
        }
    }

    return NULL;
}
