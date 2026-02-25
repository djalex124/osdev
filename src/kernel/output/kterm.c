#include <limits.h>
#include <cpuid.h>
#include <stdarg.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/crash.h>
#include <kernel/debug.h>

#include <output/screen.h>
#include <output/image.h>
#include <output/kterm.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <ps2/mouse.h>
#include <ps2/kbd.h>

#include <mm/mem.h>

#include <fs/fs.h>

#define kterm_buffersize 100
#define kterm_maxargs 16

#define kterm_titletext1 "[AQUA Kernel]"
#define kterm_titletext2 "Build: (" AQUA_VER_STRING ")"

#define kterm_infotext "[AQUA Kernel (" AQUA_VER_STRING ")]\n[Built " __TIME__" "__DATE__ " Central Time]\n[Quote: Never back down, never give up!]"

kscreen_pos kterm_pos;
char kterm_buffer[kterm_buffersize];
uint8_t kterm_bufferindex = 0;

char *kterm_argv[kterm_maxargs];
unsigned int kterm_argc;
char *kterm_prompt = "aqua >";

kkeyboard_state *kterm_next;
uint8_t kterm_changed = 0;

extern graphics_info kgraphics;
uint32_t *kterm_gbuffer;

int kterm_currentpartition = -1;

extern uint8_t kacpi_apsrunning;

extern char _binary____font_psf_start[];
uint32_t fg = 0xC5C5C5, bg = default_color;
unsigned int cx = 0, cy = 0, cw = 0, ch = 0;
psf_font *font;

kscreen_pos update_header;

