#include <stddef.h>
#include <stdint.h>

void* memset(void* bufptr, unsigned char value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = value;
	return bufptr;
}

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

void* memcpy_ssealign(void* restrict dstptr, const void* restrict srcptr, size_t size)
{
    void* to = (void*)dstptr;
    void* from = (void*)srcptr;
    size_t count = (size/64);
    for (size_t i = 0; i < count; i++)
    {
        __asm__ __volatile__ (
            "movups (%0), %%xmm0\n"
            "movups 16(%0), %%xmm1\n"
            "movups 32(%0), %%xmm2\n"
            "movups 48(%0), %%xmm3\n"
            "movntdq %%xmm0, (%1)\n"
            "movntdq %%xmm1, 16(%1)\n"
            "movntdq %%xmm2, 32(%1)\n"
            "movntdq %%xmm3, 48(%1)\n"
            :: "r"(from), "r"(to) : "memory");

        from += 64;
        to += 64;
    }
    count = size % 64;
    if (count)
        return memcpy(from, to, count);
    return dstptr;
}

size_t str_len(const char* s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

int str_cmp(const char* a, const char* b)
{
	for (; *a == *b; a++, b++)
    {
        if (*a == '\0')
            return 0;
    }
    return *a - *b;
}

unsigned int is_split(char c, char *split)
{
    while (*split != '\0')
    {
        if (c == *split)
            return 1;
        split++;
    }
    return 0;
}

char* str_tok(char *s, char *split)
{
    static char* backup;
    if (!s)
        s = backup;
    if (!s)
        return NULL;
    while (1)
    {
        if (is_split(*s, split))
        {
            s++;
            continue;
        }
        else if (*s == '\0')
            return NULL;
        break;
    }
    char *ret = s;
    while (1)
    {
        if (*s == '\0')
        {
            backup = s;
            return ret;
        }
        else if (is_split(*s, split))
        {
            *s = '\0';
            backup = s + 1;
            return ret;
        }
        s++;
    }
}

static char str_toa_buffer[64];

char* str_itoa(long i, int b)
{
    if (b < 1)
		return 0;
	
	char* p = str_toa_buffer;
    if (i < 0 && b == 10)
    {
        *p++ = '-';
        i *= -1;
    }

    char* low = p;
    do
    {
        *p++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + i % b];
        i /= b;
    }while (i);

    *p-- = '\0';
    while (low < p)
    {
        char tmp = *low;
        *low++ = *p;
        *p-- = tmp;
    }

	return str_toa_buffer;
}

char* str_utoa(uint64_t i, int b)
{
    if (b < 1)
		return 0;
	
	char* p = str_toa_buffer;
    char* low = p;
    do
    {
        *p++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + i % b];
        i /= b;
    }while (i);

    *p-- = '\0';
    while (low < p)
    {
        char tmp = *low;
        *low++ = *p;
        *p-- = tmp;
    }

	return str_toa_buffer;
}

int64_t str_atoi(const char *s)
{
    int sign = 1;
    int64_t res = 0, index = 0;

    while (s[index] == ' ')
        index++;
    
    if (s[index] == '-' || s[index] == '+')
        sign = 1 - 2 * (s[index++] == '-');

    while (s[index] >= '0' && s[index] <= '9')
    {
        if (res > INT64_MAX / 10 || (res == INT64_MAX / 10 && s[index] - '0' > 7))
        {
            if (sign == 1)
                return INT64_MAX;
            else
                return INT64_MIN;
        }
        res = 10 * res + (s[index++] - '0');
    }
    return res * sign;
}