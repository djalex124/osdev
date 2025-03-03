#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <kstring.h>
#include <debug.h>
#include <kernel.h>
#include <screen.h>

extern uint64_t _end[];

static uint64_t kmem_newpt_start;
static uint64_t kmem_newpt_end;

#define AQUA_DEBUG_PAGING

uint64_t* kmem_newpt()
{
    if ((kmem_newpt_start + 0x1000) > kmem_newpt_end)
    {
        kdebug_outf("\r\nkm_ea: out of early memory? halting");
        for(;;);
    }
    uint64_t page = kmem_newpt_start;
    kmem_newpt_start += 0x1000;
#ifdef AQUA_DEBUG_PAGING
    kdebug_outf("\r\nkm_ea: new page table at 0x%x", page);
#endif
    memset((uintptr_t *)page, 0, 0x1000);
    return (uintptr_t *)page;
}

static uint64_t *ptab4;

void kmem_page(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags)
{
    size = (size + 0xFFF) & ~0xFFF;
    
    /*
        Preferable paging algorithm

        TODO:
        plug into smarter physical memory manager
    */

#ifdef AQUA_DEBUG_PAGING
    kdebug_outf("\r\nkm_p: attempt to page > phys=0x%x virt=0x%x length=0x%x",
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
            ptab4[p4_index] = phys_from_virt((uint64_t)kmem_newpt()) | flags;

        ptab3 = (uint64_t *)(ptab4[p4_index] & 0xFFFFFFFFFFFFF000);
        if ((!ptab3[p3_index] & 0x1))
            ptab3[p3_index] = phys_from_virt((uint64_t)kmem_newpt()) | flags;

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
                ptab2[p2_index] = phys_from_virt((uint64_t)kmem_newpt()) | flags;

            ptab1 = (uint64_t *)(ptab2[p2_index] & 0xFFFFFFFFFFFFF000);
            ptab1[p1_index] = physical | flags;

            physical += 0x1000;
            address += 0x1000;
            size -= 0x1000;
        }
    }

#ifdef AQUA_DEBUG_PAGING
    kdebug_outf("\r\nkm_p: done");
#endif
}

void kmem_unpage(uint64_t address, uint64_t size)
{
    size = (size + 0xFFF) & ~0xFFF;

#ifdef AQUA_DEBUG_PAGING
    kdebug_outf("\r\nkm_up: attempt to unpage > virt=0x%x length=0x%x",
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
#ifdef AQUA_DEBUG_PAGING
            kdebug_outf("\r\nkm_up: unpage large page of 0x%x", address);
#endif
            if (ptab2[p2_index] & 0x1)
                ptab2[p2_index] &= ~1;

            asm volatile("invlpg (%0)" :: "b"(address) : "memory");

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

            asm volatile("invlpg (%0)" :: "b"(address) : "memory");

            address += 0x1000;
            size -= 0x1000;
        }
    }
}

//physical memory management?
//currently: UNFINISHED

uint64_t kmem_heap;
uint64_t kmem_heapend;

void* kmem_alloc(uint64_t size)
{
    if ((kmem_heap + size) > kmem_heapend)
    {
        kdebug_outf("\r\nkm_a: out of kernel heap - halting");
        for(;;);
    }
    uint64_t addr = kmem_heap;
    kmem_heap += size;
    kdebug_outf("\r\nkm_a: heap now at [0x%x]", kmem_heap);
    memset((uintptr_t *)addr, 0, size);
    return (uintptr_t *)addr;
}

void kmem_free(void* addr)
{
    return; //no good implementation until real mm
}

#ifdef AQUA_DEBUG
static char* kmem_type[17] = {
    "reserved",
    "loadercode",
    "loaderdata",
    "bootservicecode",
    "bootservicedata",
    "runtimeservicecode",
    "runtimeservicedata",
    "freememory",
    "unusablememory",
    "acpireclaim",
    "acpimemorynvs",
    "mmio",
    "mmioportspace",
    "palcode",
    "persistentmemory",
    "unaccepted",
    "maxmemory",
};
#endif

extern kernel_table ktable;

void kmem_init(kernel_table *table)
{
    memory_descriptor *mmap = table->mmap;
    memory_descriptor *mmap_entries;
    uint64_t mmap_length = table->mmap_enteries * table->mmap_size;

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (memory_descriptor *) ((uint64_t) mmap_entries + table->mmap_size))
    {
        kdebug_outf("\r\nkm_i: mmap [0x%x] - [0x%x]", 
            mmap_entries->physical_start, mmap_entries->physical_start + mmap_entries->num_pages*0x1000);
        if (mmap_entries->type < 18)
            kdebug_outf(" %s", kmem_type[mmap_entries->type]);
        else
            kdebug_outf(" %d", mmap_entries->type);
        switch (mmap_entries->type)
        {
            case 7:
                if (mmap_entries->physical_start + mmap_entries->num_pages*0x1000 < 0x100000)
                {    
                    kmem_newpt_start = mmap_entries->physical_start;
                    kmem_newpt_end = (kmem_newpt_start + mmap_entries->num_pages*0x1000);
                } // find lowest chunk of mem for early kernel paging
                break;
        }
    }

    kdebug_outf("\r\nkm_i: kernel pts [0x%x] - [0x%x]", kmem_newpt_start, kmem_newpt_end);
    kdebug_outf("\r\nkm_i: kernel [0x100000] - [0x%x]", (uint64_t)_end - kernel_virtual); //when available, page kernel with global bit

    kmem_newpt_start += kernel_virtual;
    kmem_newpt_end += kernel_virtual;

    ptab4 = kmem_newpt();
    //page tables have to be identity mapped

    kmem_page(0, 0, kernel_space, 0b11);
    kmem_page(0, kernel_virtual, kernel_space, 0b11);

    asm volatile("mov %0, %%cr3" ::"r"(((uintptr_t)ptab4 - kernel_virtual)));

    kmem_heap = table->safe_mem;
    kmem_heapend = kernel_virtual + kernel_space;

    kdebug_outf("\r\nkm_i: kernel heap [0x%x] - [0x%x]", kmem_heap, kmem_heapend);

    memcpy(&ktable, (uint64_t*)virt_from_phys((uint64_t)table), sizeof(kernel_table));
    
    kdebug_outf("\r\nkm_i: done");
}