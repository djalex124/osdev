#pragma once

#include <stdint.h>
#include <multiboot.h>

void kscreen_init();
void kscreen_set(struct multiboot_framebuffer_tag* fb_tag);
void kscreen_clr(uint32_t color);

#define default_color 0x34568B