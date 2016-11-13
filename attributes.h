#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

typedef struct attribute_info attribute_info;

#include <stdint.h>
#include "javaclassfile.h"

struct attribute_info {
    uint16_t name_index;
    uint32_t length;
    void* info;
    uint8_t attributeType;
};

enum AttributeType {
    ATTR_Unknown = 0,
    ATTR_ConstantValue,
    ATTR_SourceFile,
    ATTR_InnerClasses,
    ATTR_Code,
    ATTR_LineNumberTable,
    ATTR_Exceptions,
    ATTR_Deprecated
};

typedef struct {
    uint16_t sourcefile_index;
} att_SourceFile_info;

typedef struct {
    uint16_t constantvalue_index;
} att_ConstantValue_info;

typedef struct {
    uint16_t inner_class_index;
    uint16_t outer_class_index;
    uint16_t inner_class_name_index;
    uint16_t inner_class_access_flags;
} InnerClassInfo;

typedef struct {
    uint16_t number_of_classes;
    InnerClassInfo* inner_classes;
} att_InnerClasses_info;

typedef struct {
    uint16_t start_pc;
    uint16_t line_number;
} LineNumberTableEntry;

typedef struct {
    uint16_t line_number_table_length;
    LineNumberTableEntry* line_number_table;
} att_LineNumberTable_info;

typedef struct {
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type;
} ExceptionTableEntry;

typedef struct {
    uint16_t max_stack;
    uint16_t max_locals;
    uint32_t code_length;
    uint8_t* code;
    uint16_t exception_table_length;
    ExceptionTableEntry* exception_table;
    uint16_t attributes_count;
    attribute_info* attributes;
} att_Code_info;

typedef struct {
    uint16_t number_of_exceptions;
    uint16_t* exception_index_table;
} att_Exceptions_info;

char readAttribute(JavaClassFile* jcf, attribute_info* entry);
void freeAttributeInfo(attribute_info* entry);
void printAttribute(JavaClassFile* jcf, attribute_info* entry, int identationLevel);
void printAllAttributes(JavaClassFile* jcf);

#endif // ATTRIBUTES_H
