#include <stdint.h>
#include <kstring.h>
#include <kernel.h>
#include <screen.h>
#include <debug.h>
#include <mem.h>

uint64_t kmem_heap;
uint64_t kmem_heapend;

extern uint64_t kmem_newpt_start;
extern uint64_t kmem_newpt_end;

extern uint64_t _end[];

void kmem_printinfo()
{
    kscreen_putf("\nkheap range [0x%x - 0x%x]\n - current value: 0x%x", k_boottable.safe_mem, kmem_heapend, kmem_heap);
    kscreen_putf("\nkpagetables [0x%x - 0x%x]", kmem_newpt_start, kmem_newpt_end);
    kscreen_putf("\nkmem_table ");
    size_t entries;
    int sections = 0;
    int section_size = 0;
    int sections_available = 0;
    for (entries = 0; k_infotable.kmem_table[entries].eom != 1; entries++, section_size++)
    {
        if (k_infotable.kmem_table[entries].eos == 1)
        {
            kscreen_putf("\n - section[%x] size[0x%x]", sections, section_size*0x1000);
            sections++;
            section_size = 0;
        }
        if (!k_infotable.kmem_table[entries].used)
            sections_available++;
    }
    kscreen_putf("\nkmem_table mapped memory %d MiB", entries/256);
    kscreen_putf("\nkmem_table available memory %d MiB", sections_available/256);
}

#ifdef AQUA_DEBUG_MEM
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
    efi_memory_descriptor *mmap = table->mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = table->mmap_enteries * table->mmap_size;

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + table->mmap_size))
    {
#ifdef AQUA_DEBUG_MEM
        kdebug_outf("\r\nkm_i: mmap [0x%x] - [0x%x]", 
            mmap_entries->physical_start, mmap_entries->physical_start + mmap_entries->num_pages*0x1000);
        if (mmap_entries->type < 18)
            kdebug_outf(" %s", kmem_type[mmap_entries->type]);
        else
            kdebug_outf(" %d", mmap_entries->type);
#endif
        switch (mmap_entries->type)
        {
            case 7:
                if (mmap_entries->physical_start + mmap_entries->num_pages*0x1000 < 0x100000)
                {    
                    kmem_newpt_start = mmap_entries->physical_start;
                    kmem_newpt_end = (kmem_newpt_start + mmap_entries->num_pages*0x1000);
                } // find lowest chunk of mem for kernel paging
                else if (mmap_entries->physical_start == 0x100000)
                    kmem_heapend = mmap_entries->physical_start + mmap_entries->num_pages*0x1000 + kernel_virtual;
                break;
        }
    }

    kdebug_outf("\r\nkm_i: kernel pts [0x%x] - [0x%x]", kmem_newpt_start, kmem_newpt_end);
    kdebug_outf("\r\nkm_i: kernel [0x100000] - [0x%x]", (uint64_t)_end - kernel_virtual); //when available, page kernel with global bit

    kmem_heap = table->safe_mem;
    kdebug_outf("\r\nkm_i: kernel heap [0x%x] - [0x%x]", kmem_heap, kmem_heapend);

    kmem_virtinit();

    memcpy(&k_boottable, (uint64_t*)virt_from_phys((uint64_t)table), sizeof(boot_table));

    kmem_physinit();
    
    kdebug_outf("\r\nkm_i: done");
}