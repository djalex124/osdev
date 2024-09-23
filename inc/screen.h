#pragma once

#include <stdint.h>
#include <multiboot.h>

void kscreen_init();
void kscreen_set(struct multiboot_framebuffer_tag* fb_tag);

void kscreen_putf(const char *fmt, ...);
void kscreen_clr(uint32_t color);

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