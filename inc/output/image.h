#pragma once

#include <stdint.h>

void kimage_termblit(uint32_t *image_ptr, int x, int y);
uint32_t *kimage_getbuftga(unsigned char *ptr, int size, size_t *pages);