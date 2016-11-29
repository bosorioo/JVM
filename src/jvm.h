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
    JVM_STATUS_MAIN_CLASS_RESOLUTION_FAILED,
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
    int32_t* data;
} ClassInstance;

typedef struct String
{
    uint8_t* utf8_bytes;
    uint32_t len;
} String;

typedef struct Array
{
    uint32_t length;
    uint8_t* data;
    Opcode_newarray_type type;
} Array;

typedef struct ObjectArray
{
    uint32_t length;
    uint8_t* utf8_className;
    int32_t utf8_len;
    Reference** elements;
} ObjectArray;

typedef enum ReferenceType {
     REFTYPE_ARRAY,
     REFTYPE_CLASSINSTANCE,
     REFTYPE_OBJARRAY,
     REFTYPE_STRING
} ReferenceType;

struct Reference
{
    ReferenceType type;

    union {
        ClassInstance ci;
        Array arr;
        ObjectArray oar;
        String str;
    };
};

typedef struct ReferenceTable
{
    Reference* obj;
    struct ReferenceTable* next;
} ReferenceTable;

/// @brief Linked list data struct that holds information about a
/// class that has already been resolved.
typedef struct LoadedClasses
{
    /// @brief Pointer to the JavaClass struct of the resolved class.
    JavaClass* jc;

    /// @brief Boolean telling if the class has already been initialized
    /// or if it still needs to be.
    ///
    /// Initialization of a class is done by calling the method <clinit>
    /// after reserving space for the static fields used by the class.
    /// Static fields with 'ConstantValue' attribute have their value
    /// set during class initialization.
    /// @see initClass()
    uint8_t requiresInit;

    /// @brief Array containing the data for the static fields of the class.
    int32_t* staticFieldsData;

    /// @brief Pointer to the next node of the linked list.
    struct LoadedClasses* next;
} LoadedClasses;

/// @brief A java virtual machine, storing all loaded classes, created
/// objects and frames for methods being executed.
/// @see initJVM(), executeJVM(), deinitJVM()
struct JavaVirtualMachine
{
    /// @brief Status of the execution of the JVM, indicating whether there
    /// are errors or not.
    uint8_t status;

    /// @brief Boolean telling if the System/String
    /// classes are to be simulated instead of
    /// being loaded from their actual .class files.
    /// @note Currently it is always set to true, and the
    /// support for those classes is minimum.
    uint8_t simulatingSystemAndStringClasses;

    /// @brief Linked list of all objects that have been
    /// created during the execution of the JVM.
    ReferenceTable* objects;

    /// @brief Stack of all frames created by method calls.
    FrameStack* frames;

    /// @brief Linked list containing all classes that have been
    /// resolved by the JVM.
    LoadedClasses* classes;

    /// @brief Path to look for files when opening classes.
    ///
    /// If an attempt to open a class file in the
    /// current directory fails, then this classPath
    /// is used to build another directory to look for
    /// the class file.
    char classPath[256];
};

void initJVM(JavaVirtualMachine* jvm);
void deinitJVM(JavaVirtualMachine* jvm);
void executeJVM(JavaVirtualMachine* jvm, LoadedClasses* mainClass);
void setClassPath(JavaVirtualMachine* jvm, const char* path);
uint8_t resolveClass(JavaVirtualMachine* jvm, const uint8_t* className_utf8_bytes, int32_t utf8_len, LoadedClasses** outClass);
uint8_t resolveMethod(JavaVirtualMachine* jvm, JavaClass* jc, cp_info* cp_method, LoadedClasses** outClass);
uint8_t resolveField(JavaVirtualMachine* jvm, JavaClass* jc, cp_info* cp_field, LoadedClasses** outClass);
uint8_t runMethod(JavaVirtualMachine* jvm, JavaClass* jc, method_info* method, uint8_t numberOfParameters);
uint8_t getMethodDescriptorParameterCount(const uint8_t* descriptor_utf8, int32_t utf8_len);

LoadedClasses* addClassToLoadedClasses(JavaVirtualMachine* jvm, JavaClass* jc);
LoadedClasses* isClassLoaded(JavaVirtualMachine* jvm, const uint8_t* utf8_bytes, int32_t utf8_len);
JavaClass* getSuperClass(JavaVirtualMachine* jvm, JavaClass* jc);
uint8_t isClassSuperOf(JavaVirtualMachine* jvm, JavaClass* super, JavaClass* jc);
uint8_t initClass(JavaVirtualMachine* jvm, LoadedClasses* lc);

Reference* newString(JavaVirtualMachine* jvm, const uint8_t* str, int32_t strlen);
Reference* newClassInstance(JavaVirtualMachine* jvm, LoadedClasses* jc);
Reference* newArray(JavaVirtualMachine* jvm, uint32_t length, Opcode_newarray_type type);
Reference* newObjectArray(JavaVirtualMachine* jvm, uint32_t length, const uint8_t* utf8_className, int32_t utf8_len);
Reference* newObjectMultiArray(JavaVirtualMachine* jvm, int32_t* dimensions, uint8_t dimensionsSize,
                               const uint8_t* utf8_className, int32_t utf8_len);

void deleteReference(Reference* obj);

/// @brief Macro used to print faults in instructions.
///
/// This macro is used in instructions that aren't
/// implemented or have missing features, like
/// exception throwing. Is is just a collection
/// of prints telling the user that an error occured
/// during the execution of that instruction.
 #define DEBUG_REPORT_INSTRUCTION_ERROR \
    printf("\nAbortion request by instruction at %s:%u.\n", __FILE__, __LINE__); \
    printf("Check at the source file what the cause could be.\n"); \
    printf("It could be an exception that was supposed to be thrown or an unsupported feature.\n"); \
    printf("Execution will proceed, but others instruction will surely request abortion.\n\n");

#endif // JVM_H

/// @defgroup jvm JVM module
///
/// @brief Defines a structure to hold the necessary information
/// to execute class files.
///
/// The main struct of this module is a JavaVirtualMachine.
/// It holds all data necessary for the execution of class file.
///
/// To setup the virtual machine, a call to initJVM() is made.
/// When execution is finished, a call to deinitJVM() will release
/// any resources associated with the JVM.
///
/// The class holding a "public static void main(String[])" method
/// must be resolved before being executed, which can be done with
/// resolveClass().
///
/// Any other class files that the entry class file uses is resolved
/// automatically during run-time.
///
/// @see jvm.c
