#include <kstring.h>
#include <screen.h>
#include <kbd.h>
#include <mem.h>
#include <pci.h>
#include <limits.h>
#include <cpuid.h>
#include <kernel.h>
#include <pit.h>
#include <fs.h>

#define kterm_buffersize 100
#define kterm_maxargs 16

#define kterm_titletext "[AQUA Kernel (" AQUA_VER_STRING ")] - [Built " __TIMESTAMP__ " Central Time]"

static kscreen_pos kterm_pos;
static char kterm_buffer[kterm_buffersize];
static uint8_t kterm_bufferindex = 0;

static char *kterm_argv[kterm_maxargs];
static unsigned int kterm_argc;

uint32_t kterm_fg = 0xC5C5C5;
uint32_t kterm_bg = default_color;

void kterm_header()
{
    kterm_pos.x = 0;
    kterm_pos.y = 0;
    kscreen_setpos(kterm_pos);
    kscreen_putf("%m%n%s%n%m", 0xA9A9A9, 0, kterm_titletext, kterm_fg, kterm_bg);
}

void kterm_run()
{
    char *arg = str_tok(kterm_buffer, " ");
    while (arg && kterm_argc < kterm_maxargs - 1)
    {
        kterm_argv[kterm_argc++] = arg;
        arg = str_tok(0, " ");
    }
    kterm_argv[kterm_argc] = 0;

    if (str_cmp(kterm_argv[0], "clear") == 0)
    {
        kscreen_clr(default_color);
        kterm_header();
    }
    else if (str_cmp(kterm_argv[0], "compare") == 0)
    {
        if (kterm_argc < 3)
        {
            kscreen_putf("\nNot enough arguments.");
            return;
        }
        long a = 0, b = 0;
        if (kterm_argv[1])
            a = str_atoi(kterm_argv[1]);
        if (kterm_argv[2])
            b = str_atoi(kterm_argv[2]);
        kscreen_putf("\nlarger number is: ");
        if (a == b)
            kscreen_putf("both numbers (%d) (%d)", a, b);
        else if (a > b)
            kscreen_putf("number 1 (%d)", a);
        else
            kscreen_putf("number 2 (%d)", b);
    }
    else if (str_cmp(kterm_argv[0], "cpu_info") == 0)
    {
        unsigned int ax, bx, cx, dx;
        __cpuid(0, ax, bx, cx, dx);

        kscreen_putf("\n - Vendor [%4s%4s%4s]", &bx, &dx, &cx);

        __cpuid(0x80000000, ax, bx, cx, dx);
        if (ax >= 0x80000004)
        {
            uint32_t* name = kmem_alloc(1);
            __cpuid(0x80000002, name[0], name[1], name[2], name[3]);
            __cpuid(0x80000003, name[4], name[5], name[6], name[7]);
            __cpuid(0x80000004, name[8], name[9], name[10], name[11]);
            name[12] = 0;
            kscreen_putf(" Brand [%s]", name);
            kmem_free(name, 1);
        }

        __cpuid(1, ax, bx, cx, dx);
        kscreen_putf("\n - Features Tracked:");
        if (dx & (1 << 25))
            kscreen_putf(" SSE");
        if (dx & (1 << 26))
            kscreen_putf(" SSE2");
        if (cx & (1 << 0))
            kscreen_putf(" SSE3");
        if (cx & (1 << 9))
            kscreen_putf(" SSSE3");
        if (cx & (1 << 19))
            kscreen_putf(" SSE4.1");
        if (cx & (1 << 20))
            kscreen_putf(" SSE4.2");
        if (cx & (1 << 28))
            kscreen_putf(" AVX");
    }
    else if (str_cmp(kterm_argv[0], "fs_info") == 0)
    {
        kfs_printinfo();
    }
    else if (str_cmp(kterm_argv[0], "help") == 0)
    {
        kscreen_putf("\nList of currently available commands:");
        kscreen_putf("\n clear - clears the screen");
        kscreen_putf("\n compare [num1] [num2] - compares two numbers and prints out the largest");
        kscreen_putf("\n cpu_info - lists CPU model and capabilities");
        kscreen_putf("\n fs_info - lists detected disks and drives");
        kscreen_putf("\n help - lists available commands");
        kscreen_putf("\n mem_info - prints current memory usage");
        kscreen_putf("\n pci_info - prints pci busses and devices");
        kscreen_putf("\n test - test random features");
        kscreen_putf("\n wait [num1] - wait given number of seconds");
    }
    else if (str_cmp(kterm_argv[0], "mem_info") == 0)
    {
        kmem_printinfo();
    }
    else if (str_cmp(kterm_argv[0], "pci_info") == 0)
    {
        kscreen_putf("\nkpci_info: current pci device table");
        kpci_device* kpci_table = k_infotable.kpci_table;
        uint8_t progif;
        for (int i = 0; i < k_infotable.kpci_tablesize; i++)
        {
            progif = kpci_configread(kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function, PCI_OFFSET_PROGIF) & 0xFF;
            kscreen_putf("\n - Bus %2x Device %2x Function %2x", kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function);
            kscreen_putf(": VendorID %4x ProgIF %2x [%s]/[%s]",
                kpci_table[i].vendorid, progif,
                kpci_getclassname(kpci_table[i].class),
                kpci_getsubclassname(kpci_table[i].class, kpci_table[i].subclass));
        }
    }
    else if (str_cmp(kterm_argv[0], "test") == 0)
    {
        kscreen_putf("\ntest output of the commands!!");
        uint16_t* test = kmem_alloc(1);
        kscreen_putf("\ntest %x", (uint64_t)test);
        test[32] = 0xCA;
        kscreen_putf("\ntest[32] %x %d", test[32], 10);
        kmem_free(test, 1);
        kscreen_putf("\nwait a few second :) -");
        for (uint16_t i = 1; i <= 5; i++)
        {
            ksleep(1000);
            kscreen_putf(" %d", i);
        }
    }
    else if (str_cmp(kterm_argv[0], "wait") == 0)
    {
        if (kterm_argc < 2)
        {
            kscreen_putf("\nNot enough arguments.");
            return;
        }
        int64_t input = 0;
        if (kterm_argv[1])
            input = str_atoi(kterm_argv[1]);
        if (input >= 0)
        {    
            kscreen_putf("\nWaiting %d seconds...", input);
            ksleep(input * 1000);
        }
        else
            kscreen_putf("\nInvalid number.");
    }
    else if (kterm_argv[0] == NULL)
        return;
    else
    {
        kscreen_putf("\nCommand \'%s\' not found.\nUse the command \'help\' to list available commands.", kterm_argv[0]);
    }
}

