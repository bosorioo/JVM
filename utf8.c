#include "utf8.h"

#define SINGLE_BYTE_MASK  0x80
#define SINGLE_BYTE_VALUE 0
#define DOUBLE_BYTE_MASK  0xE0
#define DOUBLE_BYTE_VALUE 0xC0
#define TRIPLE_BYTE_MASK  0xF0
#define TRIPLE_BYTE_VALUE 0xE0
#define FOLLOW_BYTE_MASK  0xC0
#define FOLLOW_BYTE_VALUE 0x80

uint8_t nextUTF8Character(uint8_t* utf8_bytes, int32_t utf8_len, uint32_t* outCharacter)
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

char cmp_UTF8_Ascii(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t* ascii_bytes, int32_t ascii_len)
{
    uint32_t utf8_char;
    uint8_t bytes_used;

    while (utf8_len > 0 && ascii_len > 0)
    {
        bytes_used = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

        if (bytes_used == 0 || utf8_char > 255 || (uint8_t)utf8_char != *ascii_bytes)
            return 0;

        utf8_bytes += bytes_used;
        utf8_len -= bytes_used;
        ascii_bytes++;
        ascii_len--;
    }

    return ascii_len == utf8_len;
}

uint32_t UTF8_to_Ascii(uint8_t* out_buffer, int32_t buffer_len, uint8_t* utf8_bytes, int32_t utf8_len)
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

uint32_t UTF8StringLength(uint8_t* utf8_bytes, int32_t utf8_len)
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
