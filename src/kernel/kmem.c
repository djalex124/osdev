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

#define AQUA_DEBUG_PAGING 1

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
    kdebug_outf("\r\nkm_p: attempt to page > phys=[0x%x - 0x%x] virt=[0x%x - 0x%x]",
        physical, physical + size, address, address + size);
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
            ptab4[p4_index] = phys_from_virt((uint64_t)kmem_newpt()) | 0x3;
        
        ptab3 = (uint64_t *)(ptab4[p4_index] & 0xFFFFFFFFFFFFF000);
        if ((!ptab3[p3_index] & 0x1))
            ptab3[p3_index] = phys_from_virt((uint64_t)kmem_newpt()) | 0x3;
        
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
                ptab2[p2_index] = phys_from_virt((uint64_t)kmem_newpt()) | 0x3;
            
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

uint64_t kmem_heap;
uint64_t kmem_heapend;

void* kmem_kalloc(uint64_t size)
{
    if ((kmem_heap + size) > kmem_heapend)
    {
        kdebug_outf("\r\nkm_a: out of kernel heap - halting");
        for(;;);
    }
    uint64_t addr = kmem_heap;
    kmem_heap += size;
    kdebug_outf("\r\nkm_ka: heap now at [0x%x]", kmem_heap);
    memset((uintptr_t *)addr, 0, size);
    return (uintptr_t *)addr;
}

typedef struct
{
    uint32_t page;
    uint8_t  used : 1;
    uint8_t  eos : 1;
    uint8_t  eom : 1;
}__attribute__((packed)) kmem_stack;

size_t kmem_lowestfree = 0;
kmem_stack* kmem_table;

void* kmem_alloc(size_t pages)
{
    size_t index;
    size_t connected = 1;
    for (index = kmem_lowestfree; kmem_table[index].eom != 1; index++)
    {
        if (kmem_table[index].used == 1)
        {
            connected = 1;
            continue;
        }

        if (kmem_table[index].eos)
            connected = 1;
        
        if (connected == pages)
        {
            for (size_t i = 0; i < pages; i++)
                kmem_table[index - pages + 1].used = 1;
            kdebug_outf("\r\nkm_f: setting entry[%x]eom[%x]", index - pages + 1, pages);
            void* addr = (void*)(uint64_t)(kmem_table[index - connected + 1].page * 0x1000);
            kmem_page((uint64_t)addr, (uint64_t)addr, pages * 0x1000, 0b11);
            return addr;
        }

        connected++;
    }
    kdebug_outf("\r\nkm_a: could not allocate chunk before EOM! entry[%x]eom[%x]", index, kmem_table[index].eom);
    return (void*)1;
}

