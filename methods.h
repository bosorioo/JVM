#ifndef METHODS_H
#define METHODS_H

typedef struct method_info method_info;

#include <stdint.h>
#include "javaclassfile.h"
#include "attributes.h"

typedef struct method_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info* attributes;
} method_info;

char readMethod(JavaClassFile* jcf, method_info* entry);
void freeMethodAttributes(method_info* entry);

#endif // METHODS_H
