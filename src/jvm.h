#ifndef JVM_H
#define JVM_H

typedef struct JavaVirtualMachine JavaVirtualMachine;
typedef struct Reference Reference;

#include <stdint.h>
#include "javaclass.h"
#include "opcodes.h"
#include "framestack.h"

enum JVMStatus {
    JVM_STATUS_OK,
    JVM_STATUS_NO_CLASS_LOADED,
    JVM_STATUS_CLASS_RESOLUTION_FAILED,
    JVM_STATUS_METHOD_RESOLUTION_FAILED,
    JVM_STATUS_FIELD_RESOLUTION_FAILED,
    JVM_STATUS_UNKNOWN_INSTRUCTION,
    JVM_STATUS_OUT_OF_MEMORY,
    JVM_STATUS_MAIN_METHOD_NOT_FOUND,
    JVM_STATUS_INVALID_INSTRUCTION_PARAMETERS
};

typedef struct ClassInstance
{
    JavaClass* c;
    uint8_t* data;
} ClassInstance;

typedef struct Array
{
    uint8_t dimensions;
    uint32_t* dimensionsLength;
    uint8_t* data;
    Opcode_newarray_type type;
} Array;

typedef struct ObjectArray
{
    uint8_t dimensions;
    uint32_t* dimensionsLength;
    Reference* elements;
} ObjectArray;

typedef enum ReferenceType {
     REFTYPE_ARRAY, REFTYPE_CLASSINSTANCE, REFTYPE_OBJARRAY
} ReferenceType;

struct Reference
{
    ReferenceType type;

    union {
        ClassInstance ci;
        Array arr;
        ObjectArray oar;
    };

};

typedef struct ReferenceTable
{
    Reference* obj;
    struct ReferenceTable* next;
} ReferenceTable;

typedef struct LoadedClasses
{
    JavaClass* jc;
    int32_t* staticFieldsData;
    struct LoadedClasses* next;
} LoadedClasses;

struct JavaVirtualMachine
{
    uint8_t status;

    ReferenceTable* objects;
    FrameStack* frames;
    LoadedClasses* classes;
};

void initJVM(JavaVirtualMachine* jvm);
void deinitJVM(JavaVirtualMachine* jvm);
void executeJVM(JavaVirtualMachine* jvm, JavaClass* mainClass);
uint8_t resolveClass(JavaVirtualMachine* jvm, const uint8_t* className_utf8_bytes, int32_t utf8_len, LoadedClasses** outClass);
uint8_t resolveMethod(JavaVirtualMachine* jvm, JavaClass* jc, cp_info* cp_method, LoadedClasses** outClass);
uint8_t resolveField(JavaVirtualMachine* jvm, JavaClass* jc, cp_info* cp_field, LoadedClasses** outClass);
uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method, uint8_t numberOfParameters);
LoadedClasses* addClassToLoadedClasses(JavaVirtualMachine* jvm, JavaClass* jc);
LoadedClasses* isClassLoaded(JavaVirtualMachine* jvm, const uint8_t* utf8_bytes, int32_t utf8_len);

Reference* newClassInstance(JavaVirtualMachine* jvm, JavaClass* jc);

#ifdef DEBUG
#define DEBUG_REPORT_INSTRUCTION_ERROR printf("Abortion request by instruction at %s:%d.\n", __FILE__, __LINE__);
#else
#define DEBUG_REPORT_INSTRUCTION_ERROR
#endif // DEBUG

#endif // JVM_H
