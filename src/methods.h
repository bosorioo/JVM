#ifndef METHODS_H
#define METHODS_H

typedef struct method_info method_info;

#include <stdint.h>
#include "javaclass.h"
#include "attributes.h"

struct method_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info* attributes;
};

char readMethod(JavaClass* jc, method_info* entry);
void freeMethodAttributes(method_info* entry);
void printMethods(JavaClass* jc);

method_info* getMethodMatching(JavaClass* jc, const uint8_t* name, int32_t name_len, const uint8_t* descriptor,
                               int32_t descriptor_len, uint16_t flag_mask);

#endif // METHODS_H
