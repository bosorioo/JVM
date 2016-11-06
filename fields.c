#include "fields.h"
#include "readfunctions.h"
#include "validity.h"
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

        jcf->currentAttributeEntryIndex = -1;

        for (i = 0; i < entry->attributes_count; i++)
        {
            if (!readAttribute(jcf, entry->attributes + i))
                return 0;

            jcf->currentAttributeEntryIndex++;
        }
    }

    return 1;
}
