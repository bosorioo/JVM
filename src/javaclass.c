#include "readfunctions.h"
#include "javaclass.h"
#include "constantpool.h"
#include "utf8.h"
#include "validity.h"
#include "memoryinspect.h"

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

    jc->thisClass = jc->superClass = jc->accessFlags = 0;
    jc->attributeCount = jc->fieldCount = jc->methodCount = jc->constantPoolCount = jc->interfaceCount = 0;

    jc->lastTagRead = 0;
    jc->totalBytesRead = 0;
    jc->constantPoolEntriesRead = 0;
    jc->attributeEntriesRead = 0;
    jc->currentConstantPoolEntryIndex = -1;
    jc->currentInterfaceEntryIndex = -1;
    jc->currentFieldEntryIndex = -1;
    jc->currentMethodEntryIndex = -1;
    jc->currentAttributeEntryIndex = -1;
    jc->currentValidityEntryIndex = -1;

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
                return;

            if (jc->constantPool[u16].tag == CONSTANT_Double ||
                jc->constantPool[u16].tag == CONSTANT_Long)
            {
                u16++;
                jc->currentConstantPoolEntryIndex++;
            }

            jc->currentConstantPoolEntryIndex++;
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

    if (!checkClassIndexAndAccessFlags(jc) || !checkClassNameFileNameMatch(jc, path))
        return;

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
            jc->currentInterfaceEntryIndex++;
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
            if (!readField(jc, jc->fields + u32))
                return;

            jc->currentFieldEntryIndex++;
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

        if (!jc->methodCount)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->methodCount; u32++)
        {
            if (!readMethod(jc, jc->methods + u32))
                return;

            jc->currentMethodEntryIndex++;
        }
    }

    if (!readu2(jc, &jc->attributeCount))
    {
        jc->status = UNEXPECTED_EOF;
        return;
    }

    if (jc->attributeCount > 0)
    {
        jc->attributes = (attribute_info*)malloc(sizeof(attribute_info) * jc->attributeCount);

        if (!jc->attributes)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        jc->currentAttributeEntryIndex = -1;

        for (u32 = 0; u32 < jc->attributeCount; u32++)
        {
            if (!readAttribute(jc, jc->attributes +  u32))
                return;

            jc->attributeEntriesRead++;
            jc->currentAttributeEntryIndex++;
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

void closeClassFile(JavaClass* jc)
{
    if (!jc)
        return;

    uint16_t i, j;

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
        // We have to check if the status is OK, because if it is not OK,
        // only a few method_info would have been correctly initialized.
        // If a "free" attempt was made on an attribute that wasn't initialized,
        // the program would crash.
        if (jc->status != CLASS_STATUS_OK)
            j = jc->currentMethodEntryIndex + 2;
        else
            j = jc->methodCount;

        for (i = 0; i < j; i++)
            freeMethodAttributes(jc->methods + i);

        free(jc->methods);
        jc->constantPoolCount = 0;
    }

    if (jc->fields)
    {
        if (jc->status != CLASS_STATUS_OK)
            j = jc->currentFieldEntryIndex + 2;
        else
            j = jc->fieldCount;

        for (i = 0; i < j; i++)
            freeFieldAttributes(jc->fields + i);

        free(jc->fields);
        jc->fieldCount = 0;
    }

    if (jc->attributes)
    {
        for (i = 0; i < jc->attributeEntriesRead; i++)
            freeAttributeInfo(jc->attributes + i);

        free(jc->attributes);
        jc->attributeCount = 0;
    }
}

const char* decodeJavaClassStatus(enum JavaClassStatus status)
{
    switch (status)
    {
        case CLASS_STATUS_OK: return "file is ok";
        case CLASS_STATUS_UNSUPPORTED_VERSION: return "class file version isn't supported";
        case CLASS_STATUS_FILE_COULDNT_BE_OPENED: return "class file couldn't be opened";
        case CLASS_STATUS_INVALID_SIGNATURE: return "signature (0xCAFEBABE) mismatch";
        case CLASS_NAME_FILE_NAME_MISMATCH: return "class name and .class file name don't match";
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

void decodeAccessFlags(uint16_t flags, char* buffer, int32_t buffer_len, enum AccessFlagsType acctype)
{
    uint32_t bytes = 0;
    char* buffer_ptr = buffer;
    const char* comma = ", ";
    const char* empty = "";

    #define DECODE_FLAG(flag, name) if (flags & flag) { \
        bytes = snprintf(buffer, buffer_len, "%s%s", bytes ? comma : empty, name); \
        buffer += bytes; \
        buffer_len -= bytes; }

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

void printClassFileDebugInfo(JavaClass* jc)
{
    if (jc->currentConstantPoolEntryIndex + 2 != jc->constantPoolCount)
    {
        printf("Failed to read constant pool entry at index #%d\n", jc->currentConstantPoolEntryIndex + 1);
        printf("Constant pool count: %u, constants successfully read: %d\n", jc->constantPoolCount, jc->constantPoolEntriesRead);
    }
    else if (jc->currentValidityEntryIndex != -1)
    {
        printf("Failed to check constant pool validity. Entry at index #%d is not valid.\n", jc->currentValidityEntryIndex + 1);
    }
    else if (jc->currentInterfaceEntryIndex + 1 != jc->interfaceCount)
    {
        printf("Failed to read interface at index #%d\n", jc->currentInterfaceEntryIndex + 1);
        printf("Interface count: %u, interfaces successfully read: %d\n", jc->interfaceCount, 1 + jc->currentInterfaceEntryIndex);
    }
    else if (jc->currentFieldEntryIndex + 1 != jc->fieldCount)
    {
        printf("Failed to read field at index #%d\n", jc->currentFieldEntryIndex + 1);
        printf("Last attribute index read: %d\n", jc->currentAttributeEntryIndex);
        printf("Fields count: %u, fields successfully read: %d\n", jc->fieldCount, 1 + jc->currentFieldEntryIndex);
    }
    else if (jc->currentMethodEntryIndex + 1 != jc->methodCount)
    {
        printf("Failed to read method at index #%d\n", jc->currentMethodEntryIndex + 1);

        if (jc->currentAttributeEntryIndex > -2)
        {
            printf("Couldn't read attribute at index: %d\n", jc->currentAttributeEntryIndex + 1);
        }
        printf("Methods count: %u, methods successfully read: %d\n", jc->methodCount, 1 + jc->currentMethodEntryIndex);
    }
    else if (jc->attributeEntriesRead != jc->attributeCount)
    {
        printf("Failed to read attribute at index #%d\n", jc->currentAttributeEntryIndex + 1);
        printf("Attributes count: %u, attributes successfully read: %d\n", jc->attributeCount, jc->attributeEntriesRead);
    }

    printf("File status code: %d\nStatus description: %s.\n", jc->status, decodeJavaClassStatus(jc->status));
    printf("Number of bytes read: %d\n", jc->totalBytesRead);
}

void printClassFileInfo(JavaClass* jc)
{
    char buffer[256];
    cp_info* cp;
    uint16_t u16;

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
