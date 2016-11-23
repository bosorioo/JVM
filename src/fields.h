#ifndef FIELDS_H
#define FIELDS_H

typedef struct field_info field_info;

#include <stdint.h>
#include "javaclass.h"
#include "attributes.h"

struct field_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info* attributes;

    // Offset is used to identify in which byte offset
    // this field is stored (in the static data area of
    // a class or in the instance attribute area for class
    // instances).
    uint16_t offset;
};

char readField(JavaClass* jc, field_info* entry);
void freeFieldAttributes(field_info* entry);
void printAllFields(JavaClass* jc);

field_info* getFieldMatchingUTF8(JavaClass* jc, const uint8_t* name, int32_t name_len, const uint8_t* descriptor,
                                 int32_t descriptor_len, uint16_t flag_mask);

#endif // FIELDS_H
