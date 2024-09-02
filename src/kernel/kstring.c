#include <stddef.h>
#include <stdint.h>

void* memset(void* bufptr, uint64_t value, size_t size) {
	uint64_t* buf = (uint64_t*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (uint64_t) value;
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

//the plan is to use cpuid to check for quickest possible mem functions