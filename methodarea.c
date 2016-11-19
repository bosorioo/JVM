#include "methodarea.h"
#include "utf8.h"
#include <stdlib.h>

void initMethodArea(method_area* ma)
{
    ma->classes = NULL;
}

uint8_t addClassToMethodArea(method_area* ma, JavaClass* jc)
{
    ClassLinkedList* node = (ClassLinkedList*)malloc(sizeof(ClassLinkedList));

    if (node)
    {
        node->jc = jc;
        node->next = ma->classes;
        ma->classes = node;
    }

    return node != NULL;
}

uint8_t isClassLoaded(method_area* ma, const uint8_t* className_utf8_bytes, int32_t utf8_len)
{
    ClassLinkedList* classes = ma->classes;
    JavaClass* jc;
    cp_info* cpi;

    while (classes)
    {
        jc = classes->jc;
        cpi = jc->constantPool + jc->thisClass - 1;
        cpi = jc->constantPool + cpi->Class.name_index - 1;

        if (cmp_UTF8_Ascii(cpi->Utf8.bytes, cpi->Utf8.length, className_utf8_bytes, utf8_len))
            return 1;

        classes = classes->next;
    }

    return 0;
}
