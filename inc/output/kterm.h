#pragma once

#include <stdint.h>

#include <output/screen.h>

#include <ps2/kbd.h>

void kterm_loop();
void kterm_init();

void kterm_nhputf(const char *fmt, ...);
void kterm_putf(const char *fmt, ...);
void kterm_clr(uint32_t color);

void kterm_input(kkeyboard_state *k);

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

void kterm_setpartition(int fs);
int kterm_getpartition();

#define kterm_buffersize 100
#define kterm_maxargs 16

#define kterm_titletext1 "[AQUA Kernel]"
#define kterm_titletext2 "Build: (" AQUA_VER_STRING ")"

#define kterm_infotext "[AQUA Kernel (" AQUA_VER_STRING ")]\n[Built " __TIME__" "__DATE__ " Central Time]\n[Quote: Never back down, never give up!]"

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