static inline void kterm_putp(int x, int y, uint32_t color)
{
    if (x >= kgraphics.horizontal_res || x < 0)
        return;
    else if (y >= kgraphics.vertical_res || y < 0)
        return;
    
    unsigned where = x*4 + y*kgraphics.ppsl*4;
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
    kterm_drawrect(0, 0, kgraphics.horizontal_res, kgraphics.vertical_res, color);
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
    memcpy_ssealign(kterm_gbuffer + (font->height * kgraphics.ppsl),
        kterm_gbuffer + 2 * (font->height * kgraphics.ppsl),
        (ch - 2) * kgraphics.ppsl * font->height * 4);
    kterm_drawrect(0, (cy - 1) * font->height, kgraphics.horizontal_res, font->height, bg);
    
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

    kterm_drawrect(0, 0, kgraphics.horizontal_res, font->height, bg);

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

void kterm_input(kkeyboard_state *k)
{
    kterm_next = k;
    kterm_changed = 1;
}

void kterm_run()
{
    char *arg = str_tok(kterm_buffer, " ");
    while (arg && kterm_argc < kterm_maxargs - 1)
    {
        kterm_argv[kterm_argc++] = arg;
        arg = str_tok((void *)0, " ");
    }
    kterm_argv[kterm_argc] = 0;

    if (kterm_argv[0] == NULL)
        return;
    else if (str_cmp(kterm_argv[0], "clear") == 0)
    {
        kterm_clr(bg);
    }
    else if (str_cmp(kterm_argv[0], "color") == 0)
    {
        if (kterm_argc == 1)
            kterm_setcolor(0xC5C5C5, default_color);
        else if (kterm_argc == 3)
        {
            int64_t new_fg, new_bg;
            new_fg = str_atoi(kterm_argv[1]);
            new_bg = str_atoi(kterm_argv[2]);
            if (new_fg > 0xFFFFFFFF || new_fg < 0 ||
                new_bg > 0xFFFFFFFF || new_bg < 0)
                kterm_putf("\nInvalid colors.");
            else
                kterm_setcolor(new_fg, new_bg);
        }
        else
            kterm_putf("\nInvalid argument count.");
    }
    else if (str_cmp(kterm_argv[0], "compare") == 0)
    {
        if (kterm_argc < 3)
            kterm_putf("\nNot enough arguments.");
        else
        {
            long a = 0, b = 0;
            if (kterm_argv[1])
                a = str_atoi(kterm_argv[1]);
            if (kterm_argv[2])
                b = str_atoi(kterm_argv[2]);
            kterm_putf("\nlarger number is: ");
            if (a == b)
                kterm_putf("both numbers (%d) (%d)", a, b);
            else if (a > b)
                kterm_putf("number 1 (%d)", a);
            else
                kterm_putf("number 2 (%d)", b);
        }
    }
    else if (str_cmp(kterm_argv[0], "cpu_info") == 0)
    {
        unsigned int ax, bx, cx, dx;
        __cpuid(0, ax, bx, cx, dx);

        kterm_putf("\n - Vendor [%4s%4s%4s]", &bx, &dx, &cx);

        __cpuid(0x80000000, ax, bx, cx, dx);
        if (ax >= 0x80000004)
        {
            uint32_t* name = kmem_alloc(1);
            __cpuid(0x80000002, name[0], name[1], name[2], name[3]);
            __cpuid(0x80000003, name[4], name[5], name[6], name[7]);
            __cpuid(0x80000004, name[8], name[9], name[10], name[11]);
            name[12] = 0;
            
            str_trim((char *)name);
            kterm_putf(" Brand [%s]", name);
            kmem_free(name, 1);
        }

        __cpuid(1, ax, bx, cx, dx);
        kterm_putf("\n - Features Tracked:");
        if (dx & (1 << 25))
            kterm_putf(" SSE");
        if (dx & (1 << 26))
            kterm_putf(" SSE2");
        if (cx & (1 << 0))
            kterm_putf(" SSE3");
        if (cx & (1 << 9))
            kterm_putf(" SSSE3");
        if (cx & (1 << 19))
            kterm_putf(" SSE4.1");
        if (cx & (1 << 20))
            kterm_putf(" SSE4.2");
        if (cx & (1 << 28))
            kterm_putf(" AVX");

        kterm_putf("\n - Total APs Running: %d", kacpi_apsrunning);
    }
    else if (str_cmp(kterm_argv[0], "crash") == 0)
    {
        kterm_putf("\nInitiating crash...");
        kcrash("User Requested");
    }
    else if (str_cmp(kterm_argv[0], "fs") == 0)
    {
        if (kterm_argc < 2)
            kterm_putf("\nNot enough arguments.");
        else
        {
            long part = 0;
            if (kterm_argv[1])
                part = str_atoi(kterm_argv[1]);
            
            if (part < 0 || part > 15)
            {
                kterm_putf("\nInvalid partition selection.");
                kterm_currentpartition = -1;
            }
            else if ((uint64_t)k_infotable.kfs_partitions[part] == 0)
            {
                kterm_putf("\nPartition does not exist.");
                kterm_currentpartition = -1;
            }
            else
            {
                kterm_putf("\nPartition set to %d.", part);
                kterm_currentpartition = part;
            }
        }
    }
    else if (str_cmp(kterm_argv[0], "fs_info") == 0)
    {
        kfs_printinfo();
        kterm_putf("\nmounted partitions:");

        int partitions = 0;
        for (int i = 0; i < 16; i++)
        {
            if ((uint64_t)k_infotable.kfs_partitions[i])
            {
                kterm_putf("\n - partition %d:", i);
                kfs_printpartition(k_infotable.kfs_partitions[i]);
                partitions = 1;
            }
        }

        if (partitions == 0)
            kterm_putf("\n - No partitions mounted");
    }
    else if (str_cmp(kterm_argv[0], "font") == 0)
    {
        kterm_putf("\n");
        uint8_t c = 0;
        for (; c < 255; c++)
            kterm_putf("%c", c);
    }
    else if (str_cmp(kterm_argv[0], "help") == 0)
    {
        kterm_putf("\nList of currently available commands:");
        kterm_putf("\n clear - clears the screen");
        kterm_putf("\n color [fg] [bg] - set terminal colors in base10 of hex code, no values to reset");
        kterm_putf("\n compare [num1] [num2] - compares two numbers and prints out the largest");
        kterm_putf("\n cpu_info - lists CPU model and capabilities");
        kterm_putf("\n crash - crashes the AQUA kernel");
        kterm_putf("\n fs - sets the currently selected partition for file operations");
        kterm_putf("\n fs_info - lists detected disks and drives");
        kterm_putf("\n font - prints all characters in boot font");
        kterm_putf("\n help - lists available commands");
        kterm_putf("\n image [filename] - attempt printing .tga image to screen from file");
        kterm_putf("\n info - prints current AQUA build information");
        kterm_putf("\n mem_info - prints current memory usage");
        kterm_putf("\n pci_info - prints pci busses and devices");
        kterm_putf("\n read_file [filename] - attempt read of file on selected partition");
        kterm_putf("\n shutdown - attempts acpi shutdown");
        kterm_putf("\n test - test random features");
        kterm_putf("\n test_mouse - tests ps2 mouse input");
        kterm_putf("\n wait [num1] - wait given number of seconds");
    }
    else if (str_cmp(kterm_argv[0], "image") == 0)
    {
        if (kterm_argc < 2)
            kterm_putf("\nNot enough arguments.");
        else if (kterm_currentpartition < 0 || kterm_currentpartition > 15)
        {
            kterm_putf("\nInvalid partition selection.");
            kterm_currentpartition = -1;
        }
        else if ((uint64_t)k_infotable.kfs_partitions[kterm_currentpartition] == 0)
        {
            kterm_putf("\nPartition does not exist.");
            kterm_currentpartition = -1;
        }
        else
        {
            char *filename = 0;
            if (kterm_argv[1])
                filename = kterm_argv[1];
            
            size_t file_length;
            uint8_t *file = 0;
            size_t image_pages = 0;
    
            kfs_partition *selected_partition = k_infotable.kfs_partitions[kterm_currentpartition];
            if (selected_partition->fs == 1)
                file = kfs_readfilefat(selected_partition, filename, &file_length);

            if ((uint64_t)file == 0)
            {
                kterm_putf("\nUnable to read file.");
                return;
            }

            if (file[0] != 0 || file[1] != 0 || file[2] != 0x0A || file[3] != 0 || file[4] != 0
                || file[5] != 0 || file[6] != 0 || file[7] != 0 || file[8] != 0 || file[9] != 0
                || (file[16] != 24 && file[16] != 32))
            {
                kterm_putf("\nInvalid tga file.");
            }
            else
            {
                uint32_t *image_pixels = kimage_getbuftga(file, (int)file_length, &image_pages);
                kimage_termblit(image_pixels, 100, 100);
                kmem_free(image_pixels, image_pages);
            }

            kmem_free(file, (file_length + 0x1000 - 1) / 0x1000);
        }
    }
    else if (str_cmp(kterm_argv[0], "info") == 0)
    {
        kterm_putf("\n%s", kterm_infotext);
    }
    else if (str_cmp(kterm_argv[0], "mem_info") == 0)
    {
        kmem_printinfo();
    }
    else if (str_cmp(kterm_argv[0], "pci_info") == 0)
    {
        kterm_putf("\nkpci_info: current pci device table");
        kpci_device* kpci_table = k_infotable.kpci_table;
        for (int i = 0; i < k_infotable.kpci_tablesize; i++)
        {
            kterm_putf("\n - %2x:%2x:%2x:%2x ", kpci_table[i].section, kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function);
            kterm_putf("VendorID %4x DeviceID %4x [%s]/[%s]",
                kpci_getvendorid(&kpci_table[i]), kpci_getdeviceid(&kpci_table[i]),
                kpci_getclassname(kpci_getbaseclass(&kpci_table[i])),
                kpci_getsubclassname(kpci_getbaseclass(&kpci_table[i]), kpci_getsubclass(&kpci_table[i])));
        }
    }
    else if (str_cmp(kterm_argv[0], "read_file") == 0)
    {
        if (kterm_argc < 2)
            kterm_putf("\nNot enough arguments.");
        else if (kterm_currentpartition < 0 || kterm_currentpartition > 15)
        {
            kterm_putf("\nInvalid partition selection.");
            kterm_currentpartition = -1;
        }
        else if ((uint64_t)k_infotable.kfs_partitions[kterm_currentpartition] == 0)
        {
            kterm_putf("\nPartition does not exist.");
            kterm_currentpartition = -1;
        }
        else
        {
            char *filename = 0;
            if (kterm_argv[1])
                filename = kterm_argv[1];
            kfs_printreadfile(kterm_currentpartition, filename);
        }
    }
    else if (str_cmp(kterm_argv[0], "shutdown") == 0)
    {
        kterm_putf("\nTrying to shutdown from ACPI...");
        kacpi_shutdown();
    }
    else if (str_cmp(kterm_argv[0], "test") == 0)
    {
        kterm_putf("\ntest output of the commands!!");
        uint16_t* test = kmem_palloc(2, kmem_paging_1kb);
        uint16_t* test2 = kmem_page((uint64_t)&test[15], 0x1000, kmem_paging_present | kmem_paging_writable, kmem_paging_1kb);
        kterm_putf("\ntest %x", (uint64_t)test2);
        test2[32] = 0xCA;
        kterm_putf("\ntest2[32] %x", test2[32]);
        kmem_unpage(test2, 0x1000);
        kmem_pfree(test, 2);
        uint16_t* test3 = kmem_alloc(1024);
        kterm_putf("\ntest3 %x", test3);
        kmem_free(test3, 1024);
        kterm_putf("\nwait a few second :) -");
        for (uint16_t i = 1; i <= 5; i++)
        {
            ksleep(1000);
            kterm_putf(" %d", i);
        }
    }
    else if (str_cmp(kterm_argv[0], "test_mouse") == 0)
    {
        kmouse_test();
        kkeyboard_setinput(*kterm_input);
    }
    else if (str_cmp(kterm_argv[0], "wait") == 0)
    {
        if (kterm_argc < 2)
            kterm_putf("\nNot enough arguments.");
        else
        {
            int64_t input = 0;
            if (kterm_argv[1])
                input = str_atoi(kterm_argv[1]);
            if (input >= 0)
            {    
                kterm_putf("\nWaiting %d seconds...", input);
                ksleep(input * 1000);
            }
            else
                kterm_putf("\nInvalid number.");
        }
    }
    else
    {
        kterm_putf("\nCommand \'%s\' not found.\nUse the command \'help\' to list available commands.", kterm_argv[0]);
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
                kterm_putf("(%d) ", kterm_currentpartition);
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
    cw = kgraphics.horizontal_res / ((psf_font *)&_binary____font_psf_start)->width;
    ch = kgraphics.vertical_res / ((psf_font *)&_binary____font_psf_start)->height;

    update_header.x = cw - str_len(kterm_titletext2);
    update_header.y = 0;
    
    kterm_gbuffer = kscreen_setterm();
    kdebug_outf("\nkterm: kterm_gbuffer %x", (uintptr_t)kterm_gbuffer);

    kterm_clr(bg);
    
    kterm_draw = 1;
    kscreen_copy();
}