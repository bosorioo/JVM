#ifndef METHODAREA_H
#define METHODAREA_H

typedef struct method_area method_area;

#include <stdint.h>
#include "javaclass.h"

typedef struct ClassLinkedList
{
    JavaClass* jc;
    struct ClassLinkedList* next;
} ClassLinkedList;

struct method_area
{
    ClassLinkedList* classes;
};

void initMethodArea(method_area* ma);
uint8_t addClassToMethodArea(method_area* ma, JavaClass* jc);
uint8_t isClassLoaded(method_area* ma, const uint8_t* className_utf8_bytes, int32_t utf8_len);

#endif // METHODAREA_H
