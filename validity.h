#ifndef VALIDITY_H
#define VALIDITY_H

#include "javaclass.h"

char checkMethodAccessFlags(JavaClass* jc, uint16_t acessFlags);
char checkFieldAccessFlags(JavaClass* jc, uint16_t acessFlags);
char checkClassIndexAndAccessFlags(JavaClass* jc);
char checkClassNameFileNameMatch(JavaClass* jc, const char* classFilePath);
char isValidJavaIdentifier(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t isClassIdentifier);
char isValidNameIndex(JavaClass* jc, uint16_t name_index, uint8_t isClassIdentifier);
char isValidMethodNameIndex(JavaClass* jc, uint16_t name_index);
char checkConstantPoolValidity(JavaClass* jc);

#endif // VALIDITY_H
