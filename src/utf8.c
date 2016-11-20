#include "utf8.h"

#define SINGLE_BYTE_MASK  0x80
#define SINGLE_BYTE_VALUE 0
#define DOUBLE_BYTE_MASK  0xE0
#define DOUBLE_BYTE_VALUE 0xC0
#define TRIPLE_BYTE_MASK  0xF0
#define TRIPLE_BYTE_VALUE 0xE0
#define FOLLOW_BYTE_MASK  0xC0
#define FOLLOW_BYTE_VALUE 0x80

// Function to iterate over the bytes of a UTF-8.
// It reads one character and write its value to the pointer
// "*outCharacter" if it isn't NULL. The function returns the
// number of bytes read from the UTF-8 stream to represent
// that single character.
// If the return value is 0, then nothing was read. It could
// mean that the length is not sufficient, that the UTF-8
// encoding is wrong or the stream has a four-byte character,
// which isn't supported by this program.
uint8_t nextUTF8Character(const uint8_t* utf8_bytes, int32_t utf8_len, uint32_t* outCharacter)
{
    if (utf8_len <= 0)
        return 0;

    uint32_t utf8_char;
    uint8_t used_bytes;

    if ((*utf8_bytes & SINGLE_BYTE_MASK) == SINGLE_BYTE_VALUE)
    {
        utf8_char = *utf8_bytes;
        used_bytes = 1;
    }
    else if (utf8_len >= 2 &&
             ((*utf8_bytes & DOUBLE_BYTE_MASK) == DOUBLE_BYTE_VALUE) &&
             ((*(utf8_bytes + 1) & FOLLOW_BYTE_MASK) == FOLLOW_BYTE_VALUE))
    {
        utf8_char = ((*utf8_bytes & 0x1F) << 6) + (*(utf8_bytes + 1) & 0x3F);
        used_bytes = 2;
    }
    else if (utf8_len >= 3 &&
             ((*utf8_bytes & TRIPLE_BYTE_MASK) == TRIPLE_BYTE_VALUE) &&
             ((*(utf8_bytes + 1) & FOLLOW_BYTE_MASK) == FOLLOW_BYTE_VALUE) &&
             ((*(utf8_bytes + 2) & FOLLOW_BYTE_MASK) == FOLLOW_BYTE_VALUE))
    {
        utf8_char = ((*utf8_bytes & 0xF) << 12) + ((*(utf8_bytes + 1) & 0x3F) << 6) + (*(utf8_bytes + 2) & 0x3F);
        used_bytes = 3;
    }
    else
    {
        // Either an invalid UTF-8 character, or a four-byte character (which is intentionally not implemented)
        return 0;
    }

    if (outCharacter)
        *outCharacter = utf8_char;

    return used_bytes;
}

// Function to compare two strings, one in UTF-8 and other in ASCII.
// Return value is 1 in case the strings are equal (case sensitive),
// 0 otherwise.
// "*ascii_bytes" is the string and ascii_len is the number of characters
// in that string. Note that it doesn't matter if the string is null terminated
// or not.
char cmp_UTF8_Ascii(const uint8_t* utf8_bytes, int32_t utf8_len, const uint8_t* ascii_bytes, int32_t ascii_len)
{
    uint32_t utf8_char;
    uint8_t bytes_used;

    while (utf8_len > 0 && ascii_len > 0)
    {
        bytes_used = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

        if (bytes_used == 0 || utf8_char > 127 || (uint8_t)utf8_char != *ascii_bytes)
            return 0;

        utf8_bytes += bytes_used;
        utf8_len -= bytes_used;
        ascii_bytes++;
        ascii_len--;
    }

    return ascii_len == utf8_len;
}

// Function that translates a UTF-8 stream to ASCII, storing the result
// in the output buffer pointed by "*out_buffer". At most "buffer_len"
// characters will be written to the buffer, NULL character included.
uint32_t UTF8_to_Ascii(uint8_t* out_buffer, int32_t buffer_len, const uint8_t* utf8_bytes, int32_t utf8_len)
{
    uint32_t charactersWritten = 0;
    uint32_t utf8_char;
    uint8_t bytes_used;

    while (buffer_len > 1 && utf8_len > 0)
    {
        bytes_used = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

        if (bytes_used == 0)
            break;

        *out_buffer++ = utf8_char > 127 ? '?' : (uint8_t)utf8_char;
        buffer_len--;
        utf8_bytes += bytes_used;
        utf8_len -= bytes_used;
        charactersWritten++;
    }

    if (buffer_len > 0)
        *out_buffer = '\0';

    return charactersWritten;
}

// Returns the number of characters a UTF-8 string has.
uint32_t UTF8StringLength(const uint8_t* utf8_bytes, int32_t utf8_len)
{
    uint32_t length = 0;
    uint8_t bytes_used;

    while (utf8_len > 0)
    {
        bytes_used = nextUTF8Character(utf8_bytes, utf8_len, 0);

        if (bytes_used == 0)
            break;

        length++;
        utf8_len -= bytes_used;
        utf8_bytes += bytes_used;
    }

    return length;
}
