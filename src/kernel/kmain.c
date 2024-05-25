#include <stdint.h>
#include <mem.h>
#include <multiboot.h>

void khalt(void);

static uint8_t kerror = 0;

void kmultiboot(void *mboot_ptr)
{
    struct multiboot_tag *mboot_info;
    struct multiboot_framebuffer_tag *framebuffer;

    for (mboot_info = (struct multiboot_tag *) (mboot_ptr + 8);
         mboot_info->type != 0;
         mboot_info = (struct multiboot_tag *) ((uint8_t *) mboot_info + ((mboot_info->size + 7) & ~7)))
    {
        switch (mboot_info->type)
        {
            case 6:
            {
                struct multiboot_mmap_entry *mmap;
                kmem_init(mmap);
            }
            break;
            case 8:
            {
                framebuffer = (struct multiboot_framebuffer_tag *) mboot_info;
            }
            break;
        }
    }

    kmem_page(framebuffer->addr, virt_from_phys(framebuffer->addr), 0b11);
}

void kmain(uint64_t mboot_magic, void *mboot_ptr)
{
    if (mboot_magic != multiboot2_boot_magic)
        kerror++;

    kmultiboot(mboot_ptr);
    kmem_unpage(0);
    
    khalt();
}

void khalt(void)
{
    while(1)
        __asm__("hlt");
}