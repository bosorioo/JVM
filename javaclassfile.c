#include "readfunctions.h"
#include "javaclassfile.h"
#include "constantpool.h"
#include <stdlib.h>

void getFileName(const char* path, int32_t* outBegin, int32_t* outFinish)
{
    if (!path || !outBegin || !outFinish)
        return;

    *outBegin = *outFinish = 0;
    int32_t i ;

    for (i = 0; path[i] != '\0'; i++)
    {
        if (path[i] == '/' || path[i] == '\\')
            *outBegin = i + 1;
    }

    for (*outFinish = *outBegin; path[*outFinish] != '\0'; (*outFinish)++)
    {
        if (path[*outFinish] == '.')
        {
            (*outFinish)--;
            break;
        }
    }
}

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

        for (u32 = 0; u32 < jcf->constantPoolCount - 1; u32++)
        {
            if (!readConstantPoolEntry(jcf, jcf->constantPool + u32))
                return;

            jcf->currentConstantPoolEntryIndex++;
        }
    }

    // TODO: check if all class_index points to valid UTF-8 class names
    // TODO: check if all descriptor_index points to valid descriptors etc.

    if (!readu2(jcf, &jcf->accessFlags) ||
        !readu2(jcf, &jcf->thisClass) ||
        !readu2(jcf, &jcf->superClass))
    {
        jcf->status = UNEXPECTED_EOF;
        return;
    }

    if (jcf->accessFlags & ACC_INVALID_CLASS_FLAG_MASK)
    {
        jcf->status = USE_OF_RESERVED_ACCESS_FLAGS;
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

    int32_t fileNameBegin, fileNameEnd, fileNameLength;
    char fileName[256];

    getFileName(path, &fileNameBegin, &fileNameEnd);
    fileNameLength = fileNameEnd - fileNameBegin + 2;

    if (fileNameLength <= 1)
    {
        jcf->status = CLASS_FILE_NAME_INVALID;
        return;
    }

    if (fileNameLength > sizeof(fileName))
    {
        jcf->status = CLASS_FILE_NAME_TOO_LONG;
        return;
    }

    snprintf(fileName, sizeof(fileName) > fileNameLength ? fileNameLength : sizeof(fileName), "%s", path + fileNameBegin);

    // TODO: assert that jcf->thisClass and file name are the same

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
        jcf->status = FILE_CONTAINS_UNEXPECTED_DATA;

}

void freeClassFile(JavaClassFile* jcf)
{
    if (!jcf)
        return;

    uint32_t i;

    if (jcf->file)
    {
        fclose(jcf->file);
        jcf->file = NULL;
    }

    if (jcf->interfaces)
    {
        free(jcf->interfaces);
        jcf->interfaces = NULL;
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
    }

    if (jcf->methods)
    {
        // TODO: proper free of methods
    }

    if (jcf->attributes)
    {
        // TODO: proper free of attributes
    }
}

const char* decodeJavaClassFileStatus(enum JavaClassStatus status)
{
    switch (status)
    {
        case STATUS_OK: return "File is ok";
        case FILE_COULDNT_BE_OPENED: return "File couldn't be opened";
        case INVALID_SIGNATURE: return "Signature (0xCAFEBABE) mismatch";
        case CLASS_FILE_NAME_INVALID: return "File name is invalid";
        case CLASS_FILE_NAME_TOO_LONG: return "File name is too long";
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
        case USE_OF_RESERVED_ACCESS_FLAGS: return "Class access flags contains bits that should be zero";
        case INVALID_THIS_CLASS_INDEX: return "\"This Class\" field doesn't point to valid class index";
        case INVALID_SUPER_CLASS_INDEX: return "\"Super class\" field doesn't point to valid class index";
        case INVALID_INTERFACE_INDEX: return "Interface index doesn't point to a valid class index";
        case INVALID_FIELD_NAME_INDEX: return "Field has a name index that doesn't point to a valid UTF-8 index";
        case INVALID_FIELD_DESCRIPTOR_INDEX: return "Field has a descriptor index that doesn't point to a valid UTF-8 index";
        case FILE_CONTAINS_UNEXPECTED_DATA: return "Class file contains more data than expected, which wasn't processed";

        default:
            break;
    }

    return "Unknown status";
}

void printGeneralInfo(JavaClassFile* jcf)
{
    printf("File status: %d, %s.\n", jcf->status, decodeJavaClassFileStatus(jcf->status));
    printf("Number of bytes read: %d\n", jcf->totalBytesRead);
    printf("Java Class Version: %u.%u\n", jcf->majorVersion, jcf->minorVersion);
    printf("Constant pool count: %u, constants successfully read: %u\n", jcf->constantPoolCount, jcf->currentConstantPoolEntryIndex);
    printf("Interface count: %u, interfaces successfully read: %u\n", jcf->interfaceCount, jcf->currentInterfaceEntryIndex);
    printf("Fields count: %u, fields successfully read: %u\n", jcf->fieldCount, jcf->currentFieldEntryIndex);
    printf("Methods count: %u, methods successfully read: %u\n", jcf->methodCount, jcf->currentMethodEntryIndex);
    printf("Attributes count: %u, attributes successfully read: %u\n", jcf->attributeCount, jcf->currentAttributeEntryIndex);
}
