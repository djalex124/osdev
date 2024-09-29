#pragma once

#include <stdint.h>
#include <stddef.h>

void* memset(void* bufptr, uint32_t value, size_t size);
void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size);
size_t str_len(const char* s);
int str_cmp(const char* a, const char* b, size_t n);
char* str_itoa(uint64_t i, int b);