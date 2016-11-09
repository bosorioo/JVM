#ifndef FIELDS_H
#define FIELDS_H

typedef struct field_info field_info;

#include <stdint.h>
#include "javaclassfile.h"
#include "attributes.h"

struct field_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info* attributes;
};

char readField(JavaClassFile* jcf, field_info* entry);
void freeFieldAttributes(field_info* entry);

#endif // FIELDS_H
