#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <multiboot.h>
#include <serial.h>
#include <desc.h>

void khalt(void);
void kscreen(uint64_t fb, uint16_t width, uint16_t height, uint8_t bpp, uint32_t pitch);

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

    kmem_init(mmap);
    kscreen(framebuffer->addr, framebuffer->width, framebuffer->height, framebuffer->bpp, framebuffer->pitch);
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

    kmultiboot(mboot_ptr);
    kmem_unpage(0, 0x200000);

    kmem_page(0, kernel_virtual, 0x200000, 0x3);

    kdesc_install();
    asm("sti");
    
    kserial_outf("\r\nkmain: reached end of kernel logic");
    khalt();
}

void kscreen(uint64_t fb, uint16_t width, uint16_t height, uint8_t bpp, uint32_t pitch)
{
    //screen.c coming soon, settle with this for now

    kmem_page(fb, virt_from_phys(fb), width * height * bpp, 0b11);
    kserial_outf("\r\nkscr: addr at 0x%x width %d height %d bpp %d pitch %d", fb, width, height, bpp, pitch);

    uint32_t default_color = 0x34568B;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            *((uint32_t*)(virt_from_phys(fb) + y*pitch + x*(bpp/8)))=default_color;
        }
    }
}

void khalt(void)
{
    kserial_outf("\r\nkhalt: halting indefinitely!");
    while(1)
        asm("hlt");
}