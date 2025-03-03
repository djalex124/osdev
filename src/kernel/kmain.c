#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <serial.h>
#include <desc.h>
#include <screen.h>
#include <debug.h>
#include <acpi.h>
#include <kernel.h>
#include <kbd.h>
#include <mouse.h>

void khalt(void)
{
    kdebug_outf("\r\nkhalt: halting indefinitely!");
    while(1)
        asm("hlt");
}

kernel_table ktable;

void kmain(kernel_table *table)
{
#ifdef AQUA_DEBUG
    kserial_init();
#endif

    kdesc_install();
    asm("sti");

    kmem_init(table);
    kscreen_init();
    kscreen_clr(default_color);

    kscreen_putf("\r\n%m%n[AQUA Kernel (%s)]\r\n[Built %s %s UTC-6]", default_color, 0, AQUA_VER_STRING, __TIME__, __DATE__);

    kmouse_init();
    kkeyboard_init();

    /*
    
    Things to still add

    - ACPI decoding
        - SMP support (multiple cores/threads)
        - PCI device support
            - USB support
    - File system driver
    - Real display driver
    - Real memory manager (not just paging)
    
    */

    khalt();
}