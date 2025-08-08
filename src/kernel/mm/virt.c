#include <stddef.h>
#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <kernel/crash.h>

#include <mm/mem.h>

uint64_t kmem_bootpt_start;
uint64_t kmem_bootpt_end;

uint64_t* kmem_newpt()
{
    uint64_t *memory = NULL;

    if ((uint64_t)k_infotable.kmem_table)
    {
        memory = kmem_alloc(1);
        memset(memory, 0, 0x1000);
    }
    else
    {
        if ((kmem_bootpt_start + 0x1000) > kmem_bootpt_end)
            kcrash("Ran out of early boot pts!");
        memory = (uint64_t *)kmem_bootpt_start;
        kmem_bootpt_start += 0x1000;
        memset(memory, 0, 0x1000);
    }

#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\nkm_ea: new page table at 0x%x", memory);
#endif

    return memory;
}

volatile uint64_t *ptab4;

void kmem_pageinternal(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags)
{
#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\r\nkm_p: attempt to page > phys=[0x%x - 0x%x] virt=[0x%x - 0x%x]",
        physical, physical + size, address, address + size);
#endif

    size += address & 0xFFF;

    while (size)
    {
        uint64_t *ptab3, *ptab2, *ptab1;
        size_t p4_index = (address >> 39) & 0x1FF;
        size_t p3_index = (address >> 30) & 0x1FF;
        size_t p2_index = (address >> 21) & 0x1FF;
        size_t p1_index = (address >> 12) & 0x1FF;

        if ((!ptab4[p4_index] & 0x1))
            ptab4[p4_index] = ((uint64_t)kmem_newpt()) | 0x3;
        
        ptab3 = (uint64_t *)(ptab4[p4_index] & ~0xFFF);
        if ((!ptab3[p3_index] & 0x1))
            ptab3[p3_index] = ((uint64_t)kmem_newpt()) | 0x3;
        
        ptab2 = (uint64_t *)(ptab3[p3_index] & ~0xFFF);

        if ((!ptab2[p2_index] & 0x1))
            ptab2[p2_index] = ((uint64_t)kmem_newpt()) | 0x3;
        
        ptab1 = (uint64_t *)(ptab2[p2_index] & ~0xFFF);
        ptab1[p1_index] = physical | flags;
        
        physical += 0x1000;
        address += 0x1000;
        size -= 0x1000;
    }

#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\r\nkm_p: done");
#endif
}

void kmem_unpageinternal(uint64_t address, uint64_t size)
{
    size = (size + 0xFFF) & ~0xFFF;

#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\r\nkm_up: attempt to unpage > virt=0x%x length=0x%x",
        address, size);
#endif

    size += address & 0xFFF;
    address &= -0x1000ull;

    while (size)
    {
        uint64_t *ptab3, *ptab2, *ptab1;
        size_t p4_index = (address >> 39) & 0x1FF;
        size_t p3_index = (address >> 30) & 0x1FF;
        size_t p2_index = (address >> 21) & 0x1FF;
        size_t p1_index = (address >> 12) & 0x1FF;

        if (!(ptab4[p4_index] & 0x1))
            break;

        ptab3 = (uint64_t *)(ptab4[p4_index] & ~0xFFF);
        if (!(ptab3[p3_index] & 0x1))
            break;

        ptab2 = (uint64_t *)(ptab3[p3_index] & ~0xFFF);

        if (!(ptab2[p2_index] & 0x1))
            break;

        ptab1 = (uint64_t *)(ptab2[p2_index] & ~0xFFF);
        if (ptab1[p1_index] & 0x1)
            ptab1[p1_index] &= ~1;

        asm volatile("invlpg (%0)" :: "b"(address) : "memory");

        address += 0x1000;
        size -= 0x1000;
    }
}

typedef struct kmem_virtmap_s
{
    uint32_t active : 1;
    uint32_t used : 1;
    uint32_t unused : 30;
    uint64_t start;
    size_t length;
    uint32_t prev;
    uint32_t current;
    uint32_t next;
}__attribute__((packed)) kmem_virtmap;

uint64_t kernel_vmmap_max = 0;
kmem_virtmap *kernel_vmmap_start = 0;

uint32_t kmem_virtmapnew(uint32_t prev, uint32_t next, uint8_t used, uint64_t start, size_t length)
{
    uint32_t entry_count = 0;
    kmem_virtmap *inactive_entry = 0;
    for (uint32_t entry = 0; (entry <= kernel_vmmap_max) && ((uint64_t)inactive_entry == 0); entry++)
    {
        if (kernel_vmmap_start[entry].active == 0)
        {
            inactive_entry = &kernel_vmmap_start[entry];
            entry_count = entry;
        }
    }

    if ((uint64_t)inactive_entry == 0)
    {
        size_t pages = ((kernel_vmmap_max + 1) * sizeof(kmem_virtmap)) / 0x1000;
        kmem_virtmap *newmap = kmem_alloc(pages * 2);
        memcpy(newmap, kernel_vmmap_start, pages * 0x1000);
        kmem_free(kernel_vmmap_start, pages);
        kernel_vmmap_start = newmap;
        kernel_vmmap_max = (pages * 2 * 0x1000 / sizeof(kmem_virtmap)) - 1;
        inactive_entry = &kernel_vmmap_start[entry_count];
    }
    
    inactive_entry->prev = prev;
    inactive_entry->current = entry_count;
    inactive_entry->next = next;
    inactive_entry->used = used;
    inactive_entry->start = start;
    inactive_entry->length = length;
    inactive_entry->active = 1;

    return entry_count;
}

