#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <multiboot.h>
#include <serial.h>
#include <desc.h>
#include <screen.h>
#include <debug.h>
#include <acpi.h>
#include <kernel.h>
#include <kbd.h>
#include <mouse.h>

void khalt(void);

void kmultiboot(void *mboot_ptr)
{
    struct multiboot_tag *mboot_info;
    struct multiboot_mmap_tag *mmap = 0;
    struct multiboot_framebuffer_tag *framebuffer = 0;
    struct mutliboot_acpi_tag *acpi = 0;

    kdebug_outf("\r\nkmboot: ptr [0x%x]", (uint64_t)mboot_ptr);

    for (mboot_info = (struct multiboot_tag *) (mboot_ptr + 8);
         mboot_info->type != 0;
         mboot_info = (struct multiboot_tag *) ((uint8_t *) mboot_info + ((mboot_info->size + 7) & ~7)))
    {
        switch (mboot_info->type)
        {
            case 6:
                mmap = (struct multiboot_mmap_tag *) mboot_info;
                break;
            case 8:
                framebuffer = (struct multiboot_framebuffer_tag *) mboot_info;
                break;
            case 14:
                kdebug_outf("\r\nkmboot: acpi v1");
                acpi = (struct mutliboot_acpi_tag *) mboot_info;
                break;
            case 15:
                kdebug_outf("\r\nkmboot: acpi v2");
                acpi = (struct mutliboot_acpi_tag *) mboot_info;
                break;
        }
    }

    kscreen_set(framebuffer);
    kmem_init(mmap);
    kscreen_init();
    kacpi_init(acpi);
}

void kmain(uint64_t mboot_magic, void *mboot_ptr)
{
#ifdef AQUA_DEBUG
    kserial_init();
#endif

    if (mboot_magic != multiboot2_boot_magic)
    {
        kdebug_outf("\r\nkmain: incorrect multiboot magic?");
        khalt();
    }

    kdesc_install();
    asm("sti");

    kmultiboot(mboot_ptr);
    kmem_unpage(0, kernel_space); // finally higher half only mapping
    kscreen_clr(default_color);

    kscreen_putf("%m%n[AQUA Kernel (%s)]\r\n[Built %s %s UTC-6]", default_color, 0, AQUA_VER_STRING, __TIME__, __DATE__);

    kkeyboard_init();
    kmouse_init();
    khalt();
}

void khalt(void)
{
    kdebug_outf("\r\nkhalt: halting indefinitely!");
    while(1)
        asm("hlt");
}