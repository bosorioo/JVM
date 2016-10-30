#include <stdlib.h>
#include "readfunctions.h"
#include "constantpool.h"

static inline char readConstantPool_Class(JavaClassFile* jcf, cp_info* entry)
{
    if (!readu2(jcf, &entry->Class.name_index))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Class.name_index == 0 ||
        entry->Class.name_index >= jcf->constantPoolCount)
    {
        jcf->status = INVALID_CONSTANT_POOL_INDEX;
        return 0;
    }

    // We dont verify if this name index points to a UTF-8 constant in the pool
    // nor if that UTF-8 constant is valid class name because it could be a
    // constant that hasn't been added to the constant pool, therefore the
    // memory that would be accessed isn't written yet.
    // That verification has to be done later, after all the constant have
    // been read.

    return 1;
}

static inline char readConstantPool_Fieldref(JavaClassFile* jcf, cp_info* entry)
{
    if (!readu2(jcf, &entry->Fieldref.class_index))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Fieldref.class_index == 0 ||
        entry->Fieldref.class_index >= jcf->constantPoolCount)
    {
        jcf->status = INVALID_CONSTANT_POOL_INDEX;
        return 0;
    }

    if (!readu2(jcf, &entry->Fieldref.name_and_type_index))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Fieldref.name_and_type_index == 0 ||
        entry->Fieldref.name_and_type_index >= jcf->constantPoolCount)
    {
        jcf->status = INVALID_CONSTANT_POOL_INDEX;
        return 0;
    }

    return 1;
}

static inline char readConstantPool_Integer(JavaClassFile* jcf, cp_info* entry)
{
    if (!readu4(jcf, &entry->Integer.value))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    return 1;
}

static inline char readConstantPool_Long(JavaClassFile* jcf, cp_info* entry)
{
    if (!readu4(jcf, &entry->Long.high))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (!readu4(jcf, &entry->Long.low))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    return 1;
}

static inline char readConstantPool_Utf8(JavaClassFile* jcf, cp_info* entry)
{
    if (!readu2(jcf, &entry->Utf8.length))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Utf8.length > 0)
    {
        entry->Utf8.bytes = (uint8_t*)malloc(entry->Utf8.length);

        if (!entry->Utf8.bytes)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint16_t i;
        uint8_t* bytes = entry->Utf8.bytes;

        for (i = 0; i < entry->Utf8.length; i++)
        {
            int byte = fgetc(jcf->file);

            if (byte == EOF)
            {
                jcf->status = UNEXPECTED_EOF_READING_UTF8;
                return 0;
            }

            if (byte == 0 || (byte >= 0xF0))
            {
                jcf->status = INVALID_UTF8_BYTES;
                return 0;
            }

            *bytes++ = (uint8_t)byte;
            jcf->totalBytesRead++;
        }
    }
    else
    {
        entry->Utf8.bytes = 0;
    }

    return 1;
}

char readConstantPoolEntry(JavaClassFile* jcf, cp_info* entry)
{
    int byte = fgetc(jcf->file);

    if (byte == EOF)
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        entry->tag = 0xFF;
        return 0;
    }

    entry->tag = (uint8_t)byte;

    jcf->totalBytesRead++;
    jcf->lastTagRead = entry->tag;

    switch(entry->tag)
    {
        case CONSTANT_Class:
        case CONSTANT_String:
            return readConstantPool_Class(jcf, entry);

        case CONSTANT_Utf8:
            return readConstantPool_Utf8(jcf, entry);

        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
        case CONSTANT_NameAndType:
            return readConstantPool_Fieldref(jcf, entry);

        case CONSTANT_Integer:
        case CONSTANT_Float:
            return readConstantPool_Integer(jcf, entry);

        case CONSTANT_Long:
        case CONSTANT_Double:
            return readConstantPool_Long(jcf, entry);

        default:
            jcf->status = UNKNOWN_CONSTANT_POOL_TAG;
            break;
    }

    return 0;
}
