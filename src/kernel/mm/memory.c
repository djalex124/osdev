#include <stdint.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <mm/mem.h>

void kmem_printinfo()
{
    kscreen_putf("\nkmem_table ");
    size_t entries;
    int sections_available = 0;
    for (entries = 0; k_infotable.kmem_table[entries].eom != 1; entries++)
    {
        if (!k_infotable.kmem_table[entries].used)
            sections_available++;
    }
    kscreen_putf("\nkmem_table mapped memory %d MiB", entries/256);
    kscreen_putf("\nkmem_table available memory %d MiB", sections_available/256);
}

uint64_t kmem_heapend = 0;
uint64_t kmem_heap = 0;

void* kmem_kalloc(uint64_t size)
{
    if ((kmem_heap + size) > kmem_heapend)
    {
        kdebug_outf("\r\nkm_ka: out of kernel heap - halting");
        for(;;);
    }
    uint64_t addr = kmem_heap;
    kmem_heap += size;
#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\r\nkm_ka: heap now at [0x%x]", kmem_heap);
#endif
    memset((uintptr_t *)addr, 0, size);
    return (uintptr_t *)addr;
}

void kmem_kfree(uint64_t size)
{
    kmem_heap -= size;
    memset((uintptr_t *)kmem_heap, 0, size);
}

void kmem_init(boot_table *table)
{
    uint64_t safe_pts = (table->safe_mem + 0xFFF) & ~0xFFF;
    kdebug_outf("\nkm_i: early page tables available [0x%x] - [0x%x]", safe_pts, safe_pts + 0xC000);

    memcpy(&k_boottable, (uint64_t*)virt_from_phys((uint64_t)table), sizeof(boot_table));
    k_boottable.safe_mem = safe_pts + 0xC000;

    kdebug_outf("\r\nkm_i: kernel [0x100000] - [0x%x]", k_boottable.safe_mem);

    kmem_virtinit();
    kmem_physinit();

    kmem_virtbuildmap();

    kmem_heap = (uint64_t)kmem_alloc(16);
    kmem_heapend = kmem_heap + 0x10000;
    
    kdebug_outf("\nkm_i: done");
}