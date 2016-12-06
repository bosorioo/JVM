#include "jvm.h"
#include "utf8.h"
#include "natives.h"
#include "instructions.h"

#include "debugging.h"
#include <string.h>

/// @brief Initializes a JavaVirtualMachine structure.
///
/// @param JavaVirtualMachine* jvm - pointer to the structure to be
/// initialized.
///
/// This function must be called before calling other JavaVirtualMachine functions.
///
/// @see executeJVM(), deinitJVM()
void initJVM(JavaVirtualMachine* jvm)
{
    jvm->status = JVM_STATUS_OK;
    jvm->frames = NULL;
    jvm->classes = NULL;
    jvm->objects = NULL;

    jvm->classPath[0] = '\0';

    // We need to simulate those two classes, and their support is
    // highly limited. Reading them from the Oracle .class files
    // requires processing of many other .class, including
    // dealing with native methods.
    jvm->simulatingSystemAndStringClasses = 1;
}

/// @brief Deallocates all memory used by the JavaVirtualMachine structure.
///
/// @param JavaVirtualMachine* jvm - pointer to the structure to be
/// deallocated.
///
/// All loaded classes and objects created during the execution of the JVM
/// will be freed.
///
/// @see initJVM()
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
        deleteReference(reftmp->obj);
        free(reftmp);
    }

    jvm->objects = NULL;
    jvm->classes = NULL;
}

/// @brief Executes the main method of a given class.
///
/// @param JavaVirtualMachine* jvm - pointer to an
/// already initialized JVM structure.
/// @param LoadedClasses* mainClass - pointer to a class
/// that has been loaded and contains the public void static
/// main method.
///
/// If \c mainClass is a null pointer, the class
/// at the top of the stack of loaded classes will be
/// used as entry point. If no classes are loaded, then
/// the status of the JVM will be changed to
/// \c JVM_STATUS_MAIN_METHOD_NOT_FOUND.
/// If \c mainClass is not a null pointer, the class must
/// have been previously resolved with a call to \c resolveClass().
///
/// @see resolveClass(), JavaClass
void executeJVM(JavaVirtualMachine* jvm, LoadedClasses* mainClass)
{
    if (!mainClass)
    {
        if (!jvm->classes || !jvm->classes->jc)
        {
            jvm->status = JVM_STATUS_NO_CLASS_LOADED;
            return;
        }

        mainClass = jvm->classes;
    }
    else if (!initClass(jvm, mainClass))
    {
        jvm->status = JVM_STATUS_MAIN_CLASS_RESOLUTION_FAILED;
        return;
    }

    const uint8_t name[] = "main";
    const uint8_t descriptor[] = "([Ljava/lang/String;)V";

    method_info* method = getMethodMatching(mainClass->jc, name, sizeof(name) - 1, descriptor, sizeof(descriptor) - 1, ACC_STATIC);

    if (!method)
    {
        jvm->status = JVM_STATUS_MAIN_METHOD_NOT_FOUND;
        return;
    }

    if (!runMethod(jvm, mainClass->jc, method, 0))
        return;
}

void setClassPath(JavaVirtualMachine* jvm, const char* path)
{
    uint32_t index;
    uint32_t lastSlash = 0;

    for (index = 0; path[index]; index++)
    {
        if (path[index] == '/' || path[index] == '\\')
            lastSlash = index;
    }

    if (lastSlash)
        memcpy(jvm->classPath, path, lastSlash + 1);
    else
        jvm->classPath[0] = '\0';
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

    if (jvm->simulatingSystemAndStringClasses &&
        cmp_UTF8(className_utf8_bytes, utf8_len, (const uint8_t*)"java/lang/String", 16))
    {
        return 1;
    }

    // Arrays can also be a class in the constant pool that need
    // to be resolved.
    if (utf8_len > 0 && *className_utf8_bytes == '[')
    {
        do {
            utf8_len--;
            className_utf8_bytes++;
        } while (utf8_len > 0 && *className_utf8_bytes == '[');

        if (utf8_len == 0)
        {
            // Something wrong, as the class name was something like
            // "[["?
            return 0;
        }
        else if (*className_utf8_bytes != 'L')
        {
            // This class is an array of a primitive type, which
            // require no resolution.
            if (outClass)
                *outClass = NULL;
            return 1;
        }
        else
        {
            // This class is an array of instance of a certain class,
            // for example: "[Ljava/lang/String;", so we just try to
            // resolve the class java/lang/String.
            return resolveClass(jvm, className_utf8_bytes + 1, utf8_len - 2, outClass);
        }
    }

    LoadedClasses* loadedClass = isClassLoaded(jvm, className_utf8_bytes, utf8_len);

    if (loadedClass)
    {
        if (outClass)
            *outClass = loadedClass;

        return 1;
    }

#ifdef DEBUG
    printf("Resolving class %.*s\n", utf8_len, className_utf8_bytes);
#endif // DEBUG

    snprintf(path, sizeof(path), "%.*s.class", utf8_len, className_utf8_bytes);

    jc = (JavaClass*)malloc(sizeof(JavaClass));
    openClassFile(jc, path);

    if (jc->status == CLASS_STATUS_FILE_COULDNT_BE_OPENED && jvm->classPath[0])
    {
        snprintf(path, sizeof(path), "%s%.*s.class", jvm->classPath, utf8_len, className_utf8_bytes);
        openClassFile(jc, path);
    }

    if (jc->status != CLASS_STATUS_OK)
    {

#ifdef DEBUG
    printf("   class '%.*s' loading failed.\n", utf8_len, className_utf8_bytes);
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
            success = resolveClass(jvm, UTF8(cpi), &loadedClass);

            if (success)
            {
                jc->instanceFieldCount += loadedClass->jc->instanceFieldCount;

                for (u16 = 0; u16 < jc->fieldCount; u16++)
                    jc->fields[u16].offset += loadedClass->jc->instanceFieldCount;
            }

        }

        for (u16 = 0; success && u16 < jc->interfaceCount; u16++)
        {
            cpi = jc->constantPool + jc->interfaces[u16] - 1;
            cpi = jc->constantPool + cpi->Class.name_index - 1;
            success = resolveClass(jvm, UTF8(cpi), NULL);
        }
    }

    if (success)
    {
        loadedClass = addClassToLoadedClasses(jvm, jc);
        success = loadedClass != NULL;
    }

    if (success)
    {

#ifdef DEBUG
    printf("   class file '%s' loaded.\n", path);
#endif // DEBUG

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
    printf("Resolving method ");
    debugPrintMethodFieldRef(jc, cp_method);
    printf("\n");
#endif // DEBUG

    cp_info* cpi;

    cpi = jc->constantPool + cp_method->Methodref.class_index - 1;
    cpi = jc->constantPool + cpi->Class.name_index - 1;

    // Resolve the class the method belongs to
    if (!resolveClass(jvm, UTF8(cpi), outClass))
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
    printf("Resolving field ");
    debugPrintMethodFieldRef(jc, cp_field);
    printf("\n");
#endif // DEBUG

    cp_info* cpi;

    cpi = jc->constantPool + cp_field->Fieldref.class_index - 1;
    cpi = jc->constantPool + cpi->Class.name_index - 1;

    // Resolve the class the field belongs to
    if (!resolveClass(jvm, UTF8(cpi), outClass))
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
    printf("\nRunning method ");
    debugPrintMethod(jc, method);
#endif // DEBUG

    Frame* callerFrame = jvm->frames ? jvm->frames->frame : NULL;
    Frame* frame = newFrame(jc, method);

#ifdef DEBUG
    printf(", code len: %u, frame id: %d", frame->code_length, debugGetFrameId(frame));
    if (frame->code_length == 0)
        printf(" ### Native Method ###");
    printf("\n");
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
        frame->localVariables[numberOfParameters - parameterIndex - 1] = parameter;
    }

    if (method->access_flags & ACC_NATIVE)
    {
        cp_info* className = jc->constantPool + jc->thisClass - 1;
        className = jc->constantPool + className->Class.name_index - 1;

        cp_info* methodName = jc->constantPool + method->name_index - 1;
        cp_info* descriptor = jc->constantPool + method->descriptor_index - 1;

        NativeFunction native = getNative(UTF8(className), UTF8(methodName), UTF8(descriptor));

        if (native)
            native(jvm, frame, UTF8(descriptor));
    }
    else
    {
        InstructionFunction function;

        while (frame->pc < frame->code_length)
        {

#ifdef DEBUG
    printf("\n");
    debugPrintOperandStack(frame->operands);
    debugPrintLocalVariables(frame->localVariables, frame->max_locals);
#endif // DEBUG

            uint8_t opcode = *(frame->code + frame->pc++);
            function = fetchOpcodeFunction(opcode);

#ifdef DEBUG
    printf("   instruction '%s' at offset %u of frame %d\n", getOpcodeMnemonic(opcode), frame->pc - 1, debugGetFrameId(frame));
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
    }

    if (frame->returnCount > 0 && callerFrame)
    {
        // At most, two operands can be returned
        OperandStack parameters[2];
        uint8_t index;

        for (index = 0; index < frame->returnCount; index++)
            popOperand(&frame->operands, &parameters[index].value, &parameters[index].type);

        while (frame->returnCount-- > 0)
        {
            if (!pushOperand(&callerFrame->operands, parameters[frame->returnCount].value, parameters[frame->returnCount].type))
            {
                jvm->status = JVM_STATUS_OUT_OF_MEMORY;
                return 0;
            }
        }
    }

    popFrame(&jvm->frames, NULL);
    freeFrame(frame);

#ifdef DEBUG
    debugGetFrameId(NULL);
#endif // DEBUG

    return jvm->status == JVM_STATUS_OK;
}

uint8_t getMethodDescriptorParameterCount(const uint8_t* descriptor_utf8, int32_t utf8_len)
{
    uint8_t parameterCount = 0;

    while (utf8_len > 0)
    {
        switch (*descriptor_utf8)
        {
            case '(': break;
            case ')': return parameterCount;

            case 'J': case 'D':
                parameterCount += 2;
                break;

            case 'L':

                parameterCount++;

                do {
                    utf8_len--;
                    descriptor_utf8++;
                } while (utf8_len > 0 && *descriptor_utf8 != ';');

                break;

            case '[':

                parameterCount++;

                do {
                    utf8_len--;
                    descriptor_utf8++;
                } while (utf8_len > 0 && *descriptor_utf8 == '[');

                if (utf8_len > 0 && *descriptor_utf8 == 'L')
                {
                    do {
                        utf8_len--;
                        descriptor_utf8++;
                    } while (utf8_len > 0 && *descriptor_utf8 != ';');
                }

                break;

            case 'F': // float
            case 'B': // byte
            case 'C': // char
            case 'I': // int
            case 'S': // short
            case 'Z': // boolean
                parameterCount++;
                break;

            default:
                break;
        }

        descriptor_utf8++;
        utf8_len--;
    }

    return parameterCount;
}

LoadedClasses* addClassToLoadedClasses(JavaVirtualMachine* jvm, JavaClass* jc)
{
    LoadedClasses* node = (LoadedClasses*)malloc(sizeof(LoadedClasses));

    if (node)
    {
        node->jc = jc;
        node->staticFieldsData = NULL;
        node->requiresInit = 1;
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

        if (cmp_UTF8(UTF8(cpi), utf8_bytes, utf8_len))
            return classes;

        classes = classes->next;
    }

    return NULL;
}

JavaClass* getSuperClass(JavaVirtualMachine* jvm, JavaClass* jc)
{
    LoadedClasses* lc;
    cp_info* cp1;

    cp1 = jc->constantPool + jc->superClass - 1;
    cp1 = jc->constantPool + cp1->Class.name_index - 1;

    lc = isClassLoaded(jvm, UTF8(cp1));

    return lc ? lc->jc : NULL;
}

/// @pre Both \c super and \c jc must be classes that have already
/// been loaded.
/// @note If \c super and \c jc point to the same class, the function
/// returns false.
uint8_t isClassSuperOf(JavaVirtualMachine* jvm, JavaClass* super, JavaClass* jc)
{
    cp_info* cp1;
    cp_info* cp2;
    LoadedClasses* classes;

    cp2 = super->constantPool + super->thisClass - 1;
    cp2 = super->constantPool + cp2->Class.name_index - 1;

    while (jc && jc->superClass)
    {
        cp1 = jc->constantPool + jc->superClass - 1;
        cp1 = jc->constantPool + cp1->Class.name_index - 1;

        if (cmp_UTF8(UTF8(cp1), UTF8(cp2)))
            return 1;

        classes = isClassLoaded(jvm, UTF8(cp1));

        if (classes)
            jc = classes->jc;
        else
            break;
    }

    return 0;
}

uint8_t initClass(JavaVirtualMachine* jvm, LoadedClasses* lc)
{
    if (lc->requiresInit)
        lc->requiresInit = 0;
    else
        return 1;

    if (lc->jc->staticFieldCount > 0)
    {
        lc->staticFieldsData = (int32_t*)malloc(sizeof(int32_t) * lc->jc->staticFieldCount);

        uint16_t index;
        attribute_info* att;
        field_info* field;
        att_ConstantValue_info* cv;
        cp_info* cp;

        for (index = 0; index < lc->jc->fieldCount; index++)
        {
            field = lc->jc->fields + index;

            if (!(field->access_flags & ACC_STATIC))
                continue;

            att = getAttributeByType(field->attributes, field->attributes_count, ATTR_ConstantValue);

            if (!att)
                continue;

            cv = (att_ConstantValue_info*)att->info;

            if (!cv)
                continue;

            cp = lc->jc->constantPool + cv->constantvalue_index - 1;

            switch (cp->tag)
            {
                case CONSTANT_Integer: case CONSTANT_Float:
                    lc->staticFieldsData[field->offset] = cp->Integer.value;
                    break;

                case CONSTANT_Double: case CONSTANT_Long:
                    lc->staticFieldsData[field->offset] = cp->Long.high;
                    lc->staticFieldsData[field->offset + 1] = cp->Long.low;
                    break;

                case CONSTANT_String:
                    cp = lc->jc->constantPool + cp->String.string_index - 1;
                    lc->staticFieldsData[field->offset] = (int32_t)newString(jvm, UTF8(cp));
                    break;

                default:
                    break;
            }
        }
    }

    method_info* clinit = getMethodMatching(lc->jc, (uint8_t*)"<clinit>", 8, (uint8_t*)"()V", 3, ACC_STATIC);

    if (clinit && !runMethod(jvm, lc->jc, clinit, 0))
        return 0;

    return 1;
}

Reference* newString(JavaVirtualMachine* jvm, const uint8_t* str, int32_t strlen)
{
    Reference* r = (Reference*)malloc(sizeof(Reference));
    ReferenceTable* node = (ReferenceTable*)malloc(sizeof(ReferenceTable));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REFTYPE_STRING;
    r->str.len = strlen;

    if (strlen)
    {
        r->str.utf8_bytes = (uint8_t*)malloc(strlen);

        if (!r->str.utf8_bytes)
        {
            free(r);
            free(node);
            return NULL;
        }

        memcpy(r->str.utf8_bytes, str, strlen);
    }
    else
    {
        r->str.utf8_bytes = NULL;
    }

    node->next = jvm->objects;
    node->obj = r;
    jvm->objects = node;

#ifdef DEBUG
    debugPrintNewObject(r);
#endif // DEBUG

    return r;
}

Reference* newClassInstance(JavaVirtualMachine* jvm, LoadedClasses* lc)
{
    if (!initClass(jvm, lc))
        return 0;

    JavaClass* jc = lc->jc;
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

    if (jc->instanceFieldCount)
    {
        r->ci.data = (int32_t*)malloc(sizeof(int32_t) * jc->instanceFieldCount);

        if (!r->ci.data)
        {
            free(r);
            free(node);
            return NULL;
        }
    }
    else
    {
        r->ci.data = NULL;
    }

    node->next = jvm->objects;
    node->obj = r;
    jvm->objects = node;

#ifdef DEBUG
    debugPrintNewObject(r);
#endif // DEBUG

    return r;
}

Reference* newArray(JavaVirtualMachine* jvm, uint32_t length, Opcode_newarray_type type)
{
    size_t elementSize;

    switch (type)
    {
        case T_BOOLEAN:
        case T_BYTE:
            elementSize = sizeof(uint8_t);
            break;

        case T_SHORT:
        case T_CHAR:
            elementSize = sizeof(uint16_t);
            break;

        case T_FLOAT:
        case T_INT:
            elementSize = sizeof(uint32_t);
            break;

        case T_DOUBLE:
        case T_LONG:
            elementSize = sizeof(uint64_t);
            break;

        default:
            // Can't create array of other data type
            return NULL;
    }

    Reference* r = (Reference*)malloc(sizeof(Reference));
    ReferenceTable* node = (ReferenceTable*)malloc(sizeof(ReferenceTable));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REFTYPE_ARRAY;
    r->arr.length = length;
    r->arr.type = type;

    if (length > 0)
    {
        r->arr.data = (uint8_t*)malloc(elementSize * length);

        if (!r->arr.data)
        {
            free(r);
            free(node);
            return NULL;
        }

        uint8_t* data = (uint8_t*)r->arr.data;

        while (length-- > 0)
            *data++ = 0;
    }
    else
    {
        r->arr.data = NULL;
    }

    node->next = jvm->objects;
    node->obj = r;
    jvm->objects = node;

#ifdef DEBUG
    debugPrintNewObject(r);
#endif // DEBUG

    return r;
}

Reference* newObjectArray(JavaVirtualMachine* jvm, uint32_t length, const uint8_t* utf8_className, int32_t utf8_len)
{
    if (utf8_len <= 1)
        return NULL;

    switch (utf8_className[1])
    {
        case 'J': return newArray(jvm, length, T_LONG);
        case 'Z': return newArray(jvm, length, T_BOOLEAN);
        case 'B': return newArray(jvm, length, T_BYTE);
        case 'C': return newArray(jvm, length, T_CHAR);
        case 'S': return newArray(jvm, length, T_SHORT);
        case 'I': return newArray(jvm, length, T_INT);
        case 'F': return newArray(jvm, length, T_FLOAT);
        case 'D': return newArray(jvm, length, T_DOUBLE);
        default:
            break;
    }

    Reference* r = (Reference*)malloc(sizeof(Reference));
    ReferenceTable* node = (ReferenceTable*)malloc(sizeof(ReferenceTable));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REFTYPE_OBJARRAY;
    r->oar.length = length;
    r->oar.utf8_className = (uint8_t*)malloc(utf8_len);
    r->oar.utf8_len = utf8_len;

    if (!r->oar.utf8_className)
    {
        free(r);
        free(node);
        return NULL;
    }

    if (length > 0)
    {
        r->oar.elements = (Reference**)malloc(length * sizeof(Reference));

        if (!r->oar.elements)
        {
            free(r->oar.utf8_className);
            free(r);
            free(node);
            return NULL;
        }

        // Initializes all references to null
        while (length-- > 0)
            r->oar.elements[length] = NULL;

    }
    else
    {
        r->oar.elements = NULL;
    }

    // Copy class name
    while (utf8_len-- > 0)
        r->oar.utf8_className[utf8_len] = utf8_className[utf8_len];

    node->next = jvm->objects;
    node->obj = r;
    jvm->objects = node;

#ifdef DEBUG
    debugPrintNewObject(r);
#endif // DEBUG

    return r;
}

Reference* newObjectMultiArray(JavaVirtualMachine* jvm, int32_t* dimensions, uint8_t dimensionsSize,
                               const uint8_t* utf8_className, int32_t utf8_len)
{
    if (dimensionsSize == 0)
        return NULL;

    if (dimensionsSize == 1)
        return newObjectArray(jvm, dimensions[0], utf8_className, utf8_len);

    if (utf8_len <= 0)
        return NULL;

    Reference* r = (Reference*)malloc(sizeof(Reference));
    ReferenceTable* node = (ReferenceTable*)malloc(sizeof(ReferenceTable));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REFTYPE_OBJARRAY;
    r->oar.length = dimensions[0];
    r->oar.utf8_className = (uint8_t*)malloc(utf8_len);
    r->oar.utf8_len = utf8_len;

    if (!r->oar.utf8_className)
    {
        free(r);
        free(node);
        return NULL;
    }

    if (dimensions[0] > 0)
    {
        r->oar.elements = (Reference**)malloc(dimensions[0] * sizeof(Reference));

        if (!r->oar.elements)
        {
            free(r->oar.utf8_className);
            free(r);
            free(node);
            return NULL;
        }

        uint32_t dimensionLength = dimensions[0];

        // Initializes all references to subarrays
        while (dimensionLength-- > 0)
            r->oar.elements[dimensionLength] = newObjectMultiArray(jvm, dimensions + 1, dimensionsSize - 1, utf8_className + 1, utf8_len - 1);

    }
    else
    {
        r->oar.elements = NULL;
    }

    // Copy class name
    memcpy(r->oar.utf8_className, utf8_className, utf8_len);

    node->next = jvm->objects;
    node->obj = r;
    jvm->objects = node;

#ifdef DEBUG
    debugPrintNewObject(r);
#endif // DEBUG

    return r;
}

void deleteReference(Reference* obj)
{
    switch (obj->type)
    {
        case REFTYPE_STRING:
            if (obj->str.utf8_bytes)
                free(obj->str.utf8_bytes);
            break;

        case REFTYPE_ARRAY:
            if (obj->arr.data)
                free(obj->arr.data);
            break;

        case REFTYPE_CLASSINSTANCE:
            if (obj->ci.data)
                free(obj->ci.data);
            break;

        case REFTYPE_OBJARRAY:
        {
            free(obj->oar.utf8_className);

            if (obj->oar.elements)
                free(obj->oar.elements);

            // There is no need to delete the references created by this
            // object array, because they are all added to the created
            // objects table inside the JVM, which will be deleted later.

            break;
        }

        default:
            return;
    }

    free(obj);
}
