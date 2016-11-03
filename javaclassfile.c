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
    jcf->currentConstantPoolEntryIndex = 0;
    jcf->currentInterfaceEntryIndex = 0;
    jcf->currentFieldEntryIndex = 0;
    jcf->currentMethodEntryIndex = 0;
    jcf->currentAttributeEntryIndex = 0;

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

            jcf->currentConstantPoolEntryIndex++;
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

    if (jcf->accessFlags & ACC_INVALID_CLASS_FLAG_MASK)
    {
        jcf->status = USE_OF_RESERVED_CLASS_ACCESS_FLAGS;
        return;
    }

    if ((jcf->accessFlags & (ACC_ABSTRACT | ACC_INTERFACE)) && (jcf->accessFlags & ACC_FINAL))
    {
        jcf->status = INVALID_ACCESS_FLAGS;
        return;
    }

    if (!jcf->thisClass || jcf->constantPool[jcf->thisClass - 1].tag != CONSTANT_Class)
    {
        jcf->status = INVALID_THIS_CLASS_INDEX;
        return;
    }

    if (jcf->superClass && jcf->constantPool[jcf->superClass - 1].tag != CONSTANT_Class)
    {
        jcf->status = INVALID_SUPER_CLASS_INDEX;
        return;
    }

    if (!checkClassNameFileNameMatch(jcf, path))
    {
        jcf->status = CLASS_NAME_FILE_NAME_MISMATCH;
        return;
    }

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

        jcf->currentAttributeEntryIndex = 0;

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

    uint16_t i;

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
        for (i = 0; i < jcf->methodCount; i++)
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
        case STATUS_OK: return "File is ok";
        case FILE_COULDNT_BE_OPENED: return "Class file couldn't be opened";
        case INVALID_SIGNATURE: return "Signature (0xCAFEBABE) mismatch";
        case CLASS_NAME_FILE_NAME_MISMATCH: return "Class name and .class file name don't match";
        case MEMORY_ALLOCATION_FAILED: return "Not enough memory";
        case INVALID_CONSTANT_POOL_COUNT: return "Constant pool count should be at least 1";
        case UNEXPECTED_EOF: return "End of file found too soon";
        case UNEXPECTED_EOF_READING_CONSTANT_POOL: return "Unexpected end of file while reading constant pool";
        case UNEXPECTED_EOF_READING_UTF8: return "Unexpected end of file while reading UTF-8 stream";
        case UNEXPECTED_EOF_READING_INTERFACES: return "Unexpected end of file while reading interfaces' index";
        case UNEXPECTED_EOF_READING_ATTRIBUTE_INFO: return "Unexpected end of file while reading attribute information";
        case INVALID_UTF8_BYTES: return "Invalid UTF-8 encoding";
        case INVALID_CONSTANT_POOL_INDEX: return "Invalid index for constant pool entry";
        case UNKNOWN_CONSTANT_POOL_TAG: return "Unknown constant pool entry tag";
        case INVALID_ACCESS_FLAGS: return "Invalid combination of class access flags";

        case USE_OF_RESERVED_CLASS_ACCESS_FLAGS: return "Class access flags contains bits that should be zero";
        case USE_OF_RESERVED_METHOD_ACCESS_FLAGS: return "Method access flags contains bits that should be zero";
        case USE_OF_RESERVED_FIELD_ACCESS_FLAGS: return "Field access flags contains bits that should be zero";

        case INVALID_THIS_CLASS_INDEX: return "\"This Class\" field doesn't point to valid class index";
        case INVALID_SUPER_CLASS_INDEX: return "\"Super class\" field doesn't point to valid class index";
        case INVALID_INTERFACE_INDEX: return "Interface index doesn't point to a valid class index";
        case INVALID_FIELD_DESCRIPTOR_INDEX: return "descriptor_index doesn't point to a valid field descriptor";
        case INVALID_NAME_INDEX: return "name_index doesn't point to a valid UTF-8 stream";
        case INVALID_STRING_INDEX: return "string_index doesn't point to a valid UTF-8 stream";
        case INVALID_CLASS_INDEX: return "class_index doesn't point to a valid class";
        case INVALID_JAVA_IDENTIFIER: return "UTF-8 stream isn't a valid Java identifier";
        case FILE_CONTAINS_UNEXPECTED_DATA: return "Class file contains more data than expected, which wasn't processed";

        default:
            break;
    }

    return "Unknown status";
}

void decodeAccessFlags(uint32_t flags, char* buffer, int32_t buffer_len)
{
    uint32_t bytes = 0;
    const char* comma = ", ";
    const char* empty = "";

    #define decodeflag(flag, name) if (flags & flag) { \
        bytes = snprintf(buffer, buffer_len, "%s%s", bytes ? comma : empty, name); \
        buffer += bytes; \
        buffer_len -= bytes; }

    decodeflag(ACC_ABSTRACT, "abstract")
    decodeflag(ACC_INTERFACE, "interface")
    decodeflag(ACC_TRANSIENT, "transient")
    decodeflag(ACC_VOLATILE, "volatile")
    decodeflag(ACC_SUPER, "super")
    decodeflag(ACC_FINAL, "final")
    decodeflag(ACC_STATIC, "static")
    decodeflag(ACC_PUBLIC, "public")
    decodeflag(ACC_PRIVATE, "private")
    decodeflag(ACC_PROTECTED, "protected")

    #undef decodeflag
}

void printGeneralFileInfo(JavaClassFile* jcf)
{
    printf("File status: %d, %s.\n", jcf->status, decodeJavaClassFileStatus(jcf->status));
    printf("Number of bytes read: %d\n", jcf->totalBytesRead);
    printf("Version: %u.%u\n", jcf->majorVersion, jcf->minorVersion);
    printf("Constant pool count: %u, constants successfully read: %u\n", jcf->constantPoolCount, jcf->currentConstantPoolEntryIndex);
    printf("Interface count: %u, interfaces successfully read: %u\n", jcf->interfaceCount, jcf->currentInterfaceEntryIndex);
    printf("Fields count: %u, fields successfully read: %u\n", jcf->fieldCount, jcf->currentFieldEntryIndex);
    printf("Methods count: %u, methods successfully read: %u\n", jcf->methodCount, jcf->currentMethodEntryIndex);
    printf("Attributes count: %u, attributes successfully read: %u\n", jcf->attributeCount, jcf->currentAttributeEntryIndex);
}

void printClassInfo(JavaClassFile* jcf)
{
    char buffer[256];
    cp_info* cp;

    printf("---- General Information ----\n\n");

    printf("Version:\t\t%u.%u (Major.minor)\n", jcf->majorVersion, jcf->minorVersion);
    printf("Constant pool count:\t%u\n", jcf->constantPoolCount);

    decodeAccessFlags(jcf->accessFlags, buffer, sizeof(buffer));
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
        // TODO: print interfaces
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
