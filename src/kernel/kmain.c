#define kernel_file

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
#include <kterm.h>
#include <pci.h>
#include <fs.h>

void khalt(void)
{
    kdebug_outf("\r\nkhalt: halting indefinitely!");
    while(1)
        asm("hlt");
}

boot_table k_boottable;
info_table k_infotable;

void kmain(boot_table *table)
{
#ifdef AQUA_DEBUG
    kserial_init();
#endif

    kdesc_install();
    asm("sti");

    kmem_init(table);

    kacpi_init();
    kpci_init();

    kfs_init();

    kscreen_init();
    kscreen_clr(default_color);

    kterm_init();

    kmouse_init();
    kkeyboard_init();

    /*
    
    Things to still add

    - ACPI decoding
        - SMP support (multiple cores/threads)
        - PCI device support
            - USB support
    - File system driver
    
    */

    khalt();
}