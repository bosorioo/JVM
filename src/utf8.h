#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

/// @brief Utility macro to reduce code and improve readability.
///
/// Usage example:
/// @code printf("%.*s", PRINT_UTF8(my_utf8_var)); @endcode
/// Instead of:
/// @code printf("%.*s", my_utf8_var->Utf8.length, my_utf8_var->Utf8.bytes); @endcode
#define PRINT_UTF8(x) x->Utf8.length, x->Utf8.bytes

/// @brief Utility macro to reduce code and improve readability.
///
/// Usage example:
/// @code cmp_UTF8(UTF8(varA), UTF8(varB)); @endcode
/// Instead of:
/// @code cmp_UTF8(varA->Utf8.bytes, varA->Utf8.length, varB->Utf8.bytes, varB->Utf8.length); @endcode
#define UTF8(x) x->Utf8.bytes, x->Utf8.length

uint8_t nextUTF8Character(const uint8_t* utf8_bytes, int32_t utf8_len, uint32_t* outCharacter);
char cmp_UTF8_Ascii(const uint8_t* utf8_bytes, int32_t utf8_len, const uint8_t* ascii_bytes, int32_t ascii_len);
char cmp_UTF8(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len);
char cmp_UTF8_FilePath(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len);
uint32_t UTF8_to_Ascii(uint8_t* out_buffer, int32_t buffer_len, const uint8_t* utf8_bytes, int32_t utf8_len);
uint32_t UTF8StringLength(const uint8_t* utf8_bytes, int32_t utf8_len);

#endif // UTF8_H

/// @defgroup utf8 UTF-8 module
///
/// @brief Declares functions and macros to handle UTF-8 streams.
///
/// All UTF-8 streams are made of a pointer to a sequence of bytes and
/// the length of that sequence.
///
/// Two utility macros are used to help write less code.
///
/// This module provides functions to compare UTF-8 streams with other UTF-8 streams
/// and ASCII strings.
///
/// There is also a function to iterate over the characters of a UTF-8 stream.
///
/// @see utf8.c
