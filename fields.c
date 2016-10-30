#include "fields.h"
#include "readfunctions.h"
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

    // TODO: check access flags validity

    if (entry->name_index == 0 ||
        entry->name_index >= jcf->constantPoolCount ||
        jcf->constantPool[entry->name_index - 1].tag != CONSTANT_Utf8)
    {
        jcf->status = INVALID_FIELD_NAME_INDEX;
        return 0;
    }

    // TODO: check if name_index points to a valid Java identifier

    if (entry->descriptor_index == 0 ||
        entry->descriptor_index >= jcf->constantPoolCount ||
        jcf->constantPool[entry->descriptor_index - 1].tag != CONSTANT_Utf8)
    {
        jcf->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    // TODO: check if descriptor_index points to a valid field descriptor

    if (entry->attributes_count > 0)
    {
        entry->attributes = (attribute_info*)malloc(sizeof(attribute_info) * entry->attributes_count);

        if (!entry->attributes)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint16_t i;

        jcf->currentAttributeEntryIndex = 0;

        for (i = 0; i < entry->attributes_count; i++)
        {
            if (!readAttribute(jcf, entry->attributes + i))
                return 0;

            jcf->currentAttributeEntryIndex++;
        }
    }

    return 1;
}
