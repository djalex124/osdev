#pragma once

#include <stdint.h>

void kserial_init();

void kserial_outc(char c);
void kserial_outs(char *s);
void kserial_outn(uint64_t n, uint8_t b);