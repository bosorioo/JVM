#ifndef VALIDITY_H
#define VALIDITY_H

#include "javaclassfile.h"

char checkMethodAccessFlags(JavaClassFile* jcf, uint16_t acessFlags);
char checkClassIndexAndAccessFlags(JavaClassFile* jcf);
char checkClassNameFileNameMatch(JavaClassFile* jcf, const char* classFilePath);
char isValidJavaIdentifier(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t isClassIdentifier);
char isValidNameIndex(JavaClassFile* jcf, uint16_t name_index, uint8_t isClassIdentifier);
char isValidMethodNameIndex(JavaClassFile* jcf, uint16_t name_index);
char checkConstantPoolValidity(JavaClassFile* jcf);

#endif // VALIDITY_H
