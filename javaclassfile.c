#include "readfunctions.h"
#include "javaclassfile.h"
#include "constantpool.h"
#include "utf8.h"
#include "validity.h"
#include <stdlib.h>

void openClassFile(JavaClassFile* jcf, const char* path)
{
    if (!jcf)
        return;

    uint32_t u32;
    uint16_t u16;

    jcf->file = fopen(path, "rb");
    jcf->minorVersion = jcf->majorVersion = jcf->constantPoolCount = 0;
    jcf->constantPool = NULL;
    jcf->interfaces = NULL;
    jcf->fields = NULL;
    jcf->methods = NULL;
    jcf->attributes = NULL;
    jcf->status = STATUS_OK;

    jcf->thisClass = jcf->superClass = jcf->accessFlags = 0;
    jcf->attributeCount = jcf->fieldCount = jcf->methodCount = jcf->constantPoolCount = jcf->interfaceCount = 0;

    jcf->lastTagRead = 0;
    jcf->totalBytesRead = 0;
    jcf->constantPoolEntriesRead = 0;
    jcf->currentConstantPoolEntryIndex = -1;
    jcf->currentInterfaceEntryIndex = -1;
    jcf->currentFieldEntryIndex = -1;
    jcf->currentMethodEntryIndex = -1;
    jcf->currentAttributeEntryIndex = -1;

    if (!jcf->file)
    {
        jcf->status = FILE_COULDNT_BE_OPENED;
        return;
    }

    if (!readu4(jcf, &u32) || u32 != 0xCAFEBABE)
    {
        jcf->status = INVALID_SIGNATURE;
        return;
    }

    if (!readu2(jcf, &jcf->minorVersion) ||
        !readu2(jcf, &jcf->majorVersion) ||
        !readu2(jcf, &jcf->constantPoolCount))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    // TODO: assert version

    if (jcf->constantPoolCount == 0)
    {
        jcf->status = INVALID_CONSTANT_POOL_COUNT;
        return;
    }

    if (jcf->constantPoolCount > 1)
    {
        jcf->constantPool = (cp_info*)malloc(sizeof(cp_info) * (jcf->constantPoolCount - 1));

        if (!jcf->constantPool)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u16 = 0; u16 < jcf->constantPoolCount - 1; u16++)
        {
            if (!readConstantPoolEntry(jcf, jcf->constantPool + u16))
                return;

            if (jcf->constantPool[u16].tag == CONSTANT_Double ||
                jcf->constantPool[u16].tag == CONSTANT_Long)
            {
                u16++;
                jcf->currentConstantPoolEntryIndex++;
            }

            jcf->currentConstantPoolEntryIndex++;
            jcf->constantPoolEntriesRead++;
        }

        if (!checkConstantPoolValidity(jcf))
            return;
    }

    if (!readu2(jcf, &jcf->accessFlags) ||
        !readu2(jcf, &jcf->thisClass) ||
        !readu2(jcf, &jcf->superClass))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    if (!checkClassIndexAndAccessFlags(jcf) || !checkClassNameFileNameMatch(jcf, path))
        return;

    if (!readu2(jcf, &jcf->interfaceCount))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    if (jcf->interfaceCount > 0)
    {
        jcf->interfaces = (uint16_t*)malloc(sizeof(uint16_t) * jcf->interfaceCount);

        if (!jcf->interfaces)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jcf->interfaceCount; u32++)
        {
            if (!readu2(jcf, &u16))
            {
                jcf->status = UNEXPECTED_EOF_READING_INTERFACES;
                return;
            }

            if (u16 == 0 || jcf->constantPool[u16 - 1].tag != CONSTANT_Class)
            {
                jcf->status = INVALID_INTERFACE_INDEX;
                return;
            }

            *(jcf->interfaces + u32) = u16;
            jcf->currentInterfaceEntryIndex++;
        }
    }

    if (!readu2(jcf, &jcf->fieldCount))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    if (jcf->fieldCount > 0)
    {
        jcf->fields = (field_info*)malloc(sizeof(field_info) * jcf->fieldCount);

        if (!jcf->fields)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jcf->fieldCount; u32++)
        {
            if (!readField(jcf, jcf->fields + u32))
                return;

            jcf->currentFieldEntryIndex++;
        }
    }

    if (!readu2(jcf, &jcf->methodCount))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    if (jcf->methodCount > 0)
    {
        jcf->methods = (method_info*)malloc(sizeof(method_info) * jcf->methodCount);

        if (!jcf->methodCount)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        for (u32 = 0; u32 < jcf->methodCount; u32++)
        {
            if (!readMethod(jcf, jcf->methods + u32))
                return;

            jcf->currentMethodEntryIndex++;
        }
    }

    if (!readu2(jcf, &jcf->attributeCount))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    if (jcf->attributeCount > 0)
    {
        jcf->attributes = (attribute_info*)malloc(sizeof(attribute_info) * jcf->attributeCount);

        if (!jcf->attributes)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return;
        }

        jcf->currentAttributeEntryIndex = -1;

        for (u32 = 0; u32 < jcf->attributeCount; u32++)
        {
            if (!readAttribute(jcf, jcf->attributes +  u32))
                return;

            jcf->currentAttributeEntryIndex++;
        }
    }

    if (fgetc(jcf->file) != EOF)
    {
        jcf->status = FILE_CONTAINS_UNEXPECTED_DATA;
    }
    else
    {
        fclose(jcf->file);
        jcf->file = NULL;
    }
}

