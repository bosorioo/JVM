#ifndef JVM_H
#define JVM_H

typedef struct JavaVirtualMachine JavaVirtualMachine;

#include <stdint.h>
#include "javaclass.h"
#include "framestack.h"

enum JVMStatus {
    JVM_STATUS_OK,
    JVM_STATUS_NO_CLASS_LOADED,
    JVM_STATUS_CLASS_RESOLUTION_FAILED,
    JVM_STATUS_UNKNOWN_INSTRUCTION,
    JVM_STATUS_ABORTED,
    JVM_STATUS_OUT_OF_MEMORY,
};

typedef struct LoadedClasses
{
    JavaClass* jc;
    struct LoadedClasses* next;
} LoadedClasses;

struct JavaVirtualMachine
{
    uint8_t status;

    FrameStack* frames;
    LoadedClasses* classes;
};

void initJVM(JavaVirtualMachine* jvm);
void deinitJVM(JavaVirtualMachine* jvm);
void executeJVM(JavaVirtualMachine* jvm);
uint8_t resolveClass(JavaVirtualMachine* jvm, const uint8_t* className_utf8_bytes, int32_t utf8_len);
uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method);
uint8_t addClassToLoadedClasses(JavaVirtualMachine* jvm, JavaClass* jc);
uint8_t isClassLoaded(JavaVirtualMachine* jvm, const uint8_t* utf8_bytes, int32_t utf8_len);

#endif // JVM_H
