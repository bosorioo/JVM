#include "fields.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include <stdlib.h>

char readField(JavaClass* jc, field_info* entry)
{
    entry->attributes = NULL;

    if (!readu2(jc, &entry->access_flags) ||
        !readu2(jc, &entry->name_index) ||
        !readu2(jc, &entry->descriptor_index) ||
        !readu2(jc, &entry->attributes_count))
    {
        jc->status = UNEXPECTED_EOF;
        return 0;
    }

    if (entry->access_flags & ACC_INVALID_FIELD_FLAG_MASK)
    {
        jc->status = USE_OF_RESERVED_FIELD_ACCESS_FLAGS;
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

        jc->currentAttributeEntryIndex = -1;

        for (i = 0; i < entry->attributes_count; i++)
        {
            if (!readAttribute(jc, entry->attributes + i))
            {
                // Only "i" attributes have been successfully read, so to avoid
                // releasing uninitialized attributes (which could lead to a crash)
                // we set the attributes count to the correct amount
                entry->attributes_count = i;
                return 0;
            }

            jc->currentAttributeEntryIndex++;
        }
    }

    return 1;
}

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
