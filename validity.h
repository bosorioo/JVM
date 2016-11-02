#ifndef VALIDITY_H
#define VALIDITY_H

#include "javaclassfile.h"

char checkClassName(JavaClassFile* jcf, const char* classFilePath);
char isValidNameIndex(JavaClassFile* jcf, uint16_t name_index, uint8_t acceptSlashes);
char checkConstantPoolValidity(JavaClassFile* jcf);

#endif // VALIDITY_H
