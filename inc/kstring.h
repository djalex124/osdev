#pragma once

#include <stdint.h>
#include <stddef.h>

void* memset(void* bufptr, uint64_t value, size_t size);
void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size);
size_t str_len(const char* s);