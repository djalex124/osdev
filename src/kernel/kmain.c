#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <multiboot.h>
#include <serial.h>
#include <desc.h>
#include <screen.h>

void khalt(void);

void kmultiboot(void *mboot_ptr)
{
    struct multiboot_tag *mboot_info;
    struct multiboot_mmap_tag *mmap = 0;
    struct multiboot_framebuffer_tag *framebuffer = 0;

    for (mboot_info = (struct multiboot_tag *) (mboot_ptr + 8);
         mboot_info->type != 0;
         mboot_info = (struct multiboot_tag *) ((uint8_t *) mboot_info + ((mboot_info->size + 7) & ~7)))
    {
        switch (mboot_info->type)
        {
            case 2:
            {
                kserial_outf("\r\nkmboot: booted through > %s", ((struct multiboot_string_tag *)mboot_info)->string);
            }
            break;
            case 6:
            {
                kserial_outf("\r\nkmboot: mmap found > 0x%x", (uintptr_t)mboot_info);
                mmap = (struct multiboot_mmap_tag *) mboot_info;
            }
            break;
            case 8:
            {
                kserial_outf("\r\nkmboot: fb info found > 0x%x", (uintptr_t)mboot_info);
                framebuffer = (struct multiboot_framebuffer_tag *) mboot_info;
            }
            break;
        }
    }

    kscreen_set(framebuffer);
    kmem_init(mmap);
    kscreen_init();
}

void kmain(uint64_t mboot_magic, void *mboot_ptr)
{
    kserial_init();
    kserial_outf("\r\nConcatenOS Alpha Dev > Built on %s at %s", __DATE__, __TIME__);

    if (mboot_magic != multiboot2_boot_magic)
    {
        kserial_outf("\r\nkmain: incorrect multiboot magic?");
        khalt();
    }

    kdesc_install();
    asm("sti");

    kmultiboot(mboot_ptr);
    kscreen_clr(default_color);

    kserial_outf("\r\nkmain: reached end of kernel logic");
    khalt();
}

void khalt(void)
{
    kserial_outf("\r\nkhalt: halting indefinitely!");
    while(1)
        asm("hlt");
}