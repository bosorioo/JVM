#include "fields.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include <stdlib.h>

char readField(JavaClassFile* jcf, field_info* entry)
{
    entry->attributes = NULL;

    if (!readu2(jcf, &entry->access_flags) ||
        !readu2(jcf, &entry->name_index) ||
        !readu2(jcf, &entry->descriptor_index) ||
        !readu2(jcf, &entry->attributes_count))
    {
        jcf->status = UNEXPECTED_EOF;
        return 0;
    }

    if (entry->access_flags & ACC_INVALID_FIELD_FLAG_MASK)
    {
        jcf->status = USE_OF_RESERVED_FIELD_ACCESS_FLAGS;
        return 0;
    }

    // TODO: check access flags validity, example: can't be PRIVATE and PUBLIC

    if (entry->name_index == 0 ||
        entry->name_index >= jcf->constantPoolCount ||
        !isValidNameIndex(jcf, entry->name_index, 0))
    {
        jcf->status = INVALID_NAME_INDEX;
        return 0;
    }

    cp_info* cpi = jcf->constantPool + entry->descriptor_index - 1;

    if (entry->descriptor_index == 0 ||
        entry->descriptor_index >= jcf->constantPoolCount ||
        cpi->tag != CONSTANT_Utf8 ||
        cpi->Utf8.length != readFieldDescriptor(cpi->Utf8.bytes, cpi->Utf8.length, 1))
    {
        jcf->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    if (entry->attributes_count > 0)
    {
        entry->attributes = (attribute_info*)malloc(sizeof(attribute_info) * entry->attributes_count);

        if (!entry->attributes)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint16_t i;

        jcf->currentAttributeEntryIndex = -1;

        for (i = 0; i < entry->attributes_count; i++)
        {
            if (!readAttribute(jcf, entry->attributes + i))
            {
                // Only "i" attributes have been successfully read, so to avoid
                // releasing uninitialized attributes (which could lead to a crash)
                // we set the attributes count to the correct amount
                entry->attributes_count = i;
                return 0;
            }

            jcf->currentAttributeEntryIndex++;
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

void printAllFields(JavaClassFile* jcf)
{
    if (jcf->fieldCount == 0)
        return;

    char buffer[48];
    uint16_t u16;
    field_info* fi;
    cp_info* cpi;

    printf("\n---- Fields ----");

    for (u16 = 0; u16 < jcf->fieldCount; u16++)
    {
        fi = jcf->fields + u16;

        printf("\n\n\tField #%u:\n", u16 + 1);

        cpi = jcf->constantPool + fi->name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
        printf("\t\tname_index:       cp index #%u <%s>\n", fi->name_index, buffer);

        cpi = jcf->constantPool + fi->descriptor_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
        printf("\t\tdescriptor_index: cp index #%u <%s>\n", fi->descriptor_index, buffer);

        decodeAccessFlags(fi->access_flags, buffer, sizeof(buffer), ACCT_FIELD);
        printf("\t\taccess_flags:     0x%.4X [%s]\n", fi->access_flags, buffer);

        printf("\t\tattributes_count: %u", fi->attributes_count);

        if (fi->attributes_count > 0)
        {
            // TODO: print fields attributes
        }

    }

    printf("\n");
}
