#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

typedef struct attribute_info attribute_info;

#include <stdint.h>
#include "javaclassfile.h"

typedef struct attribute_info {
    uint16_t name_index;
    uint32_t length;
    uint8_t* info;
} attribute_info;

char readAttribute(JavaClassFile* jcf, attribute_info* entry);

#endif // ATTRIBUTES_H
