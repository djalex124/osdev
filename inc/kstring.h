#pragma once

#include <stdint.h>
#include <stddef.h>

void* memset(void* bufptr, unsigned char value, size_t size);
void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size);
size_t str_len(const char* s);
int str_cmp(const char* a, const char* b);
char* str_tok(char *s, const char* split);
char* str_itoa(long i, int b);
char* str_utoa(uint64_t i, int b);
int64_t str_atoi(const char *s);