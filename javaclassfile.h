#ifndef JAVACLASSFILE_H
#define JAVACLASSFILE_H

typedef struct JavaClassFile JavaClassFile;

#include <stdio.h>
#include <stdint.h>
#include "constantpool.h"
#include "attributes.h"
#include "fields.h"
#include "methods.h"

enum ClassAccessFlags {

    ACC_PUBLIC      = 0x0001, // Class and Field
    ACC_PRIVATE     = 0x0002, // Field only
    ACC_PROTECTED   = 0x0004, // Field only
    ACC_STATIC      = 0x0008, // Field only
    ACC_FINAL       = 0x0010, // Class and Field
    ACC_SUPER       = 0x0020, // Class only
    ACC_VOLATILE    = 0x0040, // Field only
    ACC_TRANSIENT   = 0x0080, // Field only
    ACC_INTERFACE   = 0x0200, // Class only
    ACC_ABSTRACT    = 0x0400, // Class only

    ACC_INVALID_CLASS_FLAG_MASK = ~(ACC_PUBLIC | ACC_FINAL | ACC_SUPER | ACC_INTERFACE | ACC_ABSTRACT),
    ACC_INVALID_FIELD_FLAG_MASK = ~(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL | ACC_VOLATILE | ACC_TRANSIENT)
};

enum JavaClassStatus {
    STATUS_OK,
    FILE_COULDNT_BE_OPENED,
    INVALID_SIGNATURE,
    CLASS_FILE_NAME_INVALID,
    CLASS_FILE_NAME_TOO_LONG,
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
    USE_OF_RESERVED_ACCESS_FLAGS,
    INVALID_THIS_CLASS_INDEX,
    INVALID_SUPER_CLASS_INDEX,
    INVALID_INTERFACE_INDEX,
    INVALID_FIELD_NAME_INDEX,
    INVALID_FIELD_DESCRIPTOR_INDEX,
    FILE_CONTAINS_UNEXPECTED_DATA
};

typedef struct JavaClassFile {
    FILE* file;
    enum JavaClassStatus status;
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

    uint16_t currentConstantPoolEntryIndex;
    uint16_t currentInterfaceEntryIndex;
    uint16_t currentFieldEntryIndex;
    uint16_t currentMethodEntryIndex;
    uint16_t currentAttributeEntryIndex;

} JavaClassFile;

void openClassFile(JavaClassFile* jcf, const char* path);
void freeClassFile(JavaClassFile* jcf);
const char* decodeJavaClassFileStatus(enum JavaClassStatus);

void printGeneralInfo(JavaClassFile* jcf);


#endif // JAVACLASSFILE_H
