#include "readfunctions.h"
#include "javaclass.h"
#include "constantpool.h"
#include "utf8.h"
#include "validity.h"
#include "debugging.h"

/// @brief Opens a class file and parse it, storing the class
/// information in the JavaClass structure.
/// @param JavaClass* jc - pointer to the structure that will
/// hold the class data
/// @param const char* path - string containing the path to the
/// class file to be read
///
/// This function opens the .class file and reads its content
/// from the file by filling in the fields of the JavaClass struct.
/// A bunch of other functions from different modules are called
/// to read some pieces of the class file. Various checks are made
/// during the parsing of the read data.
///
/// @see closeClassFile, printClassFileInfo(), printClassFileDebugInfo()
/// @hidecallergraph
void openClassFile(JavaClass* jc, const char* path)
{
    if (!jc)
        return;

    uint32_t u32;
    uint16_t u16;

    jc->file = fopen(path, "rb");
    jc->minorVersion = jc->majorVersion = jc->constantPoolCount = 0;
    jc->constantPool = NULL;
    jc->interfaces = NULL;
    jc->fields = NULL;
    jc->methods = NULL;
    jc->attributes = NULL;
    jc->status = CLASS_STATUS_OK;
    jc->classNameMismatch = 0;

    jc->thisClass = jc->superClass = jc->accessFlags = 0;
    jc->attributeCount = jc->fieldCount = jc->methodCount = jc->constantPoolCount = jc->interfaceCount = 0;

    jc->staticFieldCount = 0;
    jc->instanceFieldCount = 0;

    jc->lastTagRead = 0;
    jc->totalBytesRead = 0;
    jc->constantPoolEntriesRead = 0;
    jc->attributeEntriesRead = 0;
    jc->constantPoolEntriesRead = 0;
    jc->interfaceEntriesRead = 0;
    jc->fieldEntriesRead = 0;
    jc->methodEntriesRead = 0;
    jc->validityEntriesChecked = 0;

    if (!jc->file)
    {
        jc->status = CLASS_STATUS_FILE_COULDNT_BE_OPENED;
        return;
    }

    if (!readu4(jc, &u32) || u32 != 0xCAFEBABE)
    {
        jc->status = CLASS_STATUS_INVALID_SIGNATURE;
        return;
    }

    if (!readu2(jc, &jc->minorVersion) ||
        !readu2(jc, &jc->majorVersion) ||
        !readu2(jc, &jc->constantPoolCount))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (jc->majorVersion < 45 || jc->majorVersion > 52)
    {
        jc->status = CLASS_STATUS_UNSUPPORTED_VERSION;
        return;
    }

    if (jc->constantPoolCount == 0)
    {
        jc->status = INVALID_CONSTANT_POOL_COUNT;
        return;
    }

    if (jc->constantPoolCount > 1)
    {
        jc->constantPool = (cp_info*)malloc(sizeof(cp_info) * (jc->constantPoolCount - 1));

        if (!jc->constantPool)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u16 = 0; u16 < jc->constantPoolCount - 1; u16++)
        {
            if (!readConstantPoolEntry(jc, jc->constantPool + u16))
            {
                jc->constantPoolCount = u16 + 1;
                return;
            }

            if (jc->constantPool[u16].tag == CONSTANT_Double ||
                jc->constantPool[u16].tag == CONSTANT_Long)
            {
                u16++;
            }

            jc->constantPoolEntriesRead++;
        }

        if (!checkConstantPoolValidity(jc))
            return;
    }

    if (!readu2(jc, &jc->accessFlags) ||
        !readu2(jc, &jc->thisClass) ||
        !readu2(jc, &jc->superClass))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (!checkClassIndexAndAccessFlags(jc))
        return;

    if (!checkClassNameFileNameMatch(jc, path))
        jc->classNameMismatch = 1;

    if (!readu2(jc, &jc->interfaceCount))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (jc->interfaceCount > 0)
    {
        jc->interfaces = (uint16_t*)malloc(sizeof(uint16_t) * jc->interfaceCount);

        if (!jc->interfaces)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->interfaceCount; u32++)
        {
            if (!readu2(jc, &u16))
            {
                jc->status = UNEXPECTED_EOF_READING_INTERFACES;
                return;
            }

            if (u16 == 0 || jc->constantPool[u16 - 1].tag != CONSTANT_Class)
            {
                jc->status = INVALID_INTERFACE_INDEX;
                return;
            }

            *(jc->interfaces + u32) = u16;
            jc->interfaceEntriesRead++;
        }
    }

    if (!readu2(jc, &jc->fieldCount))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (jc->fieldCount > 0)
    {
        jc->fields = (field_info*)malloc(sizeof(field_info) * jc->fieldCount);

        if (!jc->fields)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->fieldCount; u32++)
        {
            field_info* field = jc->fields + u32;
            uint8_t isCat2;

            if (!readField(jc, field))
            {
                jc->fieldCount = u32 + 1;
                return;
            }

            isCat2 = *jc->constantPool[field->descriptor_index - 1].Utf8.bytes;
            isCat2 = isCat2 == 'J' || isCat2 == 'D';

            if (field->access_flags & ACC_STATIC)
            {
                field->offset = jc->staticFieldCount++;
                jc->staticFieldCount += isCat2;
            }
            else
            {
                field->offset = jc->instanceFieldCount++;
                jc->instanceFieldCount += isCat2;
            }


            jc->fieldEntriesRead++;
        }
    }

    if (!readu2(jc, &jc->methodCount))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (jc->methodCount > 0)
    {
        jc->methods = (method_info*)malloc(sizeof(method_info) * jc->methodCount);

        if (!jc->methods)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->methodCount; u32++)
        {
            if (!readMethod(jc, jc->methods + u32))
            {
                jc->methodCount = u32 + 1;
                return;
            }

            jc->methodEntriesRead++;
        }
    }

    if (!readu2(jc, &jc->attributeCount))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (jc->attributeCount > 0)
    {
        jc->attributeEntriesRead = 0;
        jc->attributes = (attribute_info*)malloc(sizeof(attribute_info) * jc->attributeCount);

        if (!jc->attributes)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->attributeCount; u32++)
        {
            if (!readAttribute(jc, jc->attributes + u32))
            {
                jc->attributeCount = u32 + 1;
                return;
            }

            jc->attributeEntriesRead++;
        }
    }

    if (fgetc(jc->file) != EOF)
    {
        jc->status = FILE_CONTAINS_UNEXPECTED_DATA;
    }
    else
    {
        fclose(jc->file);
        jc->file = NULL;
    }

}

