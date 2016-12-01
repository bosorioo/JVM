#include "memoryinspect.h"
#include <string.h>
#include <inttypes.h> // Usage of macro "PRId64" to print 64 bit integer
#include "readfunctions.h"
#include "constantpool.h"
#include "utf8.h"

/// @brief Reads a cp_info of type CONSTANT_Class from the file
/// 
/// Reads a cp_info of type CONSTANT_Class from the file
/// Data read is written to pointer *entry.
/// CONSTANT_Class has the same structure as CONSTANT_String,
/// so this function could be used to read that too.
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param cp_info* entry - where the data read is written 
///
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readConstantPool_Class(JavaClass* jc, cp_info* entry)
{
    if (!readu2(jc, &entry->Class.name_index))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Class.name_index == 0 ||
        entry->Class.name_index >= jc->constantPoolCount)
    {
        jc->status = INVALID_CONSTANT_POOL_INDEX;
        return 0;
    }

    // We dont verify if this name index points to a UTF-8 constant in the pool
    // nor if that UTF-8 constant is valid class name because it could be a
    // constant that hasn't been added to the constant pool, therefore the
    // memory that would be accessed isn't written yet.
    // That verification has to be done later, after all the constant have
    // been read.
    // Also, this function can be used to read a CONSTANT_Class or a
    // CONSTANT_String, so there is no way to know which one is being
    // read.

    return 1;
}

/// @brief Reads a cp_info of type CONSTANT_Fieldref from the file
///
/// CONSTANT_Fieldref has the same internal structure as CONSTANT_Methodref
/// CONSTANT_InterfaceMethodref and CONSTANT_NameAndType, so this function
/// can be also used to read those.
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param cp_info* entry - where the data read is written 
/// 
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readConstantPool_Fieldref(JavaClass* jc, cp_info* entry)
{
    if (!readu2(jc, &entry->Fieldref.class_index))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Fieldref.class_index == 0 ||
        entry->Fieldref.class_index >= jc->constantPoolCount)
    {
        jc->status = INVALID_CONSTANT_POOL_INDEX;
        return 0;
    }

    if (!readu2(jc, &entry->Fieldref.name_and_type_index))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Fieldref.name_and_type_index == 0 ||
        entry->Fieldref.name_and_type_index >= jc->constantPoolCount)
    {
        jc->status = INVALID_CONSTANT_POOL_INDEX;
        return 0;
    }

    // Class index isn't verified because they could
    // point to a constant pool entry that hasn't been added yet.
    // That verification has to be done later, after all the constant have
    // been read.
    // Also, this function could be used to read a CONSTANT_NameAndType,
    // CONSTANT_Methodref, CONSTANT_InterfaceMethodref or CONSTANT_Fieldref.
    // Therefore, there is no way to know which one is being read, so no
    // checks should be made here besides constant pool index boundary.

    return 1;
}

/// @brief Reads a cp_info of type CONSTANT_Integer from the file
///
/// CONSTANT_Integer has the same structure as CONSTANT_Float,
/// so this function could be used to read that too.
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param cp_info* entry - where the data read is written 
/// 
/// @return char - retuns 0 if something unexpected happened 
/// 1 in case of success
char readConstantPool_Integer(JavaClass* jc, cp_info* entry)
{
    if (!readu4(jc, &entry->Integer.value))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    return 1;
}

/// @brief Reads a cp_info of type CONSTANT_Long from the file
///
/// CONSTANT_Long has the same structure as CONSTANT_Double,
/// so this function could be used to read that too.
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param cp_info* entry - where the data read is written 
///
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readConstantPool_Long(JavaClass* jc, cp_info* entry)
{
    if (!readu4(jc, &entry->Long.high))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (!readu4(jc, &entry->Long.low))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    return 1;
}

