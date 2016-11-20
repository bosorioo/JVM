#include "jvm.h"
#include "utf8.h"
#include "instructions.h"
#include <stdlib.h>

void initJVM(JavaVirtualMachine* jvm)
{
    jvm->status = JVM_STATUS_OK;
    jvm->frames = NULL;
    jvm->classes = NULL;
}

void executeJVM(JavaVirtualMachine* jvm)
{
    if (!jvm->classes || !jvm->classes->jc)
    {
        jvm->status = JVM_STATUS_NO_CLASS_LOADED;
        return;
    }

    JavaClass* jc = jvm->classes->jc;
    method_info* method = getMethodByName(jc, "<clinit>");

    if (method)
    {
        if (!runMethod(jvm, jc, method))
            return;
    }

    method = getMethodByName(jc, "main");

    if (!method || !runMethod(jvm, jc, method))
        return;
}

uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method)
{
    Frame* frame = newFrame(jc, method);

    if (!frame || !pushFrame(&jvm->frames, frame))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    InstructionFunction function;

    while (frame->pc < frame->code_length)
    {
        uint8_t opcode = *(frame->code + frame->pc++);
        function = fetchOpcodeFunction(opcode);

        if (function == NULL)
        {
            jvm->status = JVM_STATUS_UNKNOWN_INSTRUCTION;
            break;
        }
        else if (!function(jvm, frame))
        {
            break;
        }
    }

    popFrame(&jvm->frames, NULL);
    freeFrame(frame);
    return 1;
}

uint8_t addClassToMethodArea(JavaVirtualMachine* jvm, JavaClass* jc)
{
    LoadedClasses* node = (LoadedClasses*)malloc(sizeof(LoadedClasses));

    if (node)
    {
        node->jc = jc;
        node->next = jvm->classes;
        jvm->classes = node;
    }

    return node != NULL;
}

uint8_t isClassLoaded(JavaVirtualMachine* jvm, const uint8_t* utf8_bytes, int32_t utf8_len)
{
    LoadedClasses* classes = jvm->classes;
    JavaClass* jc;
    cp_info* cpi;

    while (classes)
    {
        jc = classes->jc;
        cpi = jc->constantPool + jc->thisClass - 1;
        cpi = jc->constantPool + cpi->Class.name_index - 1;

        if (cmp_UTF8_Ascii(cpi->Utf8.bytes, cpi->Utf8.length, utf8_bytes, utf8_len))
            return 1;

        classes = classes->next;
    }

    return 0;
}
