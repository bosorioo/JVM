#include "readfunctions.h"
#include "utf8.h"

uint8_t readu4(JavaClassFile* jcf, uint32_t* out)
{
    int byte;
    uint32_t u4 = 0;
    uint8_t i;

    for (i = 0; i < 4; i++)
    {
        byte = fgetc(jcf->file);

        if (byte == EOF)
            break;

        u4 <<= 8;
        u4 |= byte;
        jcf->totalBytesRead++;
    }

    if (out)
        *out = u4;

    return i == 4;
}

uint8_t readu2(JavaClassFile* jcf, uint16_t* out)
{
    int byte;
    uint16_t u2 = 0;
    uint8_t i;

    for (i = 0; i < 2; i++)
    {
        byte = fgetc(jcf->file);

        if (byte == EOF)
            break;

        u2 <<= 8;
        u2 |= byte;
        jcf->totalBytesRead++;
    }

    if (out)
        *out = u2;

    return i == 2;
}

int32_t readFieldDescriptor(uint8_t* utf8_bytes, int32_t utf8_len)
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
            do
            {
                used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

                if (used_bytes == 0)
                    return 0;

                utf8_bytes += used_bytes;
                utf8_len -= used_bytes;
                totalBytesRead += used_bytes;
            } while (utf8_char != ';');

            break;
        }

        default:
            return 0;
    }

    return totalBytesRead;
}
