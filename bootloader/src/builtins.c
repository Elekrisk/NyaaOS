#include "builtins.h"


void* memset(void* ptr, int value, size_t num)
{
    void* ret = ptr;
    for (int i = 0; i < num; ++i, ++ptr)
    {
        *(uint8_t*)ptr = (uint8_t)value;
    }
    return ret;
}

void* memmove(void* dest_, const void* src_, size_t num)
{
    uint8_t* dest = dest_;
    const uint8_t* src = src_;
    if (dest < src) {
        for (size_t i = 0; i < num; ++i) 
        {
            dest[i] = src[i];
        }
    } else {
        for (size_t i = num; i > 0; --i)
        {
            dest[i] = src[i];
        }
        dest[0] = src[0];
    }

    return dest_;
}

size_t strlen(const uint16_t* str)
{
    const uint16_t* p = str;
    for (; *p != 0; ++p) {}
    return p - str;
}

uint16_t* strcpy(uint16_t* dst, const uint16_t* src)
{
    const uint16_t *s = src;
    uint16_t *d = dst;
    for (; *s != 0; s++, d++) {
        *d = *s;
    }
    *d = *s;
    return dst;
}

bool memcmp(void* a, void*b, size_t num) {
    for (size_t i = 0; i < num; ++i, ++a, ++b) {
        if (*(uint8_t*)a != *(uint8_t*)b) {
            return false;
        }
    }
    return true;
}
