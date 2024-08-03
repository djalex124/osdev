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

//the plan is to use cpuid to check for quickest possible mem functions