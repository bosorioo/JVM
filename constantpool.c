#include <stdlib.h>
#include <math.h>
#include "readfunctions.h"
#include "constantpool.h"
#include "utf8.h"

char readConstantPool_Class(JavaClassFile* jcf, cp_info* entry)
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

char readConstantPool_Fieldref(JavaClassFile* jcf, cp_info* entry)
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

    // Class index and name_and_type index aren't verified because they could
    // point to a constant pool entry that hasn't been added yet.
    // That verification has to be done later, after all the constant have
    // been read.

    return 1;
}

char readConstantPool_Integer(JavaClassFile* jcf, cp_info* entry)
{
    if (!readu4(jcf, &entry->Integer.value))
    {
        jcf->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    return 1;
}

char readConstantPool_Long(JavaClassFile* jcf, cp_info* entry)
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

char readConstantPool_Utf8(JavaClassFile* jcf, cp_info* entry)
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

            jcf->totalBytesRead++;

            if (byte == 0 || (byte >= 0xF0))
            {
                jcf->status = INVALID_UTF8_BYTES;
                return 0;
            }

            *bytes++ = (uint8_t)byte;
        }
    }
    else
    {
        entry->Utf8.bytes = NULL;
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

const char* decodeTag(uint8_t tag)
{
    switch(tag)
    {
        case CONSTANT_Class: return "Class";
        case CONSTANT_Double: return "Double";
        case CONSTANT_Fieldref: return "Fieldref";
        case CONSTANT_Float: return "Float";
        case CONSTANT_Integer: return "Integer";
        case CONSTANT_InterfaceMethodref: return "InterfaceMethodref";
        case CONSTANT_Long: return "Long";
        case CONSTANT_Methodref: return "Methodref";
        case CONSTANT_NameAndType: return "NameAndType";
        case CONSTANT_String: return "String";
        case CONSTANT_Utf8: return "Utf8";
        default:
            break;
    }

    return "Unknown Tag";
}

void printConstantPoolEntry(JavaClassFile* jcf, cp_info* entry)
{
    char buffer[48];
    uint16_t u16;

    switch(entry->tag)
    {

        case CONSTANT_Utf8:

            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), entry->Utf8.bytes, entry->Utf8.length);
            printf("\tLength: %u\n\tCharacters: %s", entry->Utf8.length, buffer);
            break;

        case CONSTANT_Class:

            u16 = entry->Class.name_index;
            entry = jcf->constantPool + entry->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), entry->Utf8.bytes, entry->Utf8.length);
            printf("\tname_index: cp index #%u\n\tCharacters: %s", u16, buffer);
            break;


        // TODO: finish printing constant pool

        default:
            break;
    }

    printf("\n\n");
}

void printConstantPool(JavaClassFile* jcf)
{
    uint16_t u16;
    cp_info* cp;

    if (jcf->constantPoolCount > 1)
    {
        printf("\n---- Constant Pool ----\n\n");

        for (u16 = 0; u16 < jcf->constantPoolCount - 1; u16++)
        {
            cp = jcf->constantPool + u16;
            printf("%*c#%u: %s (tag = %u)\n", 5 - (int)log10(u16 + 1), ' ', u16 + 1, decodeTag(cp->tag), cp->tag);
            printConstantPoolEntry(jcf, cp);
        }
    }
}
