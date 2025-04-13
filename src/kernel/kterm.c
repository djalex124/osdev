#include <kstring.h>
#include <screen.h>
#include <kbd.h>
#include <mem.h>
#include <pci.h>
#include <limits.h>
#include <cpuid.h>
#include <kernel.h>

#define kterm_buffersize 100
#define kterm_maxargs 16

#define kterm_titletext "[AQUA Kernel (" AQUA_VER_STRING ")] - [Built " __TIMESTAMP__ " Central Time]"

static kscreen_pos kterm_pos;
static char kterm_buffer[kterm_buffersize];
static uint8_t kterm_bufferindex = 0;

static char *kterm_argv[kterm_maxargs];
static unsigned int kterm_argc;

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
        kterm_pos.x = 0;
        kterm_pos.y = 0;
        kscreen_setpos(kterm_pos);
        kscreen_putf("%m%n%s%m", 0x8A8B8E, 0, kterm_titletext, default_color);
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
    else if (str_cmp(kterm_argv[0], "cpuinfo") == 0)
    {
        unsigned int unused, bx, cx, dx;
        __cpuid(0, unused, bx, cx, dx);
        kscreen_putf("\n - Brand [%4s%4s%4s]", &bx, &dx, &cx);
    }
    else if (str_cmp(kterm_argv[0], "help") == 0)
    {
        kscreen_putf("\nList of currently available commands:");
        kscreen_putf("\nclear - clears the screen");
        kscreen_putf("\ncompare [num1] [num2] - compares two numbers and prints out the largest");
        kscreen_putf("\ncpuinfo - lists CPU model and capabilities");
        kscreen_putf("\nhelp - lists available commands");
        kscreen_putf("\nmeminfo - prints current memory usage");
        kscreen_putf("\npciinfo - prints pci busses and devices");
        kscreen_putf("\ntest - test random features");
    }
    else if (str_cmp(kterm_argv[0], "meminfo") == 0)
    {
        kmem_printinfo();
    }
    else if (str_cmp(kterm_argv[0], "pciinfo") == 0)
    {
        kscreen_putf("\nkpci_info: current pci device table");
        kpci_headercommon* kpci_table = k_infotable.kpci_table;
        for (int i = 0; i < k_infotable.kpci_tablesize; i++)
        {
            kscreen_putf("\n - Bus %2x Device %2x Function %2x", kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function);
            kscreen_putf(": Class %2x/%2x VendorID %x DeviceID %x Prog IF %x", 
                kpci_table[i].class, kpci_table[i].subclass, kpci_table[i].vendorid, kpci_table[i].deviceid, kpci_table[i].progif);
        }
    }
    else if (str_cmp(kterm_argv[0], "test") == 0)
    {
        kscreen_putf("\ntest output of the commands!!");
        uint16_t* test = kmem_alloc(1);
        kscreen_putf("\ntest %x", (uint64_t)test);
        test[32] = 0xCA;
        kscreen_putf("\ntest[32] %x", test[32]);
        kmem_free(test, 1);
        kscreen_putf("%m", default_color);
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

void kterm_input(kkeyboard_state k)
{
    char key = kkeyboard_keymapUSqwerty[k.scancode];
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
                if (k.capslk ^ (k.lshift | k.rshift))
                    upper = 1;
            }
            else if (k.lshift | k.rshift)
                upper = 1;
            if (upper)
                key = kkeyboard_keymapUSqwerty_upper[k.scancode];
            kscreen_putf("%c", key);
            kterm_buffer[kterm_bufferindex] = key;
            kterm_bufferindex++;
            break;
    }

    kterm_pos = kscreen_getpos();
    kscreen_copy();
}

void kterm_init()
{
    kscreen_putf("%m%n%s", 0x8A8B8E, 0, kterm_titletext);
    kkeyboard_setinput(*kterm_input);
    kscreen_putf("%m\n%s", default_color, kterm_prompt);
    kterm_pos = kscreen_getpos();
    kscreen_copy();
}