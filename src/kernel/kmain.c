#include <stdint.h>
#include <multiboot.h>

void khalt(void);

void kmain(uint64_t mboot_magic, void* mboot_ptr)
{
    if (mboot_magic != multiboot2_boot_magic)
        return; // should hang

    struct multiboot_tag* mboot_info;
    uint32_t size = *(uint64_t *) mboot_ptr;

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
            }
            break;
            case 8:
            {
                framebuffer = (struct multiboot_framebuffer_tag *) mboot_info;
            }
            break;
        }
    }
    
    khalt();
}

void khalt(void)
{
    while(1)
        __asm__("hlt");
}