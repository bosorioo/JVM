#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <stdint.h>
#include <stdio.h>
#include "javaclassfile.h"
#include "constantpool.h"

uint8_t readu4(struct JavaClassFile* jcf, uint32_t* out);
uint8_t readu2(struct JavaClassFile* jcf, uint16_t* out);
int32_t readFieldDescriptor(uint8_t* utf8_bytes, int32_t utf8_len, char checkValidClassIdentifier);
int32_t readMethodDescriptor(uint8_t* utf8_bytes, int32_t utf8_len, char checkValidClassIdentifier);
float readConstantPoolFloat(cp_info* entry);
double readConstantPoolDouble(cp_info* entry);

#endif // FILEPARSER_H
