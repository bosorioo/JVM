#include "methods.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include <stdlib.h>

char readMethod(JavaClassFile* jcf, method_info* entry)
{
    entry->attributes = NULL;
    jcf->currentAttributeEntryIndex = -2;

    if (!readu2(jcf, &entry->access_flags) ||
        !readu2(jcf, &entry->name_index) ||
        !readu2(jcf, &entry->descriptor_index) ||
        !readu2(jcf, &entry->attributes_count))
    {
        jcf->status = UNEXPECTED_EOF;
        return 0;
    }

    if (!checkMethodAccessFlags(jcf, entry->access_flags))
        return 0;

    if (entry->name_index == 0 ||
        entry->name_index >= jcf->constantPoolCount ||
        !isValidMethodNameIndex(jcf, entry->name_index))
    {
        jcf->status = INVALID_NAME_INDEX;
        return 0;
    }

    cp_info* cpi = jcf->constantPool + entry->descriptor_index - 1;

    if (entry->descriptor_index == 0 ||
        entry->descriptor_index >= jcf->constantPoolCount ||
        cpi->tag != CONSTANT_Utf8 ||
        cpi->Utf8.length != readMethodDescriptor(cpi->Utf8.bytes, cpi->Utf8.length, 1))
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

void printMethods(JavaClassFile* jcf)
{

    uint16_t u16, att_index;
    char buffer[48];
    cp_info* cpi;
    method_info* mi;
    attribute_info* atti;

    if (jcf->methodCount > 0)
    {
        printf("\n---- Methods ----\n");

        for (u16 = 0; u16 < jcf->methodCount; u16++)
        {
            mi = jcf->methods + u16;

            cpi = jcf->constantPool + mi->name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

            printf("\n\tMethod #%u:\n\n", u16 + 1);
            printf("\t\tname_index:        cp index #%u <%s>\n", mi->name_index, buffer);

            cpi = jcf->constantPool + mi->descriptor_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tdescriptor_index:  cp index #%u <%s>\n", mi->descriptor_index, buffer);

            decodeAccessFlags(mi->access_flags, buffer, sizeof(buffer), ACCT_METHOD);
            printf("\t\taccess_flags:      0x%.4X [%s]\n", mi->access_flags, buffer);

            printf("\t\tattribute_count:   %u\n", mi->attributes_count);

            if (mi->attributes_count > 0)
            {
                for (att_index = 0; att_index < mi->attributes_count; att_index++)
                {
                    atti = mi->attributes + att_index;
                    cpi = jcf->constantPool + atti->name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

                    printf("\n\t\tMethod Attribute #%u - %s:\n", att_index + 1, buffer);
                    printAttribute(jcf, atti, 3);
                }
            }

        }
    }
}

