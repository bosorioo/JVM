#include "jvm.h"
#include "utf8.h"
#include "instructions.h"
#include "opcodes.h"

#include <stdlib.h>
#include <string.h>

void initJVM(JavaVirtualMachine* jvm)
{
    jvm->status = JVM_STATUS_OK;
    jvm->frames = NULL;
    jvm->classes = NULL;
}

void deinitJVM(JavaVirtualMachine* jvm)
{
    freeFrameStack(&jvm->frames);

    LoadedClasses* classnode = jvm->classes;
    LoadedClasses* tmp;

    while (classnode)
    {
        tmp = classnode;
        classnode = classnode->next;
        closeClassFile(tmp->jc);
        free(tmp->jc);
        free(tmp);
    }

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
    method_info* method = getMethodMatching(jc, "<clinit>", "()V", ACC_STATIC);

    if (method)
    {
        if (!runMethod(jvm, jc, method))
            return;
    }

    method = getMethodMatching(jc, "main", "([Ljava/lang/String;)V", ACC_STATIC);

    if (!method)
    {
        jvm->status = JVM_STATUS_MAIN_METHOD_NOT_FOUND;
        return;
    }

    if (!runMethod(jvm, jc, method))
        return;
}

uint8_t resolveClass(JavaVirtualMachine* jvm, const uint8_t* className_utf8_bytes, int32_t utf8_len)
{
#ifdef DEBUG
    printf("debug resolveClass %.*s\n", utf8_len, className_utf8_bytes);
#endif // DEBUG

    JavaClass* jc;
    cp_info* cpi;
    char path[1024];
    uint8_t success = 1;
    uint16_t u16;

    if (cmp_UTF8_Ascii(className_utf8_bytes, utf8_len, (const uint8_t*)"java/lang/Object", 16))
        return 1;

    snprintf(path, sizeof(path), "%.*s.class", utf8_len, className_utf8_bytes);

    jc = (JavaClass*)malloc(sizeof(JavaClass));
    openClassFile(jc, path);

    if (jc->status != CLASS_STATUS_OK)
    {

#ifdef DEBUG
    printf("   class '%.*s' loading failed\n", utf8_len, className_utf8_bytes);
    printf("   status: %s\n", decodeJavaClassStatus(jc->status));
#endif // DEBUG

        success = 0;
    }
    else
    {
        if (jc->superClass)
        {
            cpi = jc->constantPool + jc->superClass - 1;
            cpi = jc->constantPool + cpi->Class.name_index - 1;
            success = resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length);
        }

        for (u16 = 0; success && u16 < jc->interfaceCount; u16++)
        {
            cpi = jc->constantPool + jc->interfaces[u16] - 1;
            cpi = jc->constantPool + cpi->Class.name_index - 1;
            success = resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length);
        }
    }

    if (success)
    {

#ifdef DEBUG
    printf("   class file '%s' loaded\n", path);
#endif // DEBUG

        addClassToLoadedClasses(jvm, jc);
    }
    else
    {
        jvm->status = JVM_STATUS_CLASS_RESOLUTION_FAILED;
        closeClassFile(jc);
        free(jc);
    }

    return success;
}

uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method)
{
#ifdef DEBUG
    {
        cp_info* debug_cpi = jc->constantPool + method->name_index - 1;
        printf("debug runMethod %.*s\n", debug_cpi->Utf8.length, debug_cpi->Utf8.bytes);
    }
#endif // DEBUG

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

#ifdef DEBUG
        printf("   instruction '%s' at offset %u\n", getOpcodeMnemonic(opcode), frame->pc - 1);
#endif // DEBUG

        if (function == NULL)
        {

#ifdef DEBUG
        printf("   unknown instruction '%s'\n", getOpcodeMnemonic(opcode));
#endif // DEBUG

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
    return jvm->status == JVM_STATUS_OK;
}

uint8_t addClassToLoadedClasses(JavaVirtualMachine* jvm, JavaClass* jc)
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
