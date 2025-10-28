#pragma once

#include <stdint.h>
#include <kernel/kernel.h>

void kscreen_init();
void kscreen_copy();
uint32_t *kscreen_setterm();

void kscreen_drawrect(int x, int y, int w, int h, uint32_t color);

void kscreen_clr(uint32_t color);