#include "rtutil.h"

uint32_t _crc32(uint32_t seed, uint8_t* data, int size)
{
    uint32_t r = ~seed;
    uint8_t* end = data + size;

    while (data < end)
    {
        r ^= *data++;

        for (int i = 0; i < 8; i++)
        {
            uint32_t t = ~((r & 1u) - 1);
            r = (r >> 1u) ^ (0xEDB88320 & t);
        }
    }

    return ~r;
}
