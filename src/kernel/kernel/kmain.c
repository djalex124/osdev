#define kernel_file

#include <stdint.h>
#include <stddef.h>

#include <output/serial.h>
#include <output/screen.h>
#include <output/kterm.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/desc.h>
#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <kernel/kernel.h>
#include <kernel/crash.h>
#include <kernel/debug.h>

#include <ps2/mouse.h>
#include <ps2/kbd.h>

#include <mm/mem.h>

#include <fs/fs.h>

boot_table k_boottable;
info_table k_infotable;

void kmain(boot_table *table)
{
#ifdef AQUA_DEBUG
    kserial_init();
#endif

    kdesc_install();
    kpit_init(1000); //sets pit to ~1ms per interrupt

    kmem_init(table);
#ifdef AQUA_DEBUG
    kcrash_initsym();
#endif

    kscreen_init();
    kscreen_clr(default_color);

    asm("sti");

    kacpi_init();
    kpci_init();

    kfs_init();

    kterm_init();

    kmouse_init();
    kkeyboard_init();

    kterm_loop();

    /*
    
    Things to still add

    - ACPI decoding
        - Simple power management
        - PCI device support
            - USB support
    - Switch to APIC for interrupts/timers
        - Get current time from RTC
    - Scheduling
        - Mutexes, Semaphores, etc
        - Threads
        - Processes
    - ELF Support
    - Syscalls
        - Memory management
        - Screen Printing (until real desktop)
    
    */

    //Should be unreachable...
    kcrash("Reached end of kernel!");
}