/// @brief Closes the .class file and releases resources used by
/// the JavaClass struct.
/// @param JavaClass* jc - pointer to the class to be closed.
/// @note This function does not free the @b *jc pointer.
/// @see openClassFile()
/// @hidecallergraph
void closeClassFile(JavaClass* jc)
{
    if (!jc)
        return;

    uint16_t i;

    if (jc->file)
    {
        fclose(jc->file);
        jc->file = NULL;
    }

    if (jc->interfaces)
    {
        free(jc->interfaces);
        jc->interfaces = NULL;
        jc->interfaceCount = 0;
    }

    if (jc->constantPool)
    {
        for (i = 0; i < jc->constantPoolCount - 1; i++)
        {
            if (jc->constantPool[i].tag == CONSTANT_Utf8 && jc->constantPool[i].Utf8.bytes)
                free(jc->constantPool[i].Utf8.bytes);
        }

        free(jc->constantPool);
        jc->constantPool = NULL;
        jc->constantPoolCount = 0;
    }

    if (jc->methods)
    {
        for (i = 0; i < jc->methodCount; i++)
            freeMethodAttributes(jc->methods + i);

        free(jc->methods);
        jc->methodCount = 0;
    }

    if (jc->fields)
    {
        for (i = 0; i < jc->fieldCount; i++)
            freeFieldAttributes(jc->fields + i);

        free(jc->fields);
        jc->fieldCount = 0;
    }

    if (jc->attributes)
    {
        for (i = 0; i < jc->attributeCount; i++)
            freeAttributeInfo(jc->attributes + i);

        free(jc->attributes);
        jc->attributeCount = 0;
    }
}

