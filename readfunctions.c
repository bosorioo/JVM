#include "readfunctions.h"

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
