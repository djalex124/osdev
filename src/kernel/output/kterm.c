#include <stdarg.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <output/screen.h>
#include <output/kterm.h>
#include <output/kcmd.h>

#include <ps2/kbd.h>

#include <mm/mem.h>

kscreen_pos kterm_pos;
char kterm_buffer[kterm_buffersize];
uint8_t kterm_bufferindex = 0;

char *kterm_argv[kterm_maxargs];
unsigned int kterm_argc;
char *kterm_prompt = "aqua >";

kkeyboard_state *kterm_next;
uint8_t kterm_changed = 0;

uint32_t *kterm_gbuffer;

char *kterm_currentdir = 0;
int kterm_currentpartition = -1;

extern char _binary____font_psf_start[];
uint32_t fg = 0xC5C5C5, bg = default_color;
uint16_t cx = 0, cy = 0, cw = 0, ch = 0;
psf_font *font;

kscreen_pos update_header;

static inline void kterm_putp(int x, int y, uint32_t color)
{
    if (x >= k_infotable.k_graphics->horizontal_res || x < 0)
        return;
    else if (y >= k_infotable.k_graphics->vertical_res || y < 0)
        return;
    
    unsigned where = x*4 + y*k_infotable.k_graphics->ppsl*4;
    ((unsigned char*)kterm_gbuffer)[where] = color & 0xFF;
    ((unsigned char*)kterm_gbuffer)[where + 1] = (color >> 8) & 0xFF;
    ((unsigned char*)kterm_gbuffer)[where + 2] = (color >> 16) & 0xFF;
}

void kterm_drawrect(int x, int y, int w, int h, uint32_t color)
{
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            kterm_putp(x + j, y + i, color);
}

void kterm_clr(uint32_t color)
{
    kterm_drawrect(0, 0, k_infotable.k_graphics->horizontal_res, k_infotable.k_graphics->vertical_res, color);
    cx = 0;
    cy = 1;
}

void kterm_putc(uint16_t c)
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
            kterm_putp(sx, sy, *((unsigned int *)glyph) & mask ? fg : bg);
            mask >>= 1;
            sx++;
        }
        //kscreen_putp(sx, sy, bg);
        glyph += bp_line;
        sy++;
    }
}

void kterm_scroll()
{
    memcpy_ssealign(kterm_gbuffer + (font->height * k_infotable.k_graphics->ppsl),
        kterm_gbuffer + 2 * (font->height * k_infotable.k_graphics->ppsl),
        (ch - 2) * k_infotable.k_graphics->ppsl * font->height * 4);
    kterm_drawrect(0, (cy - 1) * font->height, k_infotable.k_graphics->horizontal_res, font->height, bg);
    
    cy--;
}

inline void kterm_nextline()
{
    if (++cx == cw)
    {
        cx = 0;
        if (++cy == ch)
            kterm_scroll();
    }
}

void kterm_printc(uint16_t c)
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
            kterm_scroll();
        return;
    }
    else if (c == '\0')
        kterm_putc(' ');
    else
        kterm_putc(c);
    
    kterm_nextline();
}

void kterm_prints(char* s)
{
    for (size_t i = 0; i < str_len(s); i++)
        kterm_printc(s[i]);
}

void kterm_update()
{
    kscreen_pos old_pos = kterm_getpos();
    uint32_t old_fg = fg, old_bg = bg;

    fg = 0;
    bg = 0xA9A9A9;

    cx = 0;
    cy = 0;

    kterm_drawrect(0, 0, k_infotable.k_graphics->horizontal_res, font->height, bg);

    kterm_prints(kterm_titletext1);

    kterm_setpos(update_header);
    kterm_prints(kterm_titletext2);

    kterm_setpos(old_pos);

    fg = old_fg;
    bg = old_bg;

    kscreen_copy();
}

