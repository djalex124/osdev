#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <multiboot.h>
#include <serial.h>

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
        kserial_outs("\r\nkmultiboot: mboot tag found > ");
        kserial_outn(mboot_info->type, 10);
        switch (mboot_info->type)
        {
            case 2:
            {
                kserial_outs("\r\nkmultiboot: booted through > ");
                kserial_outs(((struct multiboot_string_tag *)mboot_info)->string);
            }
            break;
            case 6:
            {
                kserial_outs("\r\nkmultiboot: mmap found > 0x");
                kserial_outn((uintptr_t)mboot_info, 16);
                mmap = (struct multiboot_mmap_tag *) mboot_info;
            }
            break;
            case 8:
            {
                kserial_outs("\r\nkmultiboot: framebuffer info found > 0x");
                kserial_outn((uintptr_t)mboot_info, 16);
                framebuffer = (struct multiboot_framebuffer_tag *) mboot_info;
                kserial_outs("\r\nkmultiboot: framebuffer buffer at > 0x");
                kserial_outn(framebuffer->addr, 16);
            }
            break;
        }
    }

    kmem_init(mmap);
    kmem_page(framebuffer->addr, virt_from_phys(framebuffer->addr), framebuffer->width * framebuffer->height * framebuffer->bpp, 0b11); // global bit?
}

void kmain(uint64_t mboot_magic, void *mboot_ptr)
{
    kserial_init();

    kserial_outs("ConcatenOS Alpha Dev > Built on ");
    kserial_outs(__DATE__);
    kserial_outs(" at ");
    kserial_outs(__TIME__);

    if (mboot_magic != multiboot2_boot_magic)
    {
        kserial_outs("\r\nkmain: incorrect multiboot magic?");
        khalt();
    }

    kmultiboot(mboot_ptr);
    kmem_unpage(0);
    
    kserial_outs("\r\nkmain: reached end of kernel logic");
    khalt();
}

void khalt(void)
{
    kserial_outs("\r\nkhalt: halting indefinitely!");
    while(1)
        __asm__("hlt");
}