#include "attributes.h"
#include "readfunctions.h"
#include <stdlib.h>

char readAttribute(JavaClassFile* jcf, attribute_info* entry)
{
    entry->info = NULL;

    if (!readu2(jcf, &entry->name_index) ||
        !readu4(jcf, &entry->length))
    {
        jcf->status = UNEXPECTED_EOF;
        return 0;
    }

    if (entry->length > 0)
    {

        // TODO: avoid memory allocation if attribute is not supported

        entry->info = (uint8_t*)malloc(entry->length);

        if (!entry->info)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint32_t i;
        int byte;

        for (i = 0; i < entry->length; i++)
        {
            byte = fgetc(jcf->file);

            if (byte == EOF)
            {
                jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
                return 0;
            }

            *(entry->info + i) = (uint8_t)byte;
            jcf->totalBytesRead++;
        }
    }


    return 1;
}

void freeAttributeInfo(attribute_info* entry)
{
    if (entry->info)
    {
        free(entry->info);
        entry->info = NULL;
    }
}
