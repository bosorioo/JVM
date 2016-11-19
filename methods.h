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

method_info* getMethodByName(JavaClass* jc, const char* methodName);

#endif // METHODS_H
