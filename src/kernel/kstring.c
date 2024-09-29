#include <stddef.h>
#include <stdint.h>

void* memset(void* bufptr, uint32_t value, size_t size) {
	uint32_t* buf = (uint32_t*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (uint32_t) value;
	return bufptr;
}

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

size_t str_len(const char* s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

int str_cmp(const char* a, const char* b, size_t n)
{
	while ((*a == *b) && *a && n)
		++a, ++b, --n;
	if (n == 0)
		return 0;
	return ((int) (uint8_t) *a) - ((int) (uint8_t) *b);
}

static char str_itoa_buffer[32];

char* str_itoa(uint64_t i, int b)
{
    if (b < 1)
		return 0;
	
	char* p = str_itoa_buffer;
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

	return str_itoa_buffer;
}

//the plan is to use cpuid to check for quickest possible mem functions