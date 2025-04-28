#pragma once

#include <stdint.h>
#include <kernel.h>

void kscreen_init();
void kscreen_copy();

void kscreen_drawrect(int x, int y, int w, int h, uint32_t color);

void kscreen_putf(const char *fmt, ...);
void kscreen_clr(uint32_t color);

extern graphics_info kgraphics;

typedef struct
{
    uint8_t x;
    uint8_t y;
}kscreen_pos;

kscreen_pos kscreen_getpos();
void kscreen_setpos(kscreen_pos pos);

#define default_color 0x34568B

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t num_glyph;
    uint32_t bp_glyph;
    uint32_t height;
    uint32_t width;
} psf_font;