/// @brief Reads a cp_info of type CONSTANT_Utf8 from the file
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param cp_info* entry - where the data read is written 
///
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readConstantPool_Utf8(JavaClass* jc, cp_info* entry)
{
    if (!readu2(jc, &entry->Utf8.length))
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        return 0;
    }

    if (entry->Utf8.length > 0)
    {
        entry->Utf8.bytes = (uint8_t*)malloc(entry->Utf8.length);

        if (!entry->Utf8.bytes)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint16_t i;
        uint8_t* bytes = entry->Utf8.bytes;

        for (i = 0; i < entry->Utf8.length; i++)
        {
            int byte = fgetc(jc->file);

            if (byte == EOF)
            {
                jc->status = UNEXPECTED_EOF_READING_UTF8;
                return 0;
            }

            jc->totalBytesRead++;

            // UTF-8 byte values can't be null and must not be in the range [0xF0, 0xFF].
            if (byte == 0 || (byte >= 0xF0))
            {
                jc->status = INVALID_UTF8_BYTES;
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

/// @brief Reads a cp_info from the file
///
/// This function identifies the structure indicated by the tag byte
/// and selects the corresponding reading type
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param cp_info* entry - where the data read is written 
///
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readConstantPoolEntry(JavaClass* jc, cp_info* entry)
{
    // Gets the entry tag
    int byte = fgetc(jc->file);

    if (byte == EOF)
    {
        jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
        entry->tag = 0xFF;
        return 0;
    }

    entry->tag = (uint8_t)byte;

    jc->totalBytesRead++;
    jc->lastTagRead = entry->tag;

    switch(entry->tag)
    {
        case CONSTANT_MethodType: // Compatibility with Java 8
        case CONSTANT_Class:
        case CONSTANT_String:
            return readConstantPool_Class(jc, entry);

        case CONSTANT_Utf8:
            return readConstantPool_Utf8(jc, entry);

        case CONSTANT_InvokeDynamic: // Compatibility with Java 8
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
        case CONSTANT_NameAndType:
            return readConstantPool_Fieldref(jc, entry);

        case CONSTANT_Integer:
        case CONSTANT_Float:
            return readConstantPool_Integer(jc, entry);

        case CONSTANT_Long:
        case CONSTANT_Double:
            return readConstantPool_Long(jc, entry);

        // Compatibility with Java 8
        case CONSTANT_MethodHandle:

            if (!readu2(jc, NULL) || fgetc(jc->file) == EOF)
            {
                jc->status = UNEXPECTED_EOF_READING_CONSTANT_POOL;
                return 0;
            }

            jc->totalBytesRead++;

            break;

        default:
            jc->status = UNKNOWN_CONSTANT_POOL_TAG;
            break;
    }

    return 0;
}

/// @brief Decodes ConstantPoolTag enumeration elements
///
/// @param uint8_t tag - identifier of the enumeration to be
/// translated.
///
/// @return const char* - pointer to char containing the decoded tag
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

/// @brief Print one single constant from the constant pool.
/// 
/// @param JavaClass* jc - pointer to JavaClass structure that must already
/// be loaded, no checks are made.
/// @param *entry - the pointer to the constant that will be
/// printed.
void printConstantPoolEntry(JavaClass* jc, cp_info* entry)
{
    char buffer[48];
    uint32_t u32;
    cp_info* cpi;

    switch(entry->tag)
    {

        case CONSTANT_Utf8:

            u32 = UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), entry->Utf8.bytes, entry->Utf8.length);
            printf("\t\tByte Count: %u\n", entry->Utf8.length);
            printf("\t\tLength: %u\n", u32);
            printf("\t\tASCII Characters: %s", buffer);

            if (u32 != entry->Utf8.length)
                printf("\n\t\tUTF-8 Characters: %.*s", (int)entry->Utf8.length, entry->Utf8.bytes);

            break;

        case CONSTANT_String:
            cpi = jc->constantPool + entry->String.string_index - 1;
            u32 = UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tstring_index: #%u\n", entry->String.string_index);
            printf("\t\tString Length: %u\n", u32);
            printf("\t\tASCII: %s", buffer);

            if (u32 != cpi->Utf8.length)
                printf("\n\t\tUTF-8: %.*s", (int)cpi->Utf8.length, cpi->Utf8.bytes);

            break;

        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:

            cpi = jc->constantPool + entry->Fieldref.class_index - 1;
            cpi = jc->constantPool + cpi->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tclass_index: #%u <%s>\n", entry->Fieldref.class_index, buffer);
            printf("\t\tname_and_type_index: #%u\n", entry->Fieldref.name_and_type_index);

            cpi = jc->constantPool + entry->Fieldref.name_and_type_index - 1;
            u32 = cpi->NameAndType.name_index;
            cpi = jc->constantPool + u32 - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_and_type.name_index: #%u <%s>\n", u32, buffer);

            cpi = jc->constantPool + entry->Fieldref.name_and_type_index - 1;
            u32 = cpi->NameAndType.descriptor_index;
            cpi = jc->constantPool + u32 - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_and_type.descriptor_index: #%u <%s>", u32, buffer);

            break;

        case CONSTANT_Class:

            cpi = jc->constantPool + entry->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_index: #%u <%s>", entry->Class.name_index, buffer);
            break;

        case CONSTANT_NameAndType:

            cpi = jc->constantPool + entry->NameAndType.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_index: #%u <%s>\n", entry->NameAndType.name_index, buffer);

            cpi = jc->constantPool + entry->NameAndType.descriptor_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tdescriptor_index: #%u <%s>", entry->NameAndType.descriptor_index, buffer);
            break;

        case CONSTANT_Integer:
            printf("\t\tBytes: 0x%08X\n", entry->Integer.value);
            printf("\t\tValue: %d", (int32_t)entry->Integer.value);
            break;

        case CONSTANT_Long:
            printf("\t\tHigh Bytes: 0x%08X\n", entry->Long.high);
            printf("\t\tLow  Bytes: 0x%08X\n", entry->Long.low);
            printf("\t\tLong Value: %" PRId64"", ((int64_t)entry->Long.high << 32) | entry->Long.low);
            break;

        case CONSTANT_Float:
            printf("\t\tBytes: 0x%08X\n", entry->Float.bytes);
            printf("\t\tValue: %e", readFloatFromUint32(entry->Float.bytes));
            break;

        case CONSTANT_Double:
            printf("\t\tHigh Bytes:   0x%08X\n", entry->Double.high);
            printf("\t\tLow  Bytes:   0x%08X\n", entry->Double.low);
            printf("\t\tDouble Value: %e", readDoubleFromUint64((uint64_t)entry->Double.high << 32 | entry->Double.low));
            break;

        default:
            break;
    }

    printf("\n");
}

/// @brief Function to print all the constants from the constant pool
/// of the class file. 
///
/// @param JavaClass *jc - pointer to JavaClass structure that must already
/// be loaded
void printConstantPool(JavaClass* jc)
{
    uint16_t u16;
    cp_info* cp;

    if (jc->constantPoolCount > 1)
    {
        printf("\n---- Constant Pool ----\n");

        for (u16 = 0; u16 < jc->constantPoolCount - 1; u16++)
        {
            cp = jc->constantPool + u16;
            printf("\n\t#%u: %s (tag = %u)\n", u16 + 1, decodeTag(cp->tag), cp->tag);
            printConstantPoolEntry(jc, cp);

            if (cp->tag == CONSTANT_Double || cp->tag == CONSTANT_Long)
                u16++;
        }
    }
}
