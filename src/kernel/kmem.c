#include <stdint.h>
#include <mem.h>
#include <serial.h>

extern uint64_t _end[];

void kmem_init(struct multiboot_mmap_tag *mmap)
{
    struct multiboot_mmap_entry *mmap_entries;
    kserial_outs("\r\nkmem_init: available=1 reserved=2 acpi_reclaim=3 nvs=4 bad=5");

    for (mmap_entries = mmap->entries;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap->size;
        mmap_entries = (struct multiboot_mmap_entry *) ((uint64_t) mmap_entries + mmap->entry_size))
    {
        kserial_outs("\r\nkmem_init: mmap entry > start=0x");
        kserial_outn(mmap_entries->address, 16);
        kserial_outs(" len=0x");
        kserial_outn(mmap_entries->length, 16);
        kserial_outs(" end=0x");
        kserial_outn(mmap_entries->address + mmap_entries->length, 16);
        kserial_outs(" type=");
        kserial_outn(mmap_entries->type, 10);
    }

    kserial_outs("\r\nkmem_init: kernel residing in 0x100000 - 0x");
    kserial_outn(phys_from_virt((uint64_t)_end), 16);
}

void kmem_page(uint64_t physical, uint64_t address, uint16_t flags)
{

}

void kmem_unpage(uint64_t address)
{

}