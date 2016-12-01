#ifndef MEMORYINSPECT_H
#define MEMORYINSPECT_H

#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG

    void* memalloc(size_t bytes, const char* file, int line, const char* call);
    void  memfree(void* ptr, const char* file, int line, const char* call);
    #define malloc(bytes) memalloc(bytes, __FILE__, __LINE__, #bytes)
    #define free(ptr) memfree(ptr, __FILE__, __LINE__, #ptr)

#include "operandstack.h"
#include "methods.h"
#include "framestack.h"
#include "jvm.h"

    int debugGetFrameId(Frame* frame);
    void debugPrintMethod(JavaClass* jc, method_info* method);
    void debugPrintMethodFieldRef(JavaClass* jc, cp_info* cpi);
    void debugPrintOperandStack(OperandStack* os);
    void debugPrintLocalVariables(int32_t* localVars, uint16_t count);
    void debugPrintNewObject(Reference* obj);
#endif

#endif // MEMORYINSPECT_H
