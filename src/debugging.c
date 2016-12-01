#ifdef DEBUG

#include "debugging.h"
#include "utf8.h"
#include <stdint.h>

#undef malloc
#undef free
#define _malloc(b) malloc(b)
#define _free(p) free(p)

typedef struct MemoryPtrStack
{
    void* ptr;
    char file[256];
    char call[256];
    int line;
    size_t bytes;
    struct MemoryPtrStack* next;
} MemoryPtrStack;

MemoryPtrStack* _MEMSTACK = NULL;

void checkMemoryLeak(void)
{
    printf("\n#### Memory Inspect Report ####\n");

    if (_MEMSTACK == NULL)
    {
        printf("No memory leak.\n");
        return;
    }

    MemoryPtrStack* node = _MEMSTACK;

    printf("Memory leak detected.\n");

    while (node)
    {
        printf("  Leak on %s:%d of %d - %s\n", node->file, node->line, node->bytes, node->call);
        node = node->next;
    }
}

void* memalloc(size_t bytes, const char* file, int line, const char* call)
{
    static char init = 1;

    if (init)
    {
        init = 0;
        atexit(checkMemoryLeak);
    }
    void* ptr = _malloc(bytes);

    if (ptr)
    {
        MemoryPtrStack* node = (MemoryPtrStack*)_malloc(sizeof(MemoryPtrStack));

        if (node)
        {
            snprintf(node->file, sizeof(node->file), "%s", file);
            snprintf(node->call, sizeof(node->call), "%s", call);
            node->line = line;
            node->ptr = ptr;
            node->bytes = bytes;
            node->next = _MEMSTACK;
            _MEMSTACK = node;
        }
        else
        {
            _free(ptr);
            return NULL;
        }
    }

    return ptr;
}

void memfree(void* ptr, const char* file, int line, const char* call)
{
    MemoryPtrStack* node = _MEMSTACK;
    MemoryPtrStack* previous = NULL;

    while (node)
    {
        if (node->ptr == ptr)
        {
            if (node == _MEMSTACK)
                _MEMSTACK = node->next;
            else
                previous->next = node->next;

            _free(node);
            _free(ptr);
            return;
        }

        previous = node;
        node = node->next;
    }

    printf("\n\n#### Memory Inspect Warning ####\nAttempt to free invalid pointer\n");
    printf(" At %s:%d, call: %s\n\n", file, line, call);
    _free(ptr);
}

#define malloc(bytes) memalloc(bytes)
#define free(ptr) memfree(ptr)

int debugGetFrameId(Frame* frame)
{
    static Frame* frameBuffer[4096];
    static int frameCount = 0;

    if (frame == NULL)
    {
        if (frameCount > 0)
            frameCount--;

        return 0;
    }

    int i;

    for (i = 0; i < frameCount; i++)
    {
        if (frameBuffer[i] == frame)
            return i;
    }

    frameBuffer[frameCount++] = frame;
    return i;
}

void debugPrintMethod(JavaClass* jc, method_info* method)
{
    char accessFlags[256];
    decodeAccessFlags(method->access_flags, accessFlags, sizeof(accessFlags), ACCT_METHOD);
    cp_info* name = jc->constantPool + method->name_index - 1;
    cp_info* descriptor = jc->constantPool + method->descriptor_index - 1;
    printf("%s %.*s%.*s",  accessFlags, PRINT_UTF8(name), PRINT_UTF8(descriptor));
}

void debugPrintMethodFieldRef(JavaClass* jc, cp_info* cpi)
{
    char buffer[256];
    uint32_t length = 0;

    cp_info* debugcpi = jc->constantPool + cpi->Methodref.class_index - 1;
    debugcpi = jc->constantPool + debugcpi->Class.name_index - 1;

    length += snprintf(buffer + length, sizeof(buffer) - length, "%.*s.", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
    debugcpi = jc->constantPool + cpi->Methodref.name_and_type_index - 1;
    debugcpi = jc->constantPool + debugcpi->NameAndType.name_index - 1;

    length += snprintf(buffer + length, sizeof(buffer) - length, "%.*s:", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
    debugcpi = jc->constantPool + cpi->Methodref.name_and_type_index - 1;
    debugcpi = jc->constantPool + debugcpi->NameAndType.descriptor_index - 1;

    length += snprintf(buffer + length, sizeof(buffer) - length, "%.*s", debugcpi->Utf8.length, debugcpi->Utf8.bytes);
    printf("%s", buffer);
}

void debugPrintOperandStack(OperandStack* node)
{
    printf("Operand stack:");

    if (!node)
    {
        printf(" empty.\n");
        return;
    }

    printf("\n");
    char separate = 0;

    while (node)
    {
        if (separate)
            printf(" -> ");
        else
            separate = 1;

        switch (node->type)
        {
            case OP_REFERENCE:
            {
                Reference* obj = (Reference*)node->value;
                printf("obj:%d", node->value);

                if (obj)
                {
                    switch(obj->type)
                    {
                        case REFTYPE_ARRAY: printf(" (array)"); break;
                        case REFTYPE_CLASSINSTANCE: printf(" (instance)"); break;
                        case REFTYPE_OBJARRAY: printf(" (obj array)"); break;
                        case REFTYPE_STRING: printf(" (string)"); break;
                        default:
                            break;
                    }
                }

                break;
            }

            default:
                printf("%d", node->value);
        }

        node = node->next;
    }

    printf("\n");
}

void debugPrintLocalVariables(int32_t* localVars, uint16_t count)
{
    printf("Local variables:");

    if (!localVars)
    {
        printf(" empty.\n");
        return;
    }

    char separate = 0;
    uint16_t i;

    for (i = 0; i < count; i++)
    {
        if (separate)
            printf(",");
        else
            separate = 1;

        printf(" %d", localVars[i]);
    }

    printf("\n");

}

void debugPrintNewObject(Reference* obj)
{
    printf("New object created, obj:%d", (int32_t)obj);

    switch(obj->type)
    {
        case REFTYPE_ARRAY: printf(" (array)"); break;
        case REFTYPE_CLASSINSTANCE: printf(" (instance)"); break;
        case REFTYPE_OBJARRAY: printf(" (obj array)"); break;
        case REFTYPE_STRING: printf(" (string)"); break;
        default:
            break;
    }

    printf("\n");
}

#else

/// @file memoryinspect.c
/// @brief This source file is for debugging purposes only.
/// Since DEBUG isn't defined, this module becomes empty.

#endif
