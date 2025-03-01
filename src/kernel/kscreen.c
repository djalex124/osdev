#include <screen.h>
#include <serial.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <mem.h>
#include <debug.h>
#include <kstring.h>

extern kernel_table ktable;
static graphics_info kgraphics;

void kscreen_clr(uint32_t color)
{
    for (int y = 0; y < kgraphics.vertical_res; y++)
    {
        for (int x = 0; x < kgraphics.horizontal_res; x++)
        {
            *((uint32_t*)(kgraphics.framebuffer_base + y*kgraphics.ppsl*4 + x*4))=color;
        }
    }
}

extern char _binary____font_psf_start[];
static uint32_t fg, bg;
static unsigned int cx = 0, cy = 0, cw = 0, ch = 0;

void kscreen_putc(uint16_t c)
{
    psf_font *font = (psf_font *)&_binary____font_psf_start;
    int scanline = kgraphics.ppsl;
    int bp_line = (font->width + 7) / 8;
    unsigned char* glyph =
        (unsigned char*)&_binary____font_psf_start +
        font->header_size +
        (c > 0 && c < font->num_glyph ? c : 0)*font->bp_glyph;
    int offset =
        (cy * font->height * scanline) + 
        (cx * font->width * sizeof(uint32_t));
    int x, y, line, mask;
    for (y = 0; y < font->height + 1; y++)
    {
        line = offset;
        mask = 1 << (font->width - 1);
        for (x = 0; x < font->width; x++)
        {
            *((uint32_t *)(kgraphics.framebuffer_base + line)) = *((unsigned int *)glyph) & mask ? fg : bg;
            mask >>= 1;
            line += sizeof(uint32_t);
        }
        *((uint32_t *)(kgraphics.framebuffer_base + line)) = bg;
        glyph += bp_line;
        offset += scanline;
    }
}

void kscreen_scroll()
{
    memcpy((uint64_t *)kgraphics.framebuffer_base, (uint64_t *)(kgraphics.framebuffer_base + kgraphics.ppsl * 16), (kgraphics.vertical_res - 16) * kgraphics.ppsl);
    memset((uint64_t *)(kgraphics.framebuffer_base + (kgraphics.vertical_res - 16) * kgraphics.ppsl), bg, kgraphics.ppsl * 16);
    cy--;
}

void kscreen_printc(uint16_t c)
{
    kserial_outf("%c", c);
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
    else
        kscreen_putc(c);
    
    if (++cx == cw)
    {
        cx = 0;
        if (++cy == ch)
            kscreen_scroll();
    }
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

    uint64_t i;
    int64_t d;
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
                i = va_arg(arg, uint64_t);
                fg = i;
                break;
            case 'm':
                i = va_arg(arg, uint64_t);
                bg = i;
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
                kscreen_printc(c);
                break;
            case 'b':
                i = va_arg(arg, uint64_t);
                s = str_itoa(i, 2);
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
                d = va_arg(arg, int64_t);
                s = str_itoa(d, 10);
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
                i = va_arg(arg, uint64_t);
                s = str_itoa(i, 16);
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
    kgraphics.framebuffer_base = ktable.graphics.framebuffer_base;
    kgraphics.horizontal_res = ktable.graphics.horizontal_res;
    kgraphics.vertical_res = ktable.graphics.vertical_res;
    kgraphics.ppsl = ktable.graphics.ppsl;
    kdebug_outf("\r\nkscr: %x %x", &ktable, &kgraphics);
    kdebug_outf("\r\nkscr: [%d]x[%d] @ %d bpp", kgraphics.horizontal_res, kgraphics.vertical_res, kgraphics.ppsl);
    kmem_page((uint64_t)kgraphics.framebuffer_base, (uint64_t)kgraphics.framebuffer_base + kernel_virtual, kgraphics.horizontal_res * kgraphics.vertical_res * 4, 0b11);
    kdebug_outf("\r\nkscr: framebuffer [0x%x]", kgraphics.framebuffer_base);
    kgraphics.framebuffer_base = (uint64_t*)((uint64_t)kgraphics.framebuffer_base + kernel_virtual);
    kdebug_outf("\r\nkscr: font [0x%x]", &_binary____font_psf_start);
    cw = kgraphics.horizontal_res/((psf_font *)&_binary____font_psf_start)->width;
    ch = kgraphics.vertical_res/((psf_font *)&_binary____font_psf_start)->height;
    kdebug_outf("\r\nkscr: terminal %dx%d", cw, ch);
}