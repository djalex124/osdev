#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <mm/mem.h>

graphics_info kgraphics;
uint32_t *kscreen_buffer;

void kscreen_copy()
{
    memcpy_ssealign(kgraphics.framebuffer_base, kscreen_buffer,
        kgraphics.horizontal_res * kgraphics.vertical_res * 4);
}

static inline void kscreen_putp(int x, int y, uint32_t color)
{
    if (x > kgraphics.horizontal_res || x < 0)
        return;
    else if (y > kgraphics.vertical_res || y < 0)
        return;
    
    unsigned where = x*4 + y*kgraphics.ppsl*4;
    ((unsigned char*)kscreen_buffer)[where] = color & 0xFF;
    ((unsigned char*)kscreen_buffer)[where + 1] = (color >> 8) & 0xFF;
    ((unsigned char*)kscreen_buffer)[where + 2] = (color >> 16) & 0xFF;
}

void kscreen_drawrect(int x, int y, int w, int h, uint32_t color)
{
    unsigned char* where = (unsigned char*)kscreen_buffer + y*kgraphics.ppsl*4;
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            where[j*4] = color & 0xFF;
            where[j*4 + 1] = (color >> 8) & 0xFF;
            where[j*4 + 2] = (color >> 16) & 0xFF;
        }
        where += kgraphics.ppsl*4;
    }
}

void kscreen_clr(uint32_t color)
{
    kscreen_drawrect(0, 0, kgraphics.horizontal_res, kgraphics.vertical_res, color);
}

extern char _binary____font_psf_start[];
uint32_t fg, bg;
unsigned int cx = 0, cy = 0, cw = 0, ch = 0;
psf_font *font;

void kscreen_putc(uint16_t c)
{
    font = (psf_font *)&_binary____font_psf_start;
    int sx = cx * font->width;
    int sy = cy * font->height;
    int bp_line = (font->width + 7) / 8;
    unsigned char* glyph =
        (unsigned char*)&_binary____font_psf_start +
        font->header_size +
        (c > 0 && c < font->num_glyph ? c : 0)*font->bp_glyph;
    int x, y, mask;
    for (y = 0; y < font->height; y++)
    {
        sx = cx * font->width;
        mask = 1 << (font->width - 1);
        for (x = 0; x < font->width; x++)
        {
            kscreen_putp(sx, sy, *((unsigned int *)glyph) & mask ? fg : bg);
            mask >>= 1;
            sx++;
        }
        kscreen_putp(sx, sy, bg);
        glyph += bp_line;
        sy++;
    }
}

void kscreen_scroll()
{
    memcpy_ssealign(kscreen_buffer + (font->height * kgraphics.ppsl),
        kscreen_buffer + 2 * (font->height * kgraphics.ppsl),
        (ch - 2) * kgraphics.ppsl * font->height * 4);
    kscreen_drawrect(0, (cy - 1) * font->height, kgraphics.horizontal_res, font->height, bg);
    
    cy--;
}

inline void kscreen_next()
{
    if (++cx == cw)
    {
        cx = 0;
        if (++cy == ch)
            kscreen_scroll();
    }
}

void kscreen_printc(uint16_t c)
{
    if (c == '\r')
    {
        cx = 0;
        return;
    }
    else if (c == '\n')
    {
        cx = 0;
        if (++cy == ch)
            kscreen_scroll();
        return;
    }
    else if (c == '\0')
        kscreen_putc(' ');
    else
        kscreen_putc(c);
    
    kscreen_next();
}

void kscreen_prints(char* s)
{
    for (size_t i = 0; i < str_len(s); i++)
        kscreen_printc(s[i]);
}

// kscreen_putf - options %n for fg color and %m for bg color
void kscreen_putf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    uint64_t unsign;
    int64_t sign;
    char *s;
    char c;
    int length;
    int len;
    int j;
    
    for (int count = 0; count < str_len(fmt); count++)
    {
        if (fmt[count] != '%')
        {
            kscreen_printc(fmt[count]);
            continue;
        }

        count++;

        length = 0;
        len = 0;
        j = 0;

        if (fmt[count] >= '0' && fmt[count] <= '9')
        {
            while (fmt[count] >= '0' && fmt[count] <= '9')
            {
                length = 10 * length + fmt[count] - '0';
                count++;
            }
        }
        else if (fmt[count] == '*')
        {
            length = va_arg(arg, int);
            count++;
        }

        switch (fmt[count])
        {
            case 'n':
                unsign = va_arg(arg, uint64_t);
                fg = unsign;
                break;
            case 'm':
                unsign = va_arg(arg, uint64_t);
                bg = unsign;
                break;
            case 's':
                s = va_arg(arg, char *);
                if (length == 0)
                {
                    kscreen_prints(s);
                    break;
                }
                len = str_len(s);
                j = 0;
                while (j < length)
                {
                    if (j > len)
                        kscreen_printc(' ');
                    else
                        kscreen_printc(s[j]);
                    j++;
                }
                break;
            case 'c':
                c = va_arg(arg, int);
                if (c == 0)
                    break;
                kscreen_putc(c);
                kscreen_next();
                break;
            case 'b':
                unsign = va_arg(arg, uint64_t);
                s = str_utoa(unsign, 2);
                if (length == 0)
                {
                    kscreen_prints(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kscreen_printc('0');
                kscreen_prints(s);
                break;
            case 'd':
                sign = va_arg(arg, int64_t);
                s = str_itoa(sign, 10);
                if (length == 0)
                {
                    kscreen_prints(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kscreen_printc('0');
                kscreen_prints(s);
                break;
            case 'x':
                unsign = va_arg(arg, uint64_t);
                s = str_utoa(unsign, 16);
                if (length == 0)
                {
                    kscreen_prints(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kscreen_printc('0');
                kscreen_prints(s);
                break;
            default:
                kscreen_printc(fmt[count]);
                break;
        }
    }

    va_end(arg);

    kscreen_copy();
}

kscreen_pos kscreen_getpos()
{
    kscreen_pos out;
    out.x = cx;
    out.y = cy;
    return out;
}

void kscreen_setpos(kscreen_pos pos)
{
    cx = pos.x;
    cy = pos.y;
}

void kscreen_init()
{
    memcpy(&kgraphics, &k_boottable.graphics, sizeof(kgraphics));
    //assume 32 bpp as is standard from UEFI's GOP
    
    kgraphics.framebuffer_base = kmem_page((uint64_t)kgraphics.framebuffer_base, kgraphics.horizontal_res * kgraphics.vertical_res * 4, 0b11);
    
    cw = kgraphics.horizontal_res / ((psf_font *)&_binary____font_psf_start)->width;
    ch = kgraphics.vertical_res / ((psf_font *)&_binary____font_psf_start)->height;
    
    kscreen_buffer = kmem_alloc((kgraphics.horizontal_res * kgraphics.vertical_res * 4)/0x1000);

#ifdef AQUA_DEBUG
    kdebug_outf("\r\nkscr: buffer [0x%x]", (uintptr_t)kscreen_buffer);
    kdebug_outf("\r\nkscr: [%d]x[%d] @ 32 bpp", kgraphics.horizontal_res, kgraphics.vertical_res);
    kdebug_outf("\r\nkscr: framebuffer [0x%x]", kgraphics.framebuffer_base);
    kdebug_outf("\r\nkscr: font [0x%x]", &_binary____font_psf_start);
    kdebug_outf("\r\nkscr: terminal %dx%d", cw, ch);
#endif
}