#include "methods.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include "string.h"
#include "memoryinspect.h"

/// @brief Reads a method_info from the file
///
/// @param JavaClass* jc - pointer to the structure to be
/// read.
/// @param method_info* entry - where the data read is written 
///
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readMethod(JavaClass* jc, method_info* entry)
{
    entry->attributes = NULL;
    jc->currentAttributeEntryIndex = -2;

    if (!readu2(jc, &entry->access_flags) ||
        !readu2(jc, &entry->name_index) ||
        !readu2(jc, &entry->descriptor_index) ||
        !readu2(jc, &entry->attributes_count))
    {
        jc->status = UNEXPECTED_EOF;
        return 0;
    }

    if (!checkMethodAccessFlags(jc, entry->access_flags))
        return 0;

    if (entry->name_index == 0 ||
        entry->name_index >= jc->constantPoolCount ||
        !isValidMethodNameIndex(jc, entry->name_index))
    {
        jc->status = INVALID_NAME_INDEX;
        return 0;
    }

    cp_info* cpi = jc->constantPool + entry->descriptor_index - 1;

    if (entry->descriptor_index == 0 ||
        entry->descriptor_index >= jc->constantPoolCount ||
        cpi->tag != CONSTANT_Utf8 ||
        cpi->Utf8.length != readMethodDescriptor(cpi->Utf8.bytes, cpi->Utf8.length, 1))
    {
        jc->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    if (entry->attributes_count > 0)
    {
        entry->attributes = (attribute_info*)malloc(sizeof(attribute_info) * entry->attributes_count);

        if (!entry->attributes)
        {
            jc->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint16_t i;

        jc->currentAttributeEntryIndex = -1;

        for (i = 0; i < entry->attributes_count; i++)
        {
            if (!readAttribute(jc, entry->attributes + i))
            {
                // Only "i" + 1 attributes were attempted to read, so to avoid
                // releasing uninitialized attributes (which could lead to a crash)
                // we set the attributes count to the correct amount
                entry->attributes_count = i + 1;
                return 0;
            }

            jc->currentAttributeEntryIndex++;
        }
    }

    return 1;
}

/// @brief Releases attributes used by the method_info struct
///
/// @param method_info* entry - pointer to the method_info that 
/// contains the attributes
///
/// @note This function does not free the @b *entry pointer, 
/// just attributes
void freeMethodAttributes(method_info* entry)
{
    uint32_t i;

    if (entry->attributes != NULL)
    {
        for (i = 0; i < entry->attributes_count; i++)
            freeAttributeInfo(entry->attributes + i);

        free(entry->attributes);

        entry->attributes_count = 0;
        entry->attributes = NULL;
    }
}

/// @brief Function to print all methods of the class file. 
///
/// @param JavaClass *jc - pointer to JavaClass structure that must already
/// be loaded
void printMethods(JavaClass* jc)
{

    uint16_t u16, att_index;
    char buffer[48];
    cp_info* cpi;
    method_info* mi;
    attribute_info* atti;

    if (jc->methodCount > 0)
    {
        printf("\n---- Methods ----\n");

        for (u16 = 0; u16 < jc->methodCount; u16++)
        {
            mi = jc->methods + u16;

            cpi = jc->constantPool + mi->name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

            printf("\n\tMethod #%u:\n\n", u16 + 1);
            printf("\t\tname_index:        #%u <%s>\n", mi->name_index, buffer);

            cpi = jc->constantPool + mi->descriptor_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tdescriptor_index:  #%u <%s>\n", mi->descriptor_index, buffer);

            decodeAccessFlags(mi->access_flags, buffer, sizeof(buffer), ACCT_METHOD);
            printf("\t\taccess_flags:      0x%.4X [%s]\n", mi->access_flags, buffer);

            printf("\t\tattribute_count:   %u\n", mi->attributes_count);

            if (mi->attributes_count > 0)
            {
                for (att_index = 0; att_index < mi->attributes_count; att_index++)
                {
                    atti = mi->attributes + att_index;
                    cpi = jc->constantPool + atti->name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

                    printf("\n\t\tMethod Attribute #%u - %s:\n", att_index + 1, buffer);
                    printAttribute(jc, atti, 3);
                }
            }

        }
    }
}

method_info* getMethodMatching(JavaClass* jc, const uint8_t* name, int32_t name_len, const uint8_t* descriptor,
                               int32_t descriptor_len, uint16_t flag_mask)
{
    method_info* method = jc->methods;
    cp_info* cpi;
    uint16_t index;

    for (index = jc->methodCount; index > 0; index--, method++)
    {
        // Check if flags match
        if ((method->access_flags & flag_mask) != flag_mask)
            continue;

        // Get the method name
        cpi = jc->constantPool + method->name_index - 1;
        // And check if it matches
        if (!cmp_UTF8_Ascii(cpi->Utf8.bytes, cpi->Utf8.length, name, name_len))
            continue;

        // Get the method descriptor
        cpi = jc->constantPool + method->descriptor_index - 1;
        // And check if it matches
        if (!cmp_UTF8_Ascii(cpi->Utf8.bytes, cpi->Utf8.length, descriptor, descriptor_len))
            continue;


        return method;
    }

    return NULL;
}
