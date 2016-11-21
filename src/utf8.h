#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

uint8_t nextUTF8Character(const uint8_t* utf8_bytes, int32_t utf8_len, uint32_t* outCharacter);
char cmp_UTF8_Ascii(const uint8_t* utf8_bytes, int32_t utf8_len, const uint8_t* ascii_bytes, int32_t ascii_len);
char cmp_UTF8(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len);
char cmp_UTF8_FilePath(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len);
uint32_t UTF8_to_Ascii(uint8_t* out_buffer, int32_t buffer_len, const uint8_t* utf8_bytes, int32_t utf8_len);
uint32_t UTF8StringLength(const uint8_t* utf8_bytes, int32_t utf8_len);

#endif // UTF8_H
