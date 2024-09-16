#include <screen.h>
#include <serial.h>
#include <mem.h>
#include <debug.h>

static struct multiboot_framebuffer_tag screen_info;

void kscreen_clr(uint32_t color)
{
    for (int y = 0; y < screen_info.height; y++)
    {
        for (int x = 0; x < screen_info.width; x++)
        {
            *((uint32_t*)(screen_info.addr + y*screen_info.pitch + x*(screen_info.bpp/8)))=color;
        }
    }
}

void kscreen_set(struct multiboot_framebuffer_tag* fb_tag)
{
    screen_info.addr = fb_tag->addr;
    screen_info.bpp = fb_tag->bpp;
    screen_info.width = fb_tag->width;
    screen_info.height = fb_tag->height;
    screen_info.pitch = fb_tag->pitch;
}

extern char _font_start[];

void kscreen_putc(char c, int cx, int cy, uint64_t fg, uint64_t bg)
{
    
}

void kscreen_init()
{
    kserial_outf("\r\nkscr: [%d]x[%d] @ %d bpp", screen_info.width, screen_info.height, screen_info.bpp);
    kmem_page(screen_info.addr, screen_info.addr, screen_info.width * screen_info.height * screen_info.bpp, 0b11);
    kdebug_outf("\r\nkscr: p [%d] framebuffer [0x%x]", screen_info.pitch, screen_info.addr);
}