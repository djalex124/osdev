#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <serial.h>
#include <kstring.h>

extern uint64_t _end[];

static uint64_t kmem_earlyalloc_start;
static uint64_t kmem_earlyalloc_end;

//#define kmem_paging_debug

uint64_t* kmem_earlyalloc()
{
    if ((kmem_earlyalloc_start + 0x1000) > kmem_earlyalloc_end)
    {
        kserial_outf("\r\nkm_ea: out of early memory? halting");
        for(;;);
    }
    uint64_t page = kmem_earlyalloc_start;
    kmem_earlyalloc_start += 0x1000;
#ifdef kmem_paging_debug
    kserial_outf("\r\nkm_ea: new page table at 0x%x", page);
#endif
    memset((uintptr_t *)page, 0, 0x1000);
    return (uintptr_t *)page;
}

static uint64_t *ptab4;

void kmem_page(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags)
{
    /*
        Preferable paging algorithm

        TODO:
        plug into smarter physical memory manager
    */

#ifdef kmem_paging_debug
    kserial_outf("\r\nkm_p: attempt to page > phys=0x%x virt=0x%x length=0x%x",
        physical, address, size);
#endif

    size += address & 0xFFF;
    physical &= -0x1000ull;
    address &= -0x1000ull;

    while (size)
    {
        uint64_t *ptab3, *ptab2;
        size_t p4_index = (address >> 39) & 0x1FF;
        size_t p3_index = (address >> 30) & 0x1FF;
        size_t p2_index = (address >> 21) & 0x1FF;

        if ((!ptab4[p4_index] & 0x1))
            ptab4[p4_index] = phys_from_virt((uint64_t)kmem_earlyalloc()) | flags;

        ptab3 = (uint64_t *)(ptab4[p4_index] & 0xFFFFFFFFFFFFF000);
        if ((!ptab3[p3_index] & 0x1))
            ptab3[p3_index] = phys_from_virt((uint64_t)kmem_earlyalloc()) | flags;

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

            if ((!ptab2[p2_index] & 0x1))
                ptab2[p2_index] = phys_from_virt((uint64_t)kmem_earlyalloc()) | flags;

            ptab1 = (uint64_t *)(ptab2[p2_index] & 0xFFFFFFFFFFFFF000);
            ptab1[p1_index] = physical | flags;

            physical += 0x1000;
            address += 0x1000;
            size -= 0x1000;
        }
    }

#ifdef kmem_paging_debug
    kserial_outf("\r\nkm_p: done");
#endif
}

void kmem_unpage(uint64_t address, uint64_t size)
{
#ifdef kmem_paging_debug
    kserial_outf("\r\nkm_up: attempt to unpage > virt=0x%x length=0x%x",
        address, size);
#endif

    size += address & 0xFFF;
    address &= -0x1000ull;

    while (size)
    {
        uint64_t *ptab3, *ptab2;
        size_t p4_index = (address >> 39) & 0x1FF;
        size_t p3_index = (address >> 30) & 0x1FF;
        size_t p2_index = (address >> 21) & 0x1FF;

        if (!(ptab4[p4_index] & 0x1))
            break;;

        ptab3 = (uint64_t *)(ptab4[p4_index] & 0xFFFFFFFFFFFFF000);
        if (!(ptab3[p3_index] & 0x1))
            break;;

        ptab2 = (uint64_t *)(ptab3[p3_index] & 0xFFFFFFFFFFFFF000);

        if (size >= 0x200000) //2mb page?
        {
#ifdef kmem_paging_debug
            kserial_outf("\r\nkm_up: unpage large page of 0x%x", address);
#endif
            if (ptab2[p2_index] & 0x1)
                ptab2[p2_index] &= ~1;

            address += 0x200000;
            size -= 0x200000;
        }
        else
        {
            size_t p1_index = (address >> 12) & 0x1FF;
            uint64_t *ptab1;

            if (!(ptab2[p2_index] & 0x1))
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

extern uintptr_t boot_tab4;

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
                {    
                    kmem_earlyalloc_start = mmap_entries->address;
                    kmem_earlyalloc_end = (kmem_earlyalloc_start + mmap_entries->length);
                } // find lowest chunk of mem for early kernel paging
                break;
        }
    }

	if (kmem_earlyalloc_start == 0) // dont overwrite bios data
		kmem_earlyalloc_start += 0x1000;

#ifdef kmem_paging_debug
    kserial_outf("\r\nkm_i: kernel pts 0x%x - 0x%x", kmem_earlyalloc_start, kmem_earlyalloc_end);
    kserial_outf("\r\nkm_i: kernel residing 0x100000 - 0x%x", phys_from_virt((uint64_t)_end)); //when available, page kernel with global bit
#endif

    kmem_earlyalloc_start+=kernel_virtual;
    kmem_earlyalloc_end+=kernel_virtual;
    
    ptab4 = kmem_earlyalloc();
    kmem_page(0, 0, 0x200000, 0b11);
    kmem_page(0, kernel_virtual, 0x200000, 0b11);

    asm volatile("mov %0, %%cr3" ::"r"(phys_from_virt((uintptr_t)ptab4)));
    
#ifdef kmem_paging_debug
    kserial_outf("\r\n --- kmem_init done ---");
#endif
}