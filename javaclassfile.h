#ifndef JAVACLASSFILE_H
#define JAVACLASSFILE_H

typedef struct JavaClassFile JavaClassFile;

#include <stdio.h>
#include <stdint.h>
#include "constantpool.h"
#include "attributes.h"
#include "fields.h"
#include "methods.h"

enum AccessFlagsType {
    ACCT_CLASS,
    ACCT_FIELD,
    ACCT_METHOD
};

enum AccessFlags {

    ACC_PUBLIC          = 0x0001, // Class, Field, Method
    ACC_PRIVATE         = 0x0002, // Field, Method
    ACC_PROTECTED       = 0x0004, // Field, Method
    ACC_STATIC          = 0x0008, // Field, Method
    ACC_FINAL           = 0x0010, // Class, Field, Method
    ACC_SUPER           = 0x0020, // Class
    ACC_SYNCHRONIZED    = 0x0020, // Method
    ACC_VOLATILE        = 0x0040, // Field
    ACC_TRANSIENT       = 0x0080, // Field
    ACC_NATIVE          = 0x0100, // Method
    ACC_INTERFACE       = 0x0200, // Class
    ACC_ABSTRACT        = 0x0400, // Class, Method
    ACC_STRICT          = 0x0800, // Method

    ACC_INVALID_CLASS_FLAG_MASK = ~(ACC_PUBLIC | ACC_FINAL | ACC_SUPER | ACC_INTERFACE | ACC_ABSTRACT),

    ACC_INVALID_FIELD_FLAG_MASK = ~(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL |
                                    ACC_VOLATILE | ACC_TRANSIENT),

    ACC_INVALID_METHOD_FLAG_MASK = ~(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL |
                                     ACC_SYNCHRONIZED | ACC_NATIVE | ACC_ABSTRACT | ACC_STRICT)

};

enum JavaClassStatus {
    STATUS_OK,
    FILE_COULDNT_BE_OPENED,
    INVALID_SIGNATURE,
    CLASS_NAME_FILE_NAME_MISMATCH,
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
    FILE_CONTAINS_UNEXPECTED_DATA
};

struct JavaClassFile {
    // General
    FILE* file;
    enum JavaClassStatus status;

    // Java
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

    // Debug info
    uint32_t totalBytesRead;
    uint8_t lastTagRead;
    int32_t currentConstantPoolEntryIndex;
    int32_t constantPoolEntriesRead;
    int32_t currentInterfaceEntryIndex;
    int32_t currentFieldEntryIndex;
    int32_t currentMethodEntryIndex;
    int32_t currentAttributeEntryIndex;
    int32_t attributeEntriesRead;
    int32_t currentValidityEntryIndex;

};

void openClassFile(JavaClassFile* jcf, const char* path);
void closeClassFile(JavaClassFile* jcf);
const char* decodeJavaClassFileStatus(enum JavaClassStatus);
void decodeAccessFlags(uint16_t flags, char* buffer, int32_t buffer_len, enum AccessFlagsType acctype);

void printClassFileDebugInfo(JavaClassFile* jcf);
void printClassFileInfo(JavaClassFile* jcf);

#endif // JAVACLASSFILE_H
