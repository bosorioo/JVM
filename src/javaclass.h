#ifndef JAVACLASSFILE_H
#define JAVACLASSFILE_H

typedef struct JavaClass JavaClass;

#include <stdio.h>
#include <stdint.h>
#include "constantpool.h"
#include "attributes.h"
#include "fields.h"
#include "methods.h"

enum AccessFlagsType {
    ACCT_CLASS,
    ACCT_FIELD,
    ACCT_METHOD,
    ACCT_INNERCLASS
};

enum AccessFlags {

    ACC_PUBLIC          = 0x0001, // Class, Field, Method, InnerClass
    ACC_PRIVATE         = 0x0002, // Field, Method, InnerClass
    ACC_PROTECTED       = 0x0004, // Field, Method, InnerClass
    ACC_STATIC          = 0x0008, // Field, Method, InnerClass
    ACC_FINAL           = 0x0010, // Class, Field, Method, InnerClass
    ACC_SUPER           = 0x0020, // Class
    ACC_SYNCHRONIZED    = 0x0020, // Method
    ACC_BRIDGE          = 0x0040, // Method
    ACC_VOLATILE        = 0x0040, // Field
    ACC_TRANSIENT       = 0x0080, // Field
    ACC_VARARGS         = 0x0080, // Method
    ACC_NATIVE          = 0x0100, // Method
    ACC_INTERFACE       = 0x0200, // Class, InnerClass
    ACC_ABSTRACT        = 0x0400, // Class, Method, InnerClass
    ACC_STRICT          = 0x0800, // Method
    ACC_SYNTHETIC       = 0x1000, // Method

    ACC_INVALID_CLASS_FLAG_MASK = ~(ACC_PUBLIC | ACC_FINAL | ACC_SUPER | ACC_INTERFACE | ACC_ABSTRACT),

    ACC_INVALID_FIELD_FLAG_MASK = ~(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL |
                                    ACC_VOLATILE | ACC_TRANSIENT),

    ACC_INVALID_METHOD_FLAG_MASK = ~(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL |
                                     ACC_SYNCHRONIZED | ACC_NATIVE | ACC_ABSTRACT | ACC_STRICT |
                                     ACC_BRIDGE | ACC_VARARGS | ACC_SYNTHETIC),

    ACC_INVALID_INNERCLASS_FLAG_MASK = ~(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC |
                                         ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT)

};

enum JavaClassStatus {
    CLASS_STATUS_OK,
    CLASS_STATUS_UNSUPPORTED_VERSION,
    CLASS_STATUS_FILE_COULDNT_BE_OPENED,
    CLASS_STATUS_INVALID_SIGNATURE,
    MEMORY_ALLOCATION_FAILED,
    INVALID_CONSTANT_POOL_COUNT,
    UNEXPECTED_EOF,
    UNEXPECTED_EOF_READING_CONSTANT_POOL,
    UNEXPECTED_EOF_READING_UTF8,
    UNEXPECTED_EOF_READING_INTERFACES,
    UNEXPECTED_EOF_READING_ATTRIBUTE_INFO,
    INVALID_UTF8_BYTES,
    INVALID_CONSTANT_POOL_INDEX,
    UNKNOWN_CONSTANT_POOL_TAG,

    INVALID_ACCESS_FLAGS,
    USE_OF_RESERVED_CLASS_ACCESS_FLAGS,
    USE_OF_RESERVED_METHOD_ACCESS_FLAGS,
    USE_OF_RESERVED_FIELD_ACCESS_FLAGS,

    INVALID_THIS_CLASS_INDEX,
    INVALID_SUPER_CLASS_INDEX,
    INVALID_INTERFACE_INDEX,

    INVALID_FIELD_DESCRIPTOR_INDEX,
    INVALID_METHOD_DESCRIPTOR_INDEX,
    INVALID_NAME_INDEX,
    INVALID_STRING_INDEX,
    INVALID_CLASS_INDEX,
    INVALID_NAME_AND_TYPE_INDEX,
    INVALID_JAVA_IDENTIFIER,

    ATTRIBUTE_LENGTH_MISMATCH,
    ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX,
    ATTRIBUTE_INVALID_SOURCEFILE_INDEX,
    ATTRIBUTE_INVALID_INNERCLASS_INDEXES,
    ATTRIBUTE_INVALID_EXCEPTIONS_CLASS_INDEX,
    ATTRIBUTE_INVALID_CODE_LENGTH,

    FILE_CONTAINS_UNEXPECTED_DATA
};

struct JavaClass {
    // General
    FILE* file;
    enum JavaClassStatus status;
    uint8_t classNameMismatch;

    // Java Class Specification
    uint16_t minorVersion, majorVersion;
    uint16_t constantPoolCount;
    cp_info* constantPool;
    uint16_t accessFlags;
    uint16_t thisClass;
    uint16_t superClass;
    uint16_t interfaceCount;
    uint16_t* interfaces;
    uint16_t fieldCount;
    field_info* fields;
    uint16_t methodCount;
    method_info* methods;
    uint16_t attributeCount;
    attribute_info* attributes;

    // Class Data Info
    uint16_t staticFieldCount;
    uint16_t instanceFieldCount;

    // Debug info
    uint32_t totalBytesRead;
    uint8_t lastTagRead;
    int32_t constantPoolEntriesRead;
    int32_t interfaceEntriesRead;
    int32_t fieldEntriesRead;
    int32_t methodEntriesRead;
    int32_t attributeEntriesRead;
    int32_t validityEntriesChecked;

};

void openClassFile(JavaClass* jc, const char* path);
void closeClassFile(JavaClass* jc);
const char* decodeJavaClassStatus(enum JavaClassStatus);
void decodeAccessFlags(uint16_t flags, char* buffer, int32_t buffer_len, enum AccessFlagsType acctype);

void printClassFileDebugInfo(JavaClass* jc);
void printClassFileInfo(JavaClass* jc);

#endif // JAVACLASSFILE_H
