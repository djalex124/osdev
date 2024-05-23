#include <stdint.h>
#include <multiboot.h>

void kmain(uint64_t mboot_magic, void* mboot_ptr)
{
    if (mboot_magic != multiboot2_magic)
        return 0; // should hang
    
    for(;;);
}