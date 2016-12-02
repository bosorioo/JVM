#include "fields.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include "debugging.h"

/// @brief Reads a field_info from the file
///
/// @param JavaClass* jc - pointer to the structure to be
/// read
/// @param field_info* entry - where the data read is written
///
/// @return char - retuns 0 if something unexpected happened or
/// failure, 1 in case of success
char readField(JavaClass* jc, field_info* entry)
{
    entry->attributes = NULL;
    jc->attributeEntriesRead = -1;

    if (!readu2(jc, &entry->access_flags) ||
        !readu2(jc, &entry->name_index) ||
        !readu2(jc, &entry->descriptor_index) ||
        !readu2(jc, &entry->attributes_count))
    {
        jc->status = UNEXPECTED_EOF;
        return 0;
    }

    if (!checkFieldAccessFlags(jc, entry->access_flags))
        return 0;

    if (entry->name_index == 0 ||
        entry->name_index >= jc->constantPoolCount ||
        !isValidNameIndex(jc, entry->name_index, 0))
    {
        jc->status = INVALID_NAME_INDEX;
        return 0;
    }

    cp_info* cpi = jc->constantPool + entry->descriptor_index - 1;

    if (entry->descriptor_index == 0 ||
        entry->descriptor_index >= jc->constantPoolCount ||
        cpi->tag != CONSTANT_Utf8 ||
        cpi->Utf8.length != readFieldDescriptor(cpi->Utf8.bytes, cpi->Utf8.length, 1))
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

        jc->attributeEntriesRead = 0;

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

            jc->attributeEntriesRead++;
        }
    }

    return 1;
}

/// @brief Releases attributes used by the field_info struct
///
/// @param field_info* entry - pointer to the field_info that
/// contains the attributes
///
/// @note This function does not free the @b *entry pointer,
/// just attributes
void freeFieldAttributes(field_info* entry)
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

/// @brief Function to print all fields of the class file.
///
/// @param JavaClass *jc - pointer to JavaClass structure that must already
/// be loaded
void printAllFields(JavaClass* jc)
{
    if (jc->fieldCount == 0)
        return;

    char buffer[48];
    uint16_t u16, att_index;
    field_info* fi;
    cp_info* cpi;
    attribute_info* atti;

    printf("\n---- Fields ----");

    for (u16 = 0; u16 < jc->fieldCount; u16++)
    {
        fi = jc->fields + u16;

        printf("\n\n\tField #%u:\n\n", u16 + 1);

        cpi = jc->constantPool + fi->name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
        printf("\t\tname_index:       #%u <%s>\n", fi->name_index, buffer);

        cpi = jc->constantPool + fi->descriptor_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
        printf("\t\tdescriptor_index: #%u <%s>\n", fi->descriptor_index, buffer);

        decodeAccessFlags(fi->access_flags, buffer, sizeof(buffer), ACCT_FIELD);
        printf("\t\taccess_flags:     0x%.4X [%s]\n", fi->access_flags, buffer);

        printf("\t\tattributes_count: %u\n", fi->attributes_count);

        if (fi->attributes_count > 0)
        {
            for (att_index = 0; att_index < fi->attributes_count; att_index++)
            {
                atti = fi->attributes + att_index;
                cpi = jc->constantPool + atti->name_index - 1;
                UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

                printf("\n\t\tField Attribute #%u - %s:\n", att_index + 1, buffer);
                printAttribute(jc, atti, 3);
            }
        }

    }

    printf("\n");
}

field_info* getFieldMatching(JavaClass* jc, const uint8_t* name, int32_t name_len, const uint8_t* descriptor,
                             int32_t descriptor_len, uint16_t flag_mask)
{
    field_info* field = jc->fields;
    cp_info* cpi;
    uint16_t index;

    for (index = jc->fieldCount; index > 0; index--, field++)
    {
        // Check if flags match
        if ((field->access_flags & flag_mask) != flag_mask)
            continue;

        // Get the field name
        cpi = jc->constantPool + field->name_index - 1;
        // And check if it matches
        if (!cmp_UTF8_Ascii(cpi->Utf8.bytes, cpi->Utf8.length, name, name_len))
            continue;

        // Get the field descriptor
        cpi = jc->constantPool + field->descriptor_index - 1;
        // And check if it matches
        if (!cmp_UTF8_Ascii(cpi->Utf8.bytes, cpi->Utf8.length, descriptor, descriptor_len))
            continue;


        return field;
    }

    return NULL;
}