/// @brief Decodes JavaClassStatus enumeration elements
///
/// @param enum JavaClassStatus status - identifier of the enumeration to be
/// translated.
///
/// @return const char* - pointer to char containing the decoded status
const char* decodeJavaClassStatus(enum JavaClassStatus status)
{
    switch (status)
    {
        case CLASS_STATUS_OK: return "file is ok";
        case CLASS_STATUS_UNSUPPORTED_VERSION: return "class file version isn't supported";
        case CLASS_STATUS_FILE_COULDNT_BE_OPENED: return "class file couldn't be opened";
        case CLASS_STATUS_INVALID_SIGNATURE: return "signature (0xCAFEBABE) mismatch";
        case MEMORY_ALLOCATION_FAILED: return "not enough memory";
        case INVALID_CONSTANT_POOL_COUNT: return "constant pool count should be at least 1";
        case UNEXPECTED_EOF: return "end of file found too soon";
        case UNEXPECTED_EOF_READING_CONSTANT_POOL: return "unexpected end of file while reading constant pool";
        case UNEXPECTED_EOF_READING_UTF8: return "unexpected end of file while reading UTF-8 stream";
        case UNEXPECTED_EOF_READING_INTERFACES: return "unexpected end of file while reading interfaces' index";
        case UNEXPECTED_EOF_READING_ATTRIBUTE_INFO: return "unexpected end of file while reading attribute information";
        case INVALID_UTF8_BYTES: return "invalid UTF-8 encoding";
        case INVALID_CONSTANT_POOL_INDEX: return "invalid index for constant pool entry";
        case UNKNOWN_CONSTANT_POOL_TAG: return "unknown constant pool entry tag";
        case INVALID_ACCESS_FLAGS: return "invalid combination of access flags";

        case USE_OF_RESERVED_CLASS_ACCESS_FLAGS: return "class access flags contains bits that should be zero";
        case USE_OF_RESERVED_METHOD_ACCESS_FLAGS: return "method access flags contains bits that should be zero";
        case USE_OF_RESERVED_FIELD_ACCESS_FLAGS: return "field access flags contains bits that should be zero";

        case INVALID_THIS_CLASS_INDEX: return "\"this Class\" field doesn't point to valid class index";
        case INVALID_SUPER_CLASS_INDEX: return "\"super class\" field doesn't point to valid class index";
        case INVALID_INTERFACE_INDEX: return "interface index doesn't point to a valid class index";
        case INVALID_FIELD_DESCRIPTOR_INDEX: return "field descriptor isn't valid";
        case INVALID_METHOD_DESCRIPTOR_INDEX: return "method descriptor isn't valid";
        case INVALID_NAME_INDEX: return "name_index doesn't point to a valid name";
        case INVALID_STRING_INDEX: return "string_index doesn't point to a valid UTF-8 stream";
        case INVALID_CLASS_INDEX: return "class_index doesn't point to a valid class name";
        case INVALID_NAME_AND_TYPE_INDEX: return "(NameAndType) name_index or descriptor_index doesn't point to a valid UTF-8 stream";
        case INVALID_JAVA_IDENTIFIER: return "UTF-8 stream isn't a valid Java identifier";

        case ATTRIBUTE_LENGTH_MISMATCH: return "Attribute length is different than expected";
        case ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX: return "constantvalue_index isn't a valid constant value index";
        case ATTRIBUTE_INVALID_SOURCEFILE_INDEX: return "sourcefile_index isn't a valid source file index";
        case ATTRIBUTE_INVALID_INNERCLASS_INDEXES: return "InnerClass has at least one invalid index";
        case ATTRIBUTE_INVALID_EXCEPTIONS_CLASS_INDEX: return "Exceptions has an index that doesn't point to a valid class";
        case ATTRIBUTE_INVALID_CODE_LENGTH: return "Attribute code must have a length greater than 0 and less than 65536 bytes";

        case FILE_CONTAINS_UNEXPECTED_DATA: return "class file contains more data than expected, which wasn't processed";

        default:
            break;
    }

    return "unknown status";
}

