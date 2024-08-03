#include <screen.h>
#include <serial.h>
#include <mem.h>

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

void kscreen_init()
{
    kserial_outf("\r\nkscr: width %d height %d bpp %d pitch %d", screen_info.width, screen_info.height, screen_info.bpp, screen_info.pitch);
    kmem_page(screen_info.addr, screen_info.addr, screen_info.width * screen_info.height * screen_info.bpp, 0b11);
    kserial_outf("\r\nkscr: framebuffer at 0x%x", screen_info.addr);
}