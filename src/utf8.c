#include "utf8.h"

#define SINGLE_BYTE_MASK  0x80
#define SINGLE_BYTE_VALUE 0
#define DOUBLE_BYTE_MASK  0xE0
#define DOUBLE_BYTE_VALUE 0xC0
#define TRIPLE_BYTE_MASK  0xF0
#define TRIPLE_BYTE_VALUE 0xE0
#define FOLLOW_BYTE_MASK  0xC0
#define FOLLOW_BYTE_VALUE 0x80

/// @brief Function to iterate over the bytes of a UTF-8.
///
/// @param const uint8_t* utf8_bytes - represents the character being read
/// @param int32_t utf8_len - is the number of characteres
/// @param uint32_t* outCharacter - pointer where the character being read
/// is written, if it isn't NULL
///
/// @return uint8_t - the number of bytes read from the UTF-8 stream to 
/// represent that single character. If the return value is 0, then nothing 
/// was read. It could mean that the length is not sufficient, that the UTF-8
/// encoding is wrong or the stream has a four-byte character, which isn't 
/// supported by this program.
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

/// @brief Function to compare two strings, one in UTF-8 and other in ASCII.
///
/// @param const uint8_t* utf8_bytes - is the string in utf8_bytes
/// @param int32_t utf8_len - is the number of characters
/// @param const uint8_t* ascii_bytes - is the string in ASCII
/// @param int32_t ascii_len - is the number of characters
///
/// @return char - value is 1 in case the strings are equal (case sensitive),
/// 0 otherwise.
/// 
/// @note doesn't matter if the string is null terminated
/// or not.
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

/// @brief Function to compare two strings, both in UTF-8.
/// 
/// @param const uint8_t* utf8A_bytes - pointer to the bytes that make the 
/// UTF-8 A string to be compared
/// @param const uint8_t* utf8B_bytes - pointer to the bytes that make the 
/// UTF-8 B string to be compared
/// @param int32_t utf8A_len - length of the bytes that make the A string 
/// @param int32_t utf8B_len - length of the bytes that make the B string 
/// 
/// @return return value is 1 in case the strings are equal (case sensitive),
/// 0 otherwise.
char cmp_UTF8(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len)
{
    if (utf8A_len != utf8B_len)
        return 0;

    int32_t i;

    for (i = 0; i < utf8A_len; i++)
    {
        if (utf8A_bytes[i] != utf8B_bytes[i])
            return 0;
    }

    return 1;
}

/// @brief Function to compare two strings that contains file paths, both in UTF-8.
/// 
/// The difference in this function is that it considers slashes (/)
/// and backslashes (\) the same characters, and consecutive slashes or
/// backslashes are treated like one character, for sake of checking if
/// two strings are actually the path to the same file/directory.
/// utf8A_bytes and utf8B_bytes are the pointers to the bytes that make the
/// UTF-8 strings A and B that will be compared.
/// utf8A_len and utf8B_len are the length of the bytes that make those strings,
/// respectively.
///
/// @param const uint8_t* utf8A_bytes - pointer to the bytes that make the 
/// UTF-8 A string to be compared
/// @param const uint8_t* utf8B_bytes pointer to the bytes that make the 
/// UTF-8 B string to be compared
/// @param int32_t utf8A_len - length of the bytes that make the A string 
/// @param int32_t utf8B_len - length of the bytes that make the B string 
///
/// @return return value is 1 in case the strings are equal (case sensitive),
/// 0 otherwise.
char cmp_UTF8_FilePath(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len)
{
    uint32_t utf8_charA, utf8_charB;
    uint8_t bytes_used;

    while (utf8A_len > 0 && utf8B_len > 0)
    {

        bytes_used = nextUTF8Character(utf8A_bytes, utf8A_len, &utf8_charA);

        if (bytes_used == 0)
            return 0; // Invalid UTF-8 always return false in comparison

        utf8A_bytes += bytes_used;
        utf8A_len -= bytes_used;

        bytes_used = nextUTF8Character(utf8B_bytes, utf8B_len, &utf8_charB);

        if (bytes_used == 0)
            return 0; // Invalid UTF-8 always return false in comparison

        utf8B_bytes += bytes_used;
        utf8B_len -= bytes_used;

        if ( (utf8_charA == utf8_charB) ||
             (utf8_charA == '/' && utf8_charB == '\\') ||
             (utf8_charA == '\\' && utf8_charB == '/'))
        {
            // Advance consecutive slashes and backslashes of string A
            if (utf8_charA == '/' || utf8_charA == '\\')
            {
                while (utf8A_len > 0)
                {
                    bytes_used = nextUTF8Character(utf8A_bytes, utf8A_len, &utf8_charA);

                    if (bytes_used == 0)
                        return 0; // Invalid UTF-8 always return false in comparison

                    if (utf8_charA != '/' && utf8_charA != '\\')
                        break;

                    utf8A_bytes += bytes_used;
                    utf8A_len -= bytes_used;
                }
            }

            // Advance consecutive slashes and backslashes of string B
            if (utf8_charB == '/' || utf8_charB == '\\')
            {
                while (utf8B_len > 0)
                {
                    bytes_used = nextUTF8Character(utf8B_bytes, utf8B_len, &utf8_charB);

                    if (bytes_used == 0)
                        return 0; // Invalid UTF-8 always return false in comparison

                    if (utf8_charB != '/' && utf8_charB != '\\')
                        break;

                    utf8B_bytes += bytes_used;
                    utf8B_len -= bytes_used;
                }
            }

            // Go to next iteration since both characters extracted are the same
            continue;
        }

        // If reached here, string A had a character not equal to string B
        return 0;
    }

    // If reached here, one of the strings has reached the end.
    // So check if their sizes match. If they don't, then one
    // string is longer than the other
    return utf8A_len == utf8B_len;
}

/// @brief Function that translates a UTF-8 stream to ASCII
///
/// @param const uint8_t* utf8_bytes - UTF-8 strem to be translate
/// @param int32_t utf8_len - length of the bytes that make the UTF-8 strem 
/// @param uint8_t* out_buffer - pointer where the result will be stored
/// @param int32_t buffer_len - length of the bytes that make out_buffer 
/// 
/// @note "buffer_len" characters will be written to the buffer, NULL character included.
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

/// @brief Returns the number of characters a UTF-8 string has.
///
/// @param const uint8_t* utf8_bytes - UTF-8 strem
/// @param length of the bytes that make utf8_bytes
///
/// @return number of characters a UTF-8 string has
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
