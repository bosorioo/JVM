#include "readfunctions.h"
#include "utf8.h"
#include "validity.h"
#include <math.h>

/// @cond
static const union {
    uint16_t value;
    uint8_t bytes[2];
} byte_order = {1};
#define IS_LITTLE_ENDIAN byte_order.bytes[1]
/// @endcond

/// @brief Reads a four-byte unsigned integer from the JavaClass file
/// @param JavaClass* jc - poiter to an already open JavaClass file
/// @param [out] uint32_t* out - pointer to variable that will receive
/// the value read, if non null.
/// @return 1 in case of success, 0 in case of failure
uint8_t readu4(JavaClass* jc, uint32_t* out)
{
    uint32_t u4 = 0;

    if (fread(&u4, sizeof(uint32_t), 1, jc->file) != 1)
        return 0;

    jc->totalBytesRead += 4;

    if (!IS_LITTLE_ENDIAN)
        u4 = (u4 >> 24) | ((u4 & 0xFF0000) >> 8) | ((u4 & 0xFF00) << 8) | ((u4 & 0xFF) << 24);

    if (out)
        *out = u4;

    return 1;
}

/// @brief Reads a two-byte unsigned integer from the JavaClass file
/// @param JavaClass* jc - poiter to an already open JavaClass file
/// @param [out] uint16_t* out - pointer to variable that will receive
/// the value read, if non null.
/// @return 1 in case of success, 0 in case of failure
uint8_t readu2(JavaClass* jc, uint16_t* out)
{
    uint16_t u2 = 0;

    if (fread(&u2, sizeof(uint16_t), 1, jc->file) != 1)
        return 0;

    jc->totalBytesRead += 2;

    if (!IS_LITTLE_ENDIAN)
        u2 = (u2 >> 8) | ((u2 & 0xFF) << 8);

    if (out)
        *out = u2;

    return 1;
}

// This function tries to read a field descriptor from the UTF-8 stream.
// It will only read valid field descriptor characters. The return value is
// 0 if the reading fails. It could fail due to the string being empty or the
// string containing invalid field descriptor characters. Also, if the parameter
// "checkValidClassIdentifier" is non-zero, the function will check whether
// classes' field descriptor (L<class name>;) are a valid class identifier.
// If the function succeeds, then the return value is the number of bytes read.
// Number of bytes read could differ from number of characters read, as classes
// could have UTF-8 identifiers that require more than one byte in the representation
// of some character. This function stops once it sucessfully reads one field descriptor.
int32_t readFieldDescriptor(uint8_t* utf8_bytes, int32_t utf8_len, char checkValidClassIdentifier)
{
    int32_t totalBytesRead = 0;
    uint32_t utf8_char;
    uint8_t used_bytes;

    do
    {
        used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

        if (used_bytes == 0)
            return 0;

        utf8_bytes += used_bytes;
        utf8_len -= used_bytes;
        totalBytesRead += used_bytes;

    } while (utf8_char == '[');

    switch (utf8_char)
    {
        case 'B': case 'C': case 'D': case 'F': case 'I': case 'J': case 'S': case 'Z': break;
        case 'L':
        {
            uint8_t* identifierBegin = utf8_bytes;
            int32_t identifierLength = 0;

            do
            {
                used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

                if (used_bytes == 0)
                    return 0;

                utf8_bytes += used_bytes;
                utf8_len -= used_bytes;
                totalBytesRead += used_bytes;
                identifierLength += used_bytes;

            } while (utf8_char != ';');

            if (checkValidClassIdentifier && !isValidJavaIdentifier(identifierBegin, identifierLength - 1, 1))
                return 0;

            break;
        }

        default:
            return 0;
    }

    return totalBytesRead;
}

// This function tries to read a method descriptor from the UTF-8 stream.
// It will only read valid method descriptor characters. The return value is
// 0 if the reading fails. It could fail due to the string being empty or the
// string containing invalid method descriptor characters or incorrect syntax.
// Also, if the parameter "checkValidClassIdentifier" is non-zero, the function
// will check whether classes' field descriptor (L<class name>;) are valid class a
// identifier. If the function succeeds, then the return value is the number of bytes read.
// Number of bytes read could differ from number of characters read, as classes
// could have UTF-8 identifiers that require more than one byte in the representation
// of some character. This function stops once it sucessfully reads one method descriptor.
int32_t readMethodDescriptor(uint8_t* utf8_bytes, int32_t utf8_len, char checkValidClassIdentifier)
{
    int32_t bytesProcessed = 0;
    uint32_t utf8_char;
    uint8_t used_bytes;

    used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

    if (used_bytes == 0 || utf8_char != '(')
        return 0;

    utf8_bytes += used_bytes;
    utf8_len -= used_bytes;
    bytesProcessed += used_bytes;

    int32_t field_descriptor_length;

    do
    {
        field_descriptor_length = readFieldDescriptor(utf8_bytes, utf8_len, checkValidClassIdentifier);

        if (field_descriptor_length == 0)
        {
            used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

            if (used_bytes == 0 || utf8_char != ')')
                return 0;

            utf8_bytes += used_bytes;
            utf8_len -= used_bytes;
            bytesProcessed += used_bytes;

            break;
        }

        utf8_bytes += field_descriptor_length;
        utf8_len -= field_descriptor_length;
        bytesProcessed += field_descriptor_length;

    } while (1);

    field_descriptor_length = readFieldDescriptor(utf8_bytes, utf8_len, 1);

    if (field_descriptor_length == 0)
    {
        used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

        if (used_bytes == 0 || utf8_char != 'V')
            return 0;

        utf8_len -= used_bytes;
        bytesProcessed += used_bytes;
    }
    else
    {
        utf8_len -= field_descriptor_length;
        bytesProcessed += field_descriptor_length;
    }

    return utf8_len == 0 ? bytesProcessed : 0;
}

// Converts a 32 bit value to its floating point (single precision)
// representation.
float readFloatFromUint32(uint32_t value)
{
    if (value == 0x7F800000UL)
        return INFINITY;

    if (value == 0xFF800000UL)
        return -INFINITY;

    if ((value >= 0x7F800000UL && value <= 0x7FFFFFFFUL) ||
        (value >= 0xFF800000UL && value <= 0xFFFFFFFFUL))
    {
        return NAN;
    }

    int32_t s = (value >> 31) == 0 ? 1 : -1;
    int32_t e = (value >> 23) & 0xFF;
    int32_t m = (e == 0) ?
        (value & 0x7FFFFFUL) << 1 :
        (value & 0x7FFFFFUL) | 0x800000;

    return s * (float)ldexp((double)m, e - 150);
}

double readDoubleFromUint64(uint64_t value)
{
    if (value == 0x7FF0000000000000ULL)
        return INFINITY;

    if (value == 0xFFF0000000000000ULL)
        return -INFINITY;

    if ((value >= 0x7FF0000000000001ULL && value <= 0x7FFFFFFFFFFFFFFFULL) ||
        (value >= 0xFFF0000000000001ULL && value <= 0xFFFFFFFFFFFFFFFFULL))
    {
        return NAN;
    }

    int32_t s = (value >> 63) == 0 ? 1 : -1;
    int32_t e = (value >> 52) & 0x7FF;
    int64_t m = (e == 0) ?
        (value & 0xFFFFFFFFFFFFFULL) << 1 :
        (value & 0xFFFFFFFFFFFFFULL) | 0x10000000000000ULL;

    return s * ldexp((double)m, e - 1075);
}