/// @brief Given flags of a certain method/field/class, this class will convert
/// the bits to a string telling which flags are set to 1, writing the result in the
/// output buffer.
///
/// @param uint16_t flags - the flags of the method/field/class
/// @param [out] char* buffer - pointer to where the string will be stored
/// @param int32_t buffer_len - length of the buffer
/// @param enum AccessFlagsType acctype - type of access flags, telling if it is a class,
/// a method or a field.
///
/// This method will write at most \b buffer_len characters to the output buffer.
/// Depending on parameter \b acctype, meaning for some bits might change.
void decodeAccessFlags(uint16_t flags, char* buffer, int32_t buffer_len, enum AccessFlagsType acctype)
{
    uint32_t bytes = 0;
    char* buffer_ptr = buffer;
    const char* comma = ", ";
    const char* empty = "";

    /// @cond
    /// No need to document this macro
    #define DECODE_FLAG(flag, name) if (flags & flag) { \
        bytes = snprintf(buffer, buffer_len, "%s%s", bytes ? comma : empty, name); \
        buffer += bytes; \
        buffer_len -= bytes; }
    /// @endcod

    if (acctype == ACCT_CLASS)
    {
        DECODE_FLAG(ACC_SUPER, "super")
    }

    if (acctype == ACCT_CLASS || acctype == ACCT_METHOD || acctype == ACCT_INNERCLASS)
    {
        DECODE_FLAG(ACC_ABSTRACT, "abstract")
    }

    if (acctype == ACCT_METHOD)
    {
        DECODE_FLAG(ACC_SYNCHRONIZED, "synchronized")
        DECODE_FLAG(ACC_NATIVE, "native")
        DECODE_FLAG(ACC_STRICT, "strict")
    }

    if (acctype == ACCT_FIELD)
    {
        DECODE_FLAG(ACC_TRANSIENT, "transient")
        DECODE_FLAG(ACC_VOLATILE, "volatile")
    }

    DECODE_FLAG(ACC_PUBLIC, "public")

    if (acctype == ACCT_FIELD || acctype == ACCT_METHOD || acctype == ACCT_INNERCLASS)
    {
        DECODE_FLAG(ACC_PRIVATE, "private")
        DECODE_FLAG(ACC_PROTECTED, "protected")
    }

    if (acctype == ACCT_FIELD || acctype == ACCT_METHOD || acctype == ACCT_INNERCLASS)
    {
        DECODE_FLAG(ACC_STATIC, "static")
    }

    DECODE_FLAG(ACC_FINAL, "final")

    if (acctype == ACCT_CLASS || acctype == ACCT_INNERCLASS)
    {
        DECODE_FLAG(ACC_INTERFACE, "interface")
    }

    // If no flags were written
    if (buffer == buffer_ptr)
        snprintf(buffer, buffer_len, "no flags");

    #undef DECODE_FLAG
}