uint32_t kmem_virtmaptrymerge(uint32_t a, uint32_t b)
{
    if (kernel_vmmap_start[a].used == 1 || kernel_vmmap_start[b].used == 1)
        return -1;
    else if (kernel_vmmap_start[b].prev != kernel_vmmap_start[a].current)
        return -1;

    kernel_vmmap_start[a].length += kernel_vmmap_start[b].length;
    kernel_vmmap_start[a].next = kernel_vmmap_start[b].next;
    kernel_vmmap_start[b].active = 0;

    return a;
}

void* kmem_virtmapalloc(uint64_t size)
{
    kmem_virtmap *free_space = 0;

    for (uint32_t entry = 0; (entry < kernel_vmmap_max) && ((uint64_t)free_space == 0); entry++)
    {
        if (kernel_vmmap_start[entry].length >= size && kernel_vmmap_start[entry].used == 0)
            free_space = &kernel_vmmap_start[entry];
    }

    if ((uint64_t)free_space == 0)
        kcrash("Ran out of virtual memory!");
    
    size_t split_length = free_space->length - size;
    free_space->length = size;
    free_space->used = 1;

    uint32_t leftover_index = kmem_virtmapnew(free_space->current, free_space->next, 
        0, free_space->start + size, split_length);
    free_space->next = leftover_index;

    return (void *)free_space->start;
}

void kmem_virtmapfree(uint64_t addr)
{
    kmem_virtmap *addr_space = 0;

    for (uint32_t entry = 0; (entry < kernel_vmmap_max) && ((uint64_t)addr_space == 0); entry++)
    {
        if (kernel_vmmap_start[entry].start == addr)
            addr_space = &kernel_vmmap_start[entry];
    }

    if ((uint64_t)addr_space == 0)
    {
#ifdef AQUA_DEBUG_MEM
        kdebug_outf("\nkm_v: invalid address to free 0x%x", addr);
#endif
        return;
    }

    addr_space->used = 0;
    
    (void)kmem_virtmaptrymerge(addr_space->current, addr_space->next);
    (void)kmem_virtmaptrymerge(addr_space->prev, addr_space->current);
}

void* kmem_page(uint64_t address, uint64_t size, uint16_t flags)
{
    uint64_t offset = address & 0xFFF;

    size = (size + 0xFFF) & ~0xFFF;
    address &= -0x1000ull;

    if (kernel_vmmap_max)
    {
        uint64_t virtual_mapping = (uint64_t)kmem_virtmapalloc(size);
        kmem_pageinternal(address, virtual_mapping, size, flags);

        return (void *)(virtual_mapping + offset);
    }

    kmem_pageinternal(address, address, size, flags);

    return (void *)(address + offset);
}

void kmem_unpage(void *address, uint64_t size)
{
    size = (size + 0xFFF) & ~0xFFF;
    uint64_t addr = (uintptr_t)address;
    addr &= -0x1000ull;

    if (kernel_vmmap_max)
        kmem_virtmapfree(addr);

    kmem_unpageinternal(addr, size);
}

void kmem_virtinit()
{
    kmem_bootpt_end = phys_from_virt(k_boottable.safe_mem);
    kmem_bootpt_start = phys_from_virt(k_boottable.safe_mem - 0xC000);

    ptab4 = kmem_newpt();
    //page tables have to be identity mapped

    kmem_pageinternal(0, 0, k_boottable.safe_mem - kernel_virtual, 0b11);
    kmem_pageinternal(0x8000, 0x8000, 0x1000, 0b11);
    kmem_pageinternal(0, kernel_virtual, k_boottable.safe_mem - kernel_virtual, 0b11);

    asm volatile("mov %0, %%cr3" ::"r"((uintptr_t)ptab4));
}

void kmem_virtbuildmap()
{
    kernel_vmmap_start = kmem_alloc(1);
    kernel_vmmap_max = (0x1000 / sizeof(kmem_virtmap)) - 1;

    kernel_vmmap_start[0].start = kernel_virtual;
    kernel_vmmap_start[0].length = 0xFFFFFFFFFFFFF000 - kernel_virtual;
    kernel_vmmap_start[0].used = 0;
    kernel_vmmap_start[0].active = 1;
    kernel_vmmap_start[0].current = 0;
    
    (void)kmem_virtmapalloc(k_boottable.safe_mem - kernel_virtual);
}