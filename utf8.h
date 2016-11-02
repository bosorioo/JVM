#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

uint8_t nextUTF8Character(uint8_t* utf8_bytes, int32_t utf8_len, uint32_t* outCharacter);
char cmp_UTF8_Ascii(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t* ascii_bytes, int32_t ascii_len);
void UTF8_to_Ascii(uint8_t* out_buffer, int32_t buffer_len, uint8_t* utf8_bytes, int32_t utf8_len);

#endif // UTF8_H
