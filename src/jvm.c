#include "jvm.h"
#include "utf8.h"
#include "instructions.h"
#include "opcodes.h"

#include "memoryinspect.h"
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

        if (tmp->staticFieldsData)
            free(tmp->staticFieldsData);

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

    // Get the class at the top of the stack of loaded classes.
    // Normally, only one class is loaded and then this function is
    // called, so that one and only class will be the entry point.
    method_info* method = getMethodMatching(jvm->classes->jc, "main", "([Ljava/lang/String;)V", ACC_STATIC);

    if (!method)
    {
        jvm->status = JVM_STATUS_MAIN_METHOD_NOT_FOUND;
        return;
    }

    if (!runMethod(jvm, jvm->classes->jc, method))
        return;
}

uint8_t resolveClass(JavaVirtualMachine* jvm, const uint8_t* className_utf8_bytes, int32_t utf8_len, LoadedClasses** outClass)
{
    JavaClass* jc;
    cp_info* cpi;
    char path[1024];
    uint8_t success = 1;
    uint16_t u16;

    LoadedClasses* loadedClass = isClassLoaded(jvm, className_utf8_bytes, utf8_len);

    if (loadedClass)
    {
        if (outClass)
            *outClass = loadedClass;

        return 1;
    }

#ifdef DEBUG
    printf("debug resolveClass %.*s\n", utf8_len, className_utf8_bytes);
#endif // DEBUG

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
            success = resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length, NULL);
        }

        for (u16 = 0; success && u16 < jc->interfaceCount; u16++)
        {
            cpi = jc->constantPool + jc->interfaces[u16] - 1;
            cpi = jc->constantPool + cpi->Class.name_index - 1;
            success = resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length, NULL);
        }
    }

    if (success)
    {

#ifdef DEBUG
    printf("   class file '%s' loaded\n", path);
#endif // DEBUG

        loadedClass = addClassToLoadedClasses(jvm, jc);

        method_info* clinit = getMethodMatching(jc, "<clinit>", "()V", ACC_STATIC);

        if (clinit)
        {
            if (!runMethod(jvm, jc, clinit))
                success = 0;
        }

        if (outClass)
            *outClass = loadedClass;
    }
    else
    {
        jvm->status = JVM_STATUS_CLASS_RESOLUTION_FAILED;
        closeClassFile(jc);
        free(jc);
    }

    return success;
}

uint8_t resolveMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method)
{
    // TODO: resolve method
    jvm->status = JVM_STATUS_METHOD_RESOLUTION_FAILED;
    return 0;
}

uint8_t resolveField(JavaVirtualMachine* jvm, JavaClass* jc, cp_info* cp_field, LoadedClasses** outClass)
{

#ifdef DEBUG
    {
        char debugbuffer[256];
        uint32_t length = 0;
        cp_info* debugcpi = jc->constantPool + cp_field->Fieldref.class_index - 1;
        debugcpi = jc->constantPool + debugcpi->Class.name_index - 1;
        length += snprintf(debugbuffer + length, sizeof(debugbuffer) - length, "%.*s.", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
        debugcpi = jc->constantPool + cp_field->Fieldref.name_and_type_index - 1;
        debugcpi = jc->constantPool + debugcpi->NameAndType.name_index - 1;
        length += snprintf(debugbuffer + length, sizeof(debugbuffer) - length, "%.*s:", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
        debugcpi = jc->constantPool + cp_field->Fieldref.name_and_type_index - 1;
        debugcpi = jc->constantPool + debugcpi->NameAndType.descriptor_index - 1;
        length += snprintf(debugbuffer + length, sizeof(debugbuffer) - length, "%.*s", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
        printf("debug resolveField %s\n", debugbuffer);
    }
#endif // DEBUG

    cp_info* cpi;

    cpi = jc->constantPool + cp_field->Fieldref.class_index - 1;
    cpi = jc->constantPool + cpi->Class.name_index - 1;

    // Resolve the class the field belongs to
    if (!resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length, outClass))
        return 0;

    cpi = jc->constantPool + cp_field->Fieldref.name_and_type_index - 1;
    cpi = jc->constantPool + cpi->NameAndType.descriptor_index - 1;

    uint8_t* descriptor_bytes = cpi->Utf8.bytes;
    int32_t descriptor_len = cpi->Utf8.length;

    // Skip '[' characters, in case this field is an array
    while (*descriptor_bytes == '[')
    {
        descriptor_bytes++;
        descriptor_len--;
    }

    // If the type of this field is a class, then that
    // class must also be resolved
    if (*descriptor_bytes == 'L')
    {
        if (!resolveClass(jvm, descriptor_bytes + 1, descriptor_len - 2, NULL))
            return 0;
    }

    return 1;
}

uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method)
{
#ifdef DEBUG
    {
        cp_info* debug_cpi = jc->constantPool + method->name_index - 1;
        printf("debug runMethod %.*s\n", debug_cpi->Utf8.length, debug_cpi->Utf8.bytes);
    }
#endif // DEBUG

    Frame* callerFrame = jvm->frames ? jvm->frames->frame : NULL;
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

    if (frame->returnCount > 0 && callerFrame)
    {
        // TODO: move return values from one frame to another
    }

    popFrame(&jvm->frames, NULL);
    freeFrame(frame);
    return jvm->status == JVM_STATUS_OK;
}

LoadedClasses* addClassToLoadedClasses(JavaVirtualMachine* jvm, JavaClass* jc)
{
    LoadedClasses* node = (LoadedClasses*)malloc(sizeof(LoadedClasses));

    if (node)
    {
        node->jc = jc;
        node->staticFieldsData = (int32_t*)malloc(sizeof(int32_t) * jc->staticFieldCount);
        node->next = jvm->classes;
        jvm->classes = node;
    }

    return node;
}

LoadedClasses* isClassLoaded(JavaVirtualMachine* jvm, const uint8_t* utf8_bytes, int32_t utf8_len)
{
    LoadedClasses* classes = jvm->classes;
    JavaClass* jc;
    cp_info* cpi;

    while (classes)
    {
        jc = classes->jc;
        cpi = jc->constantPool + jc->thisClass - 1;
        cpi = jc->constantPool + cpi->Class.name_index - 1;

        if (cmp_UTF8(cpi->Utf8.bytes, cpi->Utf8.length, utf8_bytes, utf8_len))
            return classes;

        classes = classes->next;
    }

    return NULL;
}
