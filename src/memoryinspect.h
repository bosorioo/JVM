#ifndef MEMORYINSPECT_H
#define MEMORYINSPECT_H

#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG
    void* memalloc(size_t bytes, const char* file, int line, const char* call);
    void  memfree(void* ptr, const char* file, int line, const char* call);
    #define malloc(bytes) memalloc(bytes, __FILE__, __LINE__, #bytes)
    #define free(ptr) memfree(ptr, __FILE__, __LINE__, #ptr)
#endif

#endif // MEMORYINSPECT_H