void closeClassFile(JavaClassFile* jcf)
{
    if (!jcf)
        return;

    uint16_t i, j;

    if (jcf->file)
    {
        fclose(jcf->file);
        jcf->file = NULL;
    }

    if (jcf->interfaces)
    {
        free(jcf->interfaces);
        jcf->interfaces = NULL;
        jcf->interfaceCount = 0;
    }

    if (jcf->constantPool)
    {
        for (i = 0; i < jcf->constantPoolCount - 1; i++)
        {
            if (jcf->constantPool[i].tag == CONSTANT_Utf8 && jcf->constantPool[i].Utf8.bytes)
                free(jcf->constantPool[i].Utf8.bytes);
        }

        free(jcf->constantPool);
        jcf->constantPool = NULL;
        jcf->constantPoolCount = 0;
    }

    if (jcf->methods)
    {
        // We have to check if the status is OK, because if it is not OK,
        // only a few method_info would have been correctly initialized.
        // If a "free" attempt was made on an attribute that wasn't initialized,
        // the program would crash.
        if (jcf->status != STATUS_OK)
            j = jcf->currentMethodEntryIndex + 1;
        else
            j = jcf->methodCount;

        for (i = 0; i < j; i++)
            freeMethodAttributes(jcf->methods + i);

        free(jcf->methods);
        jcf->constantPoolCount = 0;
    }

    if (jcf->attributes)
    {
        for (i = 0; i < jcf->attributeCount; i++)
            freeAttributeInfo(jcf->attributes + i);

        free(jcf->attributes);
        jcf->attributeCount = 0;
    }
}

const char* decodeJavaClassFileStatus(enum JavaClassStatus status)
{
    switch (status)
    {
        case STATUS_OK: return "file is ok";
        case FILE_COULDNT_BE_OPENED: return "class file couldn't be opened";
        case INVALID_SIGNATURE: return "signature (0xCAFEBABE) mismatch";
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
        case INVALID_ACCESS_FLAGS: return "invalid combination of class access flags";

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
        case FILE_CONTAINS_UNEXPECTED_DATA: return "class file contains more data than expected, which wasn't processed";

        default:
            break;
    }

    return "Unknown status";
}

void decodeAccessFlags(uint32_t flags, char* buffer, int32_t buffer_len, enum AccessFlagsType acctype)
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
        DECODE_FLAG(ACC_INTERFACE, "interface")
        DECODE_FLAG(ACC_SUPER, "super")
    }

    if (acctype == ACCT_METHOD)
    {
        DECODE_FLAG(ACC_SYNCHRONIZED, "synchronized")
        DECODE_FLAG(ACC_NATIVE, "native")
        DECODE_FLAG(ACC_STRICT, "strict")
    }

    if (acctype == ACCT_CLASS || acctype == ACCT_METHOD)
    {
        DECODE_FLAG(ACC_ABSTRACT, "abstract")
    }

    if (acctype == ACCT_FIELD)
    {
        DECODE_FLAG(ACC_TRANSIENT, "transient")
        DECODE_FLAG(ACC_VOLATILE, "volatile")
    }

    DECODE_FLAG(ACC_FINAL, "final")

    if (acctype == ACCT_FIELD || acctype == ACCT_METHOD)
    {
        DECODE_FLAG(ACC_STATIC, "static")
    }

    DECODE_FLAG(ACC_PUBLIC, "public")

    if (acctype == ACCT_FIELD || acctype == ACCT_METHOD)
    {
        DECODE_FLAG(ACC_PRIVATE, "private")
        DECODE_FLAG(ACC_PROTECTED, "protected")
    }

    // If no flags were written
    if (buffer == buffer_ptr)
        snprintf(buffer, buffer_len, "no flags");

    #undef DECODE_FLAG
}

