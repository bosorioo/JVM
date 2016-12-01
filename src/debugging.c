#ifdef DEBUG

#include "debugging.h"
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

#else

/// @file memoryinspect.c
/// @brief This source file is for debugging purposes only.
/// Since DEBUG isn't defined, this module becomes empty.

#endif
