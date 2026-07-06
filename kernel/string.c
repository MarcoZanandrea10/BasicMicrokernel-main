#include <stdint.h>

void *memcpy(void *dest, const void *src, uint64_t n)
{
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;

    while (n--)
        *d++ = *s++;

    return dest;
}

void *memset(void *s, int c, uint64_t n)
{
    uint8_t *p = s;
    while (n--)
        *p++ = c;
    return s;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dest, const char *src)
{
    char *out = dest;

    while ((*dest++ = *src++))
        ;

    return out;
}

uint64_t strlen(const char *s)
{
    uint64_t len = 0;

    while (s[len])
        len++;

    return len;
}
