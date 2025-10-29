#pragma once

#include <stdint.h>

#include <output/screen.h>

void kterm_loop();
void kterm_init();

void kterm_nhputf(const char *fmt, ...);
void kterm_putf(const char *fmt, ...);
void kterm_clr(uint32_t color);

typedef struct
{
    uint8_t x;
    uint8_t y;
}kscreen_pos;

kscreen_pos kterm_getpos();
void kterm_setpos(kscreen_pos pos);

uint32_t kterm_getfg();
uint32_t kterm_getbg();
void kterm_setcolor(uint32_t foreground, uint32_t background);

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