/// @brief Prints class file debug information
///
/// @param JavaClass* jc, pointer to the structure to be
/// debugged.
///
/// This function is called If something went wrong, if
/// the jc.status != CLASS_STATUS_OK,shows debug information
void printClassFileDebugInfo(JavaClass* jc)
{
    if (jc->classNameMismatch)
        printf("Warning: class name and file path don't match.\n");

    if (jc->constantPoolEntriesRead != jc->constantPoolCount)
    {
        printf("Failed to read constant pool entry at index #%d\n", jc->constantPoolEntriesRead);
    }
    else if (jc->validityEntriesChecked != jc->constantPoolCount)
    {
        printf("Failed to check constant pool validity. Entry at index #%d is not valid.\n", jc->validityEntriesChecked);
    }
    else if (jc->interfaceEntriesRead != jc->interfaceCount)
    {
        printf("Failed to read interface at index #%d\n", jc->interfaceEntriesRead);
    }
    else if (jc->fieldEntriesRead != jc->fieldCount)
    {
        printf("Failed to read field at index #%d\n", jc->fieldEntriesRead);

        if (jc->attributeEntriesRead != -1)
            printf("Failed to read its attribute at index %d.\n", jc->attributeEntriesRead);
    }
    else if (jc->methodEntriesRead != jc->methodCount)
    {
        printf("Failed to read method at index #%d\n", jc->methodEntriesRead);

        if (jc->attributeEntriesRead != -1)
            printf("Failed to read its attribute at index %d.\n", jc->attributeEntriesRead);
    }
    else if (jc->attributeEntriesRead != jc->attributeCount)
    {
        printf("Failed to read attribute at index #%d\n", jc->attributeEntriesRead);

    }

    printf("File status code: %d\nStatus description: %s.\n", jc->status, decodeJavaClassStatus(jc->status));
    printf("Number of bytes read: %d\n", jc->totalBytesRead);
}

/// @brief Prints class file information
///
/// @param JavaClass* jc, pointer to the structure analyzed
///
/// This function is called for print all of information of .class file
void printClassFileInfo(JavaClass* jc)
{
    char buffer[256];
    cp_info* cp;
    uint16_t u16;

    if (jc->classNameMismatch)
        printf("---- Warning ----\n\nClass name and file path don't match.\nReading will proceed anyway.\n\n");

    printf("---- General Information ----\n\n");

    printf("Version:\t\t%u.%u (Major.minor)", jc->majorVersion, jc->minorVersion);

    if (jc->majorVersion >= 45 && jc->majorVersion <= 52)
        printf(" [jdk version 1.%d]", jc->majorVersion - 44);

    printf("\nConstant pool count:\t%u\n", jc->constantPoolCount);

    decodeAccessFlags(jc->accessFlags, buffer, sizeof(buffer), ACCT_CLASS);
    printf("Access flags:\t\t0x%.4X [%s]\n", jc->accessFlags, buffer);

    cp = jc->constantPool + jc->thisClass - 1;
    cp = jc->constantPool + cp->Class.name_index - 1;
    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
    printf("This class:\t\tcp index #%u <%s>\n", jc->thisClass, buffer);

    cp = jc->constantPool + jc->superClass - 1;
    cp = jc->constantPool + cp->Class.name_index - 1;
    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
    printf("Super class:\t\tcp index #%u <%s>\n", jc->superClass, buffer);

    printf("Interfaces count:\t%u\n", jc->interfaceCount);
    printf("Fields count:\t\t%u\n", jc->fieldCount);
    printf("Methods count:\t\t%u\n", jc->methodCount);
    printf("Attributes count:\t%u\n", jc->attributeCount);

    printConstantPool(jc);

    if (jc->interfaceCount > 0)
    {
        printf("\n---- Interfaces implemented by the class ----\n\n");

        for (u16 = 0; u16 < jc->interfaceCount; u16++)
        {
            cp = jc->constantPool + *(jc->interfaces + u16) - 1;
            cp = jc->constantPool + cp->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("\tInterface #%u: #%u <%s>\n", u16 + 1, *(jc->interfaces + u16), buffer);
        }
    }

    printAllFields(jc);
    printMethods(jc);
    printAllAttributes(jc);
}
