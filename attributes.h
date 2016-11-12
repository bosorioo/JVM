#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

typedef struct attribute_info attribute_info;

#include <stdint.h>
#include "javaclassfile.h"

struct attribute_info {
    uint16_t name_index;
    uint32_t length;
    uint8_t* info;
};

char readAttribute(JavaClassFile* jcf, attribute_info* entry);
void freeAttributeInfo(attribute_info* entry);
void printAttribute(JavaClassFile* jcf, attribute_info* entry, int identationLevel);
void printAllAttributes(JavaClassFile* jcf);

#endif // ATTRIBUTES_H