void kterm_vputf(const char *fmt, va_list arg)
{
    uint32_t oldfg = fg, oldbg = bg;

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
            kterm_printc(fmt[count]);
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
                    kterm_prints(s);
                    break;
                }
                len = str_len(s);
                j = 0;
                while (j < length)
                {
                    if (j > len)
                        kterm_printc(' ');
                    else
                        kterm_printc(s[j]);
                    j++;
                }
                break;
            case 'c':
                c = va_arg(arg, int);
                if (c == 0)
                    break;
                kterm_putc(c);
                kterm_nextline();
                break;
            case 'b':
                unsign = va_arg(arg, uint64_t);
                s = str_utoa(unsign, 2);
                if (length == 0)
                {
                    kterm_prints(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kterm_printc('0');
                kterm_prints(s);
                break;
            case 'd':
                sign = va_arg(arg, int64_t);
                s = str_itoa(sign, 10);
                if (length == 0)
                {
                    kterm_prints(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kterm_printc('0');
                kterm_prints(s);
                break;
            case 'x':
                unsign = va_arg(arg, uint64_t);
                s = str_utoa(unsign, 16);
                if (length == 0)
                {
                    kterm_prints(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kterm_printc('0');
                kterm_prints(s);
                break;
            default:
                kterm_printc(fmt[count]);
                break;
        }
    }

    fg = oldfg;
    bg = oldbg;
}

// kterm_nhputf - options %n for fg color and %m for bg color
// print without header updating
void kterm_nhputf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    kterm_vputf(fmt, arg);

    va_end(arg);

    kscreen_copy();
}

// kterm_putf - options %n for fg color and %m for bg color
void kterm_putf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    kterm_vputf(fmt, arg);

    va_end(arg);
    
    kterm_update();
}

kscreen_pos kterm_getpos()
{
    kscreen_pos out;
    out.x = cx;
    out.y = cy;
    return out;
}

void kterm_setpos(kscreen_pos pos)
{
    cx = pos.x;
    cy = pos.y;
}

uint32_t kterm_getfg()
{
    return fg;
}

uint32_t kterm_getbg()
{
    return bg;
}

void kterm_setcolor(uint32_t foreground, uint32_t background)
{
    fg = foreground;
    bg = background;
}

char *kterm_getabspath(char *fullpath)
{
    char *absolutepath = kmem_kalloc(str_len(fullpath) + 2);

    int count_index = 0;
    int count_len = str_len(fullpath);
    int count_absindex = 0;

    char last_char = 0;

    while (count_index < count_len && fullpath[count_index])
    {
        if (fullpath[count_index] == '/')
        {
            if (last_char != '/')
                absolutepath[count_absindex++] = fullpath[count_index];
        }
        else
            absolutepath[count_absindex++] = fullpath[count_index];
        last_char = fullpath[count_index];
        count_index++;
    }

    if (last_char == '/')
    {
        absolutepath[count_len] = 0;
    }
    else
    {
        absolutepath[count_len] = '/';
        absolutepath[count_len + 1] = 0;
    }

    count_index = 0;
    count_len = str_len(absolutepath);
    while (count_index < count_len && absolutepath[count_index])
    {
        if (strn_cmp(absolutepath + count_index, "../", 3) == 0)
        {
            int prev_len = 0;
            if (count_index - 2 >= 0)
                for (int i = count_index - 2; absolutepath[i] != '/' && i >= 0; i--, prev_len++);
            else
                prev_len = -1;

            memcpy(absolutepath + count_index - prev_len - 2, absolutepath + count_index + 2, count_len - count_index + 1);

            count_index = count_index - prev_len - 2;
        }

        count_index++;
    }

    count_index = 0;
    count_len = str_len(absolutepath);
    while (count_index < count_len && absolutepath[count_index])
    {
        if (strn_cmp(absolutepath + count_index, "./", 2) == 0)
        {
            memcpy(absolutepath + count_index, absolutepath + count_index + 2, count_len - count_index + 1);
            count_index -= 1;
        }

        count_index++;
    }

    return absolutepath;
}

char *kterm_getrelpath(char *filepath)
{
    size_t pathlen = str_len(filepath);
    size_t dirlen = str_len(kterm_currentdir);
    char *fullname;

    if (filepath[0] == '/')
    {
        fullname = kmem_kalloc(pathlen + 1);
        memcpy(fullname, filepath, pathlen);

        return fullname;
    }

    fullname = kmem_kalloc(pathlen + dirlen + 1);
    memcpy(fullname, kterm_currentdir, dirlen);
    memcpy(fullname + dirlen, filepath, pathlen);

    return fullname;
}

char *kterm_getdir()
{
    return kterm_currentdir;
}

void kterm_setdir(char *dir)
{
    if ((uint64_t)dir == 0)
    {
        kterm_currentdir = kmem_kalloc(2);
        kterm_currentdir[0] = '/';
        return;
    }

    if (kterm_currentdir)
        kmem_kfree(kterm_currentdir);
    
    kterm_currentdir = kmem_kalloc(str_len(dir) + 1);
    memcpy(kterm_currentdir, dir, str_len(dir));
}

int kterm_getpartition()
{
    return kterm_currentpartition;
}

void kterm_setpartition(int fs)
{
    kterm_currentpartition = fs;
    if (fs == -1)
        kterm_setdir(0);
}

void kterm_input(kkeyboard_state *k)
{
    kterm_next = k;
    kterm_changed = 1;
}

void kterm_run()
{
    str_trim(kterm_buffer);
    int index = 0;

    while (index < kterm_bufferindex && kterm_argc < kterm_maxargs)
    {
        char *current = &kterm_buffer[index];
        char splitting_char = ' ';
        int end = index;

        while (*current != splitting_char && *current != 0)
        {
            if (*current == '\'' || *current == '\"')
            {
                index++;
                splitting_char = *current;
            }

            current++;
            end++;
        }

        if (end == index)
        {
            index++;
            continue;
        }
        
        kterm_argv[kterm_argc] = kmem_kalloc(end - index + 1);
        memcpy(kterm_argv[kterm_argc], &kterm_buffer[index], end - index);
        kterm_argv[kterm_argc][end - index] = 0;
        index = end + 1;
        
        kterm_argc++;
    }

    if (kterm_argv[0] == NULL)
        return;

    kcmd_runcommand(kterm_argv, kterm_argc);

    for (int i = 0; i < kterm_argc; i++)
    {
        if (kterm_argv[i])
            kmem_kfree(kterm_argv[i]);
    }
}

inline void kterm_prevtermpos()
{
    if (kterm_pos.x == 0)
    {
        kterm_pos.y--;
        kterm_pos.x = cw - 1;
    }
    else
        kterm_pos.x--;
}

void kterm_processinput()
{
    char key = kkeyboard_keymapUSqwerty[kterm_next->scancode];
    if (key == 0 || kterm_next->pressed == 0)
        return;

    switch (key)
    {
        case '\e':
        case '\t':
            break;
        case '\b':
            if (kterm_bufferindex)
            {
                kterm_prevtermpos();
                kscreen_pos p = kterm_pos;
                kterm_prevtermpos();
                kterm_setpos(kterm_pos);
                kterm_putf("%c ", 128);
                kterm_setpos(p);
                kterm_bufferindex--;
                kterm_buffer[kterm_bufferindex] = 0;
            }
            break;
        case '\r':
            kterm_prevtermpos();
            kterm_setpos(kterm_pos);
            kterm_putf(" ");
            kterm_setpos(kterm_pos);
            kterm_run();
            memset(kterm_argv, 0, sizeof(kterm_argv));
            kterm_argc = 0;
            memset(kterm_buffer, 0, sizeof(kterm_buffer));
            kterm_bufferindex = 0;
            kterm_pos = kterm_getpos();
            if (kterm_pos.x != 0 && kterm_pos.y != ch)
                kterm_putf("\n");
            if (kterm_currentpartition != -1)
                kterm_putf("(%d:%s) ", kterm_currentpartition, kterm_currentdir);
            kterm_putf("%s%c", kterm_prompt, 128);
            break;
        default:
            if (kterm_bufferindex == kterm_buffersize - 1)
                break;
            kterm_prevtermpos();
            kterm_setpos(kterm_pos);
            uint8_t upper = 0;
            if (key >= 'a' && key <= 'z')
            {
                if (kterm_next->capslk ^ (kterm_next->lshift | kterm_next->rshift))
                    upper = 1;
            }
            else if (kterm_next->lshift | kterm_next->rshift)
                upper = 1;
            if (upper)
                key = kkeyboard_keymapUSqwerty_upper[kterm_next->scancode];
            kterm_putf("%c%c", key, 128);
            kterm_buffer[kterm_bufferindex] = key;
            kterm_bufferindex++;
            break;
    }

    kterm_pos = kterm_getpos();
}

void kterm_loop()
{
    kkeyboard_setinput(*kterm_input);
    
    kterm_putf("Welcome to ConcatenOS!");
    kterm_putf("\nTo get started, run 'help' for a list of commands.");
    kterm_putf("\n%s%c", kterm_prompt, 128);
    kterm_pos = kterm_getpos();

    for (;;)
    {
        while (kterm_changed == 0)
        {
            asm("hlt");
            continue;
        }
        kterm_processinput();
        kterm_changed = 0;
    }
}

uint8_t kterm_draw = 0;

void kterm_init()
{
    cw = k_infotable.k_graphics->horizontal_res / ((psf_font *)&_binary____font_psf_start)->width;
    ch = k_infotable.k_graphics->vertical_res / ((psf_font *)&_binary____font_psf_start)->height;
    kdebug_outf("\nkterm: kterm size %dx%x", cw, ch);

    update_header.x = cw - str_len(kterm_titletext2);
    update_header.y = 0;
    
    kterm_gbuffer = kscreen_setterm();
    kdebug_outf("\nkterm: kterm_gbuffer %x", (uintptr_t)kterm_gbuffer);

    kterm_clr(bg);
    
    kterm_draw = 1;
    kscreen_copy();
}