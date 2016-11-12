#include "methods.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include <stdlib.h>

char readMethod(JavaClassFile* jcf, method_info* entry)
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

    uint16_t u16;
    char buffer[48];
    cp_info* cp;
    method_info* mi;

    if (jcf->methodCount > 0)
    {
        printf("\n---- Methods ----\n");

        for (u16 = 0; u16 < jcf->methodCount; u16++)
        {
        
            mi = jcf->methods + u16;

            cp = jcf->constantPool + mi->name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

            printf("\n\n\tMethod #%u:\n", u16 + 1);
            printf("\t\t name_index:        cp index #%u - %s\n", mi->name_index, buffer);

            cp = jcf->constantPool + mi->descriptor_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("\t\t descriptor_index:  cp index #%u - %s\n", mi->descriptor_index, buffer);    

        }
    }
}

