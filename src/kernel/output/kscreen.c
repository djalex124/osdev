#include <stddef.h>
#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <mm/mem.h>

graphics_info kgraphics;
uint32_t *kscreen_buffer;
uint32_t *kscreen_termbuffer;

extern uint8_t kmouse_draw;
extern int16_t mx, my;

extern uint8_t kterm_draw;

static inline void kscreen_directputp(int x, int y, uint32_t color)
{
    if (x >= kgraphics.horizontal_res || x < 0)
        return;
    else if (y >= kgraphics.vertical_res || y < 0)
        return;
    
    unsigned where = x*4 + y*kgraphics.ppsl*4;
    ((unsigned char*)kscreen_buffer)[where] = color & 0xFF;
    ((unsigned char*)kscreen_buffer)[where + 1] = (color >> 8) & 0xFF;
    ((unsigned char*)kscreen_buffer)[where + 2] = (color >> 16) & 0xFF;
}

void kscreen_directdrawrect(int x, int y, int w, int h, uint32_t color)
{
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            kscreen_directputp(x + j, y + i, color);
}

void kscreen_copy()
{
    if (kterm_draw)
        memcpy_ssealign(kscreen_buffer, kscreen_termbuffer,
            kgraphics.horizontal_res * kgraphics.vertical_res * 4);
    if (kmouse_draw)
        kscreen_directdrawrect(mx, kgraphics.vertical_res - my, 5, 5, 0xFF00FF);
    memcpy_ssealign(kgraphics.framebuffer_base, kscreen_buffer,
        kgraphics.horizontal_res * kgraphics.vertical_res * 4);
}

void kscreen_clr(uint32_t color)
{
    kscreen_directdrawrect(0, 0, kgraphics.horizontal_res, kgraphics.vertical_res, color);
}

uint32_t *kscreen_setterm()
{
    return kscreen_termbuffer;
}

void kscreen_init()
{
    memcpy(&kgraphics, &k_boottable.graphics, sizeof(kgraphics));
    //assume 32 bpp as is standard from UEFI's GOP
    
    kgraphics.framebuffer_base = kmem_page((uint64_t)kgraphics.framebuffer_base, kgraphics.horizontal_res * kgraphics.vertical_res * 4, 0b10011);
    kscreen_buffer = kmem_alloc((kgraphics.horizontal_res * kgraphics.vertical_res * 4)/0x1000);
    kscreen_termbuffer = kmem_alloc((kgraphics.horizontal_res * kgraphics.vertical_res * 4)/0x1000);

#ifdef AQUA_DEBUG
    kdebug_outf("\r\nkscr: buffer [0x%x]", (uintptr_t)kscreen_buffer);
    kdebug_outf("\r\nkscr: [%d]x[%d] @ 32 bpp", kgraphics.horizontal_res, kgraphics.vertical_res);
    kdebug_outf("\r\nkscr: framebuffer [0x%x]", kgraphics.framebuffer_base);
#endif
}