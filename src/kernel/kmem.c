#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <serial.h>

extern uint64_t _end[];

uint64_t kmem_earlyalloc_start;
uint64_t kmem_earlyalloc_end;

void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

uint64_t* kmem_earlyalloc()
{
    if ((kmem_earlyalloc_start + 0x1000) > kmem_earlyalloc_end)
    {
        kserial_outf("\r\nkm_ea: out of early memory? halting");
        for(;;);
    }
    uint64_t page = kmem_earlyalloc_start;
    kmem_earlyalloc_start += 0x1000;
    kserial_outf("\r\nkm_ea: new page table at 0x%x", page);
    memset((uintptr_t *)page, 0, 0x1000);
    return (uintptr_t *)page;
}

void kmem_page(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags)
{
    /*
        Preferable paging algorithm

        TODO:
        2mb pages if size allows?
        plug into smarter physical memory manager
    */

    size += address & 0xFFF;
    physical &= -0x1000ull;
    address &= -0x1000ull;

    uint64_t *ptab4 = (uint64_t*)0x1000;

    while (size)
    {
        uint64_t *ptab3, *ptab2;
        size_t p4_index = (address >> 39) & 0x1FF;
        size_t p3_index = (address >> 30) & 0x1FF;
        size_t p2_index = (address >> 21) & 0x1FF;

        if (!ptab4[p4_index] & 0x1)
            ptab4[p4_index] = (uint64_t)kmem_earlyalloc() | flags;

        ptab3 = (uint64_t *)(ptab4[p4_index] & 0xFFFFFFFFFFFFF000);
        if (!ptab3[p3_index] & 0x1)
            ptab3[p3_index] = (uint64_t)kmem_earlyalloc() | flags;

        ptab2 = (uint64_t *)(ptab3[p3_index] & 0xFFFFFFFFFFFFF000);

        if (size >= 0x200000) //2mb page?
        {
            ptab2[p2_index] = physical | flags | (1 << 7); // huge bit?

            physical += 0x200000;
            address += 0x200000;
            size -= 0x200000;
        }
        else
        {
            size_t p1_index = (address >> 12) & 0x1FF;
            uint64_t *ptab1;

            if (!ptab2[p2_index] & 0x1)
                ptab2[p2_index] = (uint64_t)kmem_earlyalloc() | flags;

            ptab1 = (uint64_t *)(ptab2[p2_index] & 0xFFFFFFFFFFFFF000);
            ptab1[p1_index] = physical | flags;

            physical += 0x1000;
            address += 0x1000;
            size -= 0x1000;
        }
    }
}

void kmem_unpage(uint64_t address, uint64_t size)
{
    kserial_outf("\r\nkm_up: attempt to unpage > virt=0x%x length=0x%x",
        address, size);

    size += address & 0xFFF;
    address &= -0x1000ull;

    uint64_t *ptab4 = (uint64_t*)0x1000;

    while (size)
    {
        uint64_t *ptab3, *ptab2;
        size_t p4_index = (address >> 39) & 0x1FF;
        size_t p3_index = (address >> 30) & 0x1FF;
        size_t p2_index = (address >> 21) & 0x1FF;

        if (!ptab4[p4_index] & 0x1)
            break;;

        ptab3 = (uint64_t *)(ptab4[p4_index] & 0xFFFFFFFFFFFFF000);
        if (!ptab3[p3_index] & 0x1)
            break;;

        ptab2 = (uint64_t *)(ptab3[p3_index] & 0xFFFFFFFFFFFFF000);

        if (size >= 0x200000) //2mb page?
        {
            kserial_outf("\r\nkm_up: unpage large page of 0x%x", address);
            if (ptab2[p2_index] & 0x1)
                ptab2[p2_index] &= ~1;

            address += 0x200000;
            size -= 0x200000;
        }
        else
        {
            size_t p1_index = (address >> 12) & 0x1FF;
            uint64_t *ptab1;

            if (!ptab2[p2_index] & 0x1)
                break;

            ptab1 = (uint64_t *)(ptab2[p2_index] & 0xFFFFFFFFFFFFF000);
            if (ptab1[p1_index] & 0x1)
                ptab1[p1_index] &= ~1;

            address += 0x1000;
            size -= 0x1000;
        }
    }
}

//physical memory management?
//currently: UNFINISHED

void* kmem_alloc(uint64_t size)
{
    return 0;
}

void kmem_free(void* addr)
{
    return;
}

void kmem_init(struct multiboot_mmap_tag *mmap)
{
    struct multiboot_mmap_entry *mmap_entries;
    kserial_outf("\r\nkm_i: avail=1 resv=2 acpi_reclaim=3 nvs=4 bad=5");

    for (mmap_entries = mmap->entries;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap->size;
        mmap_entries = (struct multiboot_mmap_entry *) ((uint64_t) mmap_entries + mmap->entry_size))
    {
        kserial_outf("\r\nkm_i: mmap entry > start=0x%x len=0x%x end=0x%x type=%d", 
            mmap_entries->address, mmap_entries->length, mmap_entries->address + mmap_entries->length, mmap_entries->type);
        switch (mmap_entries->type)
        {
            case 1:
                if (mmap_entries->address + mmap_entries->length < 0x100000)
                    kmem_earlyalloc_end = mmap_entries->address + mmap_entries->length;
                break;
        }
    }

    kserial_outf("\r\nkm_i: os reserved 0x0 - 0x4FFF");
    kmem_earlyalloc_start = 0x5000;
    kserial_outf("\r\nkm_i: early kernel mapping 0x%x - 0x%x", kmem_earlyalloc_start, kmem_earlyalloc_end);
    kserial_outf("\r\nkm_i: kernel residing 0x100000 - 0x%x", phys_from_virt((uint64_t)_end)); //when available, page kernel with global bit
}