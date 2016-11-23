#include "jvm.h"
#include "utf8.h"
#include "instructions.h"

#include "memoryinspect.h"
#include <string.h>

/// @brief Initializes a JavaVirtualMachine structure.
///
/// @param JavaVirtualMachine* jvm - pointer to the structure to be
/// initialized.
///
/// This function must be called before calling other JavaVirtualMachine functions.
///
/// @see deinitJVM()
void initJVM(JavaVirtualMachine* jvm)
{
    jvm->status = JVM_STATUS_OK;
    jvm->frames = NULL;
    jvm->classes = NULL;
    jvm->objects = NULL;
}

/// @brief Deallocates all memory used by the JavaVirtualMachine structure.
///
/// @param JavaVirtualMachine* jvm - pointer to the structure to be
/// deallocated.
///
/// All loaded classes and objects created during the execution of the JVM
/// will be freed.
void deinitJVM(JavaVirtualMachine* jvm)
{
    freeFrameStack(&jvm->frames);

    LoadedClasses* classnode = jvm->classes;
    LoadedClasses* classtmp;

    while (classnode)
    {
        classtmp = classnode;
        classnode = classnode->next;
        closeClassFile(classtmp->jc);
        free(classtmp->jc);

        if (classtmp->staticFieldsData)
            free(classtmp->staticFieldsData);

        free(classtmp);
    }

    ReferenceTable* refnode = jvm->objects;
    ReferenceTable* reftmp;

    while (refnode)
    {
        reftmp = refnode;
        refnode = refnode->next;

        // TODO: free the reference internal data

        free(reftmp->obj);
        free(reftmp);
    }

    jvm->objects = NULL;
    jvm->classes = NULL;
}

/// @brief Executes the main method of a given class.
///
/// @param JavaVirtualMachine* jvm - pointer to an
/// already initialized JVM structure.
/// @param JavaClass* mainClass - pointer to a class
/// that contains the public void static main method.
///
/// If \c mainClass is a null pointer, the class
/// at the top of the stack of loaded classes will be
/// used as entry point. If no classes are loaded, then
/// the status of the JVM will be changed to
/// \c JVM_STATUS_MAIN_METHOD_NOT_FOUND.
/// If \c mainClass is not a null pointer, the class must
/// have been previously resolved with a call to \c resolveClass().
///
/// @see resolveClass()
void executeJVM(JavaVirtualMachine* jvm, JavaClass* mainClass)
{
    if (!mainClass)
    {
        if (!jvm->classes || !jvm->classes->jc)
        {
            jvm->status = JVM_STATUS_NO_CLASS_LOADED;
            return;
        }

        mainClass = jvm->classes->jc;
    }

    method_info* method = getMethodMatching(mainClass, "main", "([Ljava/lang/String;)V", ACC_STATIC);

    if (!method)
    {
        jvm->status = JVM_STATUS_MAIN_METHOD_NOT_FOUND;
        return;
    }

    if (!runMethod(jvm, mainClass, method, 0))
        return;
}

/// @brief Loads a .class file.
///
///
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
            if (!runMethod(jvm, jc, clinit, 0))
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

uint8_t resolveMethod(JavaVirtualMachine* jvm, JavaClass* jc, cp_info* cp_method, LoadedClasses** outClass)
{
#ifdef DEBUG
    {
        char debugbuffer[256];
        uint32_t length = 0;
        cp_info* debugcpi = jc->constantPool + cp_method->Methodref.class_index - 1;
        debugcpi = jc->constantPool + debugcpi->Class.name_index - 1;
        length += snprintf(debugbuffer + length, sizeof(debugbuffer) - length, "%.*s.", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
        debugcpi = jc->constantPool + cp_method->Methodref.name_and_type_index - 1;
        debugcpi = jc->constantPool + debugcpi->NameAndType.name_index - 1;
        length += snprintf(debugbuffer + length, sizeof(debugbuffer) - length, "%.*s:", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
        debugcpi = jc->constantPool + cp_method->Methodref.name_and_type_index - 1;
        debugcpi = jc->constantPool + debugcpi->NameAndType.descriptor_index - 1;
        length += snprintf(debugbuffer + length, sizeof(debugbuffer) - length, "%.*s", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
        printf("debug resolveMethod %s\n", debugbuffer);
    }
#endif // DEBUG

    cp_info* cpi;

    cpi = jc->constantPool + cp_method->Methodref.class_index - 1;
    cpi = jc->constantPool + cpi->Class.name_index - 1;

    // Resolve the class the method belongs to
    if (!resolveClass(jvm, cpi->Utf8.bytes, cpi->Utf8.length, outClass))
        return 0;

    // Get method descriptor
    cpi = jc->constantPool + cp_method->Methodref.name_and_type_index - 1;
    cpi = jc->constantPool + cpi->NameAndType.descriptor_index - 1;

    uint8_t* descriptor_bytes = cpi->Utf8.bytes;
    int32_t descriptor_len = cpi->Utf8.length;
    int32_t length;

    while (descriptor_len > 0)
    {
        // We increment our descriptor here. This will make the first
        // character to be lost, but the first character in a method
        // descriptor is a parenthesis, so it doesn't matter.
        descriptor_bytes++;
        descriptor_len--;

        switch(*descriptor_bytes)
        {
            // if the method has a class as parameter or as return type,
            // that class must be resolved
            case 'L':

                length = -1;

                do {
                    descriptor_bytes++;
                    descriptor_len--;
                    length++;
                } while (*descriptor_bytes != ';');

                if (!resolveClass(jvm, descriptor_bytes - length, length, NULL))
                    return 0;

                break;

            default:
                break;
        }
    }

    return 1;
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

    // Get field descriptor
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

uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method, uint8_t numberOfParameters)
{
#ifdef DEBUG
    {
        cp_info* debug_cpi = jc->constantPool + method->name_index - 1;
        printf("debug runMethod %.*s, params: %u", debug_cpi->Utf8.length, debug_cpi->Utf8.bytes, numberOfParameters);
    }
#endif // DEBUG

    Frame* callerFrame = jvm->frames ? jvm->frames->frame : NULL;
    Frame* frame = newFrame(jc, method);

#ifdef DEBUG
    printf(", code len: %u\n", frame->code_length);
#endif // DEBUG

    if (!frame || !pushFrame(&jvm->frames, frame))
    {
        jvm->status = JVM_STATUS_OUT_OF_MEMORY;
        return 0;
    }

    uint8_t parameterIndex;
    int32_t parameter;

    for (parameterIndex = 0; parameterIndex < numberOfParameters; parameterIndex++)
    {
        popOperand(&callerFrame->operands, &parameter, NULL);
        frame->localVariables[parameterIndex] = parameter;
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
            return 0;
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

Reference* newClassInstance(JavaVirtualMachine* jvm, JavaClass* jc)
{
    Reference* r = (Reference*)malloc(sizeof(Reference));
    ReferenceTable* node = (ReferenceTable*)malloc(sizeof(ReferenceTable));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REFTYPE_CLASSINSTANCE;
    r->ci.c = jc;
    r->ci.data = (uint8_t*)malloc(jc->instanceFieldCount);

    if (!r->ci.data)
    {
        free(r);
        free(node);
        return NULL;
    }

    node->next = jvm->objects;
    node->obj = r;
    jvm->objects = node;

    return r;
}
