#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <stdint.h>
#include <stdio.h>
#include "javaclassfile.h"

uint8_t readu4(struct JavaClassFile* jcf, uint32_t* out);
uint8_t readu2(struct JavaClassFile* jcf, uint16_t* out);

#endif // FILEPARSER_H