void printClassFileDebugInfo(JavaClassFile* jcf)
{
    if (jcf->currentConstantPoolEntryIndex + 2 != jcf->constantPoolCount)
    {
        printf("Failed to read constant pool entry at index #%d\n", jcf->currentConstantPoolEntryIndex + 1);
        printf("Constant pool count: %u, constants successfully read: %d\n", jcf->constantPoolCount, jcf->constantPoolEntriesRead);
    }
    else if (jcf->currentInterfaceEntryIndex + 1 != jcf->interfaceCount)
    {
        printf("Failed to read interface at index #%d\n", jcf->currentInterfaceEntryIndex + 1);
        printf("Interface count: %u, interfaces successfully read: %d\n", jcf->interfaceCount, 1 + jcf->currentInterfaceEntryIndex);
    }
    else if (jcf->currentFieldEntryIndex + 1 != jcf->fieldCount)
    {
        printf("Failed to read field at index #%d\n", jcf->currentFieldEntryIndex + 1);
        printf("Last attribute index read: %d\n", jcf->currentAttributeEntryIndex);
        printf("Fields count: %u, fields successfully read: %d\n", jcf->fieldCount, 1 + jcf->currentFieldEntryIndex);
    }
    else if (jcf->currentMethodEntryIndex + 1 != jcf->methodCount)
    {
        printf("Failed to read method at index #%d\n", jcf->currentMethodEntryIndex + 1);
        printf("Last attribute index read: %d\n", jcf->currentAttributeEntryIndex);
        printf("Methods count: %u, methods successfully read: %d\n", jcf->methodCount, 1 + jcf->currentMethodEntryIndex);
    }
    else if (jcf->currentAttributeEntryIndex + 1 != jcf->attributeCount)
    {
        printf("Failed to read attribute at index #%d\n", jcf->currentAttributeEntryIndex + 1);
        printf("Attributes count: %u, attributes successfully read: %d\n", jcf->attributeCount, 1 + jcf->currentAttributeEntryIndex);
    }

    printf("File status code: %d\nStatus description: %s.\n", jcf->status, decodeJavaClassFileStatus(jcf->status));
    printf("Number of bytes read: %d\n", jcf->totalBytesRead);
}

void printClassFileInfo(JavaClassFile* jcf)
{
    char buffer[256];
    cp_info* cp;
    uint16_t u16;

    printf("---- General Information ----\n\n");

    printf("Version:\t\t%u.%u (Major.minor)\n", jcf->majorVersion, jcf->minorVersion);
    printf("Constant pool count:\t%u\n", jcf->constantPoolCount);

    decodeAccessFlags(jcf->accessFlags, buffer, sizeof(buffer), ACCT_CLASS);
    printf("Access flags:\t\t0x%.4X [%s]\n", jcf->accessFlags, buffer);

    cp = jcf->constantPool + jcf->thisClass - 1;
    cp = jcf->constantPool + cp->Class.name_index - 1;
    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
    printf("This class:\t\tcp index #%u <%s>\n", jcf->thisClass, buffer);

    cp = jcf->constantPool + jcf->superClass - 1;
    cp = jcf->constantPool + cp->Class.name_index - 1;
    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
    printf("Super class:\t\tcp index #%u <%s>\n", jcf->superClass, buffer);

    printf("Interfaces count:\t%u\n", jcf->interfaceCount);
    printf("Fields count:\t\t%u\n", jcf->fieldCount);
    printf("Methods count:\t\t%u\n", jcf->methodCount);
    printf("Attributes count:\t%u\n", jcf->attributeCount);

    printConstantPool(jcf);

    if (jcf->interfaceCount > 0)
    {
        printf("\n---- Interfaces ----\n\n");

        for (u16 = 0; u16 < jcf->interfaceCount; u16++)
        {
            cp = jcf->constantPool + *(jcf->interfaces + u16) - 1;
            cp = jcf->constantPool + cp->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("\tInterface #%u: cp index #%u <%s>\n", u16 + 1, *(jcf->interfaces + u16), buffer);
        }
    }

    if (jcf->fieldCount > 0)
    {
        printf("\n---- Fields ----\n\n");
        // TODO: print fields
        // TODO: move field printing code to fields.c
    }

    if (jcf->methodCount > 0)
    {
        printf("\n---- Methods ----\n\n");
        // TODO: print methods
        // TODO: move method printing code to methods.c
    }

    if (jcf->attributeCount > 0)
    {
        printf("\n---- Attributes ----\n\n");
        // TODO: print attributes
        // TODO: move attribute printing code to attributes.c
    }
}