char *kterm_prompt = "aqua >";
extern unsigned int cw;

kkeyboard_state *kterm_next;
uint8_t kterm_changed = 0;

void kterm_processinput()
{
    char key = kkeyboard_keymapUSqwerty[kterm_next->scancode];
    if (key == 0)
        return;

    switch (key)
    {
        case '\e':
        case '\t':
            break;
        case '\b':
            if (kterm_bufferindex)
            {
                if (kterm_pos.x == 0)
                {
                    kterm_pos.y--;
                    kterm_pos.x = cw - 1;
                }
                else
                    kterm_pos.x--;
                kscreen_setpos(kterm_pos);
                kscreen_putf(" ");
                kscreen_setpos(kterm_pos);
                kterm_bufferindex--;
                kterm_buffer[kterm_bufferindex] = 0;
            }
            break;
        case '\r':
            kterm_run();
            memset(kterm_argv, 0, sizeof(kterm_argv));
            kterm_argc = 0;
            memset(kterm_buffer, 0, sizeof(kterm_buffer));
            kterm_bufferindex = 0;
            kscreen_putf("\n%s", kterm_prompt);
            break;
        default:
            if (kterm_bufferindex == kterm_buffersize - 1)
                break;
            kscreen_setpos(kterm_pos);
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
            kscreen_putf("%c", key);
            kterm_buffer[kterm_bufferindex] = key;
            kterm_bufferindex++;
            break;
    }

    kterm_pos = kscreen_getpos();
}

void kterm_input(kkeyboard_state *k)
{
    kterm_next = k;
    kterm_changed = 1;
}

void kterm_loop()
{
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

void kterm_init()
{
    kterm_header();
    kkeyboard_setinput(*kterm_input);
    kscreen_putf("\nWelcome to ConcatenOS!");
    kscreen_putf("\nTo get started, run 'help' for a list of commands.");
    kscreen_putf("\n%s", kterm_prompt);
    kterm_pos = kscreen_getpos();
    kscreen_copy();
}