void kmem_free(void* addr, size_t pages)
{
    size_t index = 0;
    for (index = 0; kmem_table[index].page != (uint64_t)(addr)/0x1000; index++);
    kdebug_outf("\r\nkm_f: freeing entry[%x]pages[%x]", index, pages);
    for (size_t i = index; i < index + pages; i++)
        kmem_table[i].used = 0;
    memset(addr, 0, pages * 0x1000);
    kmem_unpage((uint64_t)addr, pages * 0x1000);
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

extern boot_table ktable;

void kmem_printinfo()
{
    kscreen_putf("\nkheap range [0x%x - 0x%x]\n - current value: 0x%x", ktable.safe_mem, kmem_heapend, kmem_heap);
    kscreen_putf("\nkpagetables [0x%x - 0x%x]", kmem_newpt_start, kmem_newpt_end);
    kscreen_putf("\nkmem_table ");
    size_t entries;
    int sections = 0;
    int section_size = 0;
    int sections_available = 0;
    for (entries = 0; kmem_table[entries].eom != 1; entries++, section_size++)
    {
        if (kmem_table[entries].eos == 1)
        {
            kscreen_putf("\n - section[%x] size[0x%x]", sections, section_size*0x1000);
            sections++;
            section_size = 0;
        }
        if (!kmem_table[entries].used)
            sections_available++;
    }
    kscreen_putf("\nkmem_table mapped memory %d MiB", entries/256);
    kscreen_putf("\nkmem_table available memory %d MiB", sections_available/256);
}

void kmem_physinit()
{
    //algorithm outline:
    //finalized memory map will be dynamically created using kernel heap
    //go over section of memory
    // - if not clear, go back to beginning and move checker forward
    // - if clear, continue
    //break down each free section into page aligned areas
    //
    efi_memory_descriptor *mmap = ktable.mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = ktable.mmap_enteries * ktable.mmap_size;

    uint64_t pages = 0;
    
    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + ktable.mmap_size))
    {
        if (mmap_entries->type == 7)
        {
            if (mmap_entries->physical_start < 0x100000)
                continue; //ignored, pagetables
            kdebug_outf("\r\nkm_i: open physical mem [0x%x] pages %x", mmap_entries->physical_start, 
                mmap_entries->num_pages);
            pages += mmap_entries->num_pages;
            if (mmap_entries->physical_start == 0x100000)
                kdebug_outf(" (kernel)");
        }
    }

    kdebug_outf("\r\nkm_i: total mem needed %x (pages %x)", pages * sizeof(kmem_stack), pages);
    kmem_table = (kmem_stack *)kmem_kalloc(pages * sizeof(kmem_stack));
    
    uint64_t index = 0;
    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + ktable.mmap_size))
    {
        if (mmap_entries->type == 7)
        {
            if (mmap_entries->physical_start < 0x100000)
                continue; //ignored, pagetables
            kdebug_outf("\r\nkm_i: indexing mem [0x%x] pages %x", mmap_entries->physical_start, 
                mmap_entries->num_pages);
            uint64_t size = mmap_entries->num_pages;
            for (size_t i = index; i < index + size; i++)
            {
                kmem_table[i].page = (uint32_t)(mmap_entries->physical_start/0x1000) + i;
                if (mmap_entries->physical_start >= 0x100000 && mmap_entries->physical_start + i*0x1000 <= kmem_heap - kernel_virtual)
                    kmem_table[i].used = 1;
                else
                    kmem_table[i].used = 0;
                if (i == index + size - 1)
                    kmem_table[i].eos = 1;
                else
                    kmem_table[i].eos = 0;
                if (i == pages - 1)
                    kmem_table[i].eom = 1;
                else
                    kmem_table[i].eom = 0;
            }
            index += size;
        }
    }
}

void kmem_virtinit()
{
    kmem_newpt_start += kernel_virtual;
    kmem_newpt_end += kernel_virtual;

    ptab4 = kmem_newpt();
    //page tables have to be identity mapped

    kmem_page(0, 0, kmem_heapend - kernel_virtual, 0b11);
    kmem_page(0, kernel_virtual, kmem_heapend - kernel_virtual, 0b11);

    asm volatile("mov %0, %%cr3" ::"r"(((uintptr_t)ptab4 - kernel_virtual)));
}

void kmem_init(boot_table *table)
{
    efi_memory_descriptor *mmap = table->mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = table->mmap_enteries * table->mmap_size;

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + table->mmap_size))
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
                    break;
                } // find lowest chunk of mem for kernel paging
                if (mmap_entries->physical_start == 0x100000)
                    kmem_heapend = mmap_entries->physical_start + mmap_entries->num_pages*0x1000 + kernel_virtual;
                break;
        }
    }

    kdebug_outf("\r\nkm_i: kernel pts [0x%x] - [0x%x]", kmem_newpt_start, kmem_newpt_end);
    kdebug_outf("\r\nkm_i: kernel [0x100000] - [0x%x]", (uint64_t)_end - kernel_virtual); //when available, page kernel with global bit

    kmem_heap = table->safe_mem;
    kdebug_outf("\r\nkm_i: kernel heap [0x%x] - [0x%x]", kmem_heap, kmem_heapend);

    kmem_virtinit();

    memcpy(&ktable, (uint64_t*)virt_from_phys((uint64_t)table), sizeof(boot_table));

    kmem_physinit();
    
    kdebug_outf("\r\nkm_i: done");
}