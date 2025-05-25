#include <kstring.h>
#include <debug.h>
#include <mem.h>

size_t kmem_lowestfree = 0;
kmem_stack* kmem_table;

void* kmem_alloc(size_t pages)
{
    size_t index;
    size_t connected = 0;
    for (index = kmem_lowestfree + 1; kmem_table[index].eom != 1; index++)
    {
        if (kmem_table[index].used == 1)
        {
            connected = 0;
            continue;
        }

        if (kmem_table[index].eos)
            connected = 0;
        
        if (connected == pages - 1)
        {
#ifdef AQUA_DEBUG_MEM
            kdebug_outf("\r\nkm_a: setting entry[%x] pages[%x]", index - pages, pages);
#endif
            for (size_t i = 0; i < pages; i++)
                kmem_table[index - pages + i].used = 1;
            
            void* addr = (void*)(uint64_t)(kmem_table[index - pages].page * 0x1000);
            kmem_page((uint64_t)addr, (uint64_t)addr, pages * 0x1000, 0b11);

            if (kmem_lowestfree < index - pages + 1)
                kmem_lowestfree = index - pages + 1;

            return addr;
        }

        connected++;
    }
    kdebug_outf("\r\nkm_a: could not allocate chunk before EOM! entry[%x]pages[%x]eom[%x]", index, pages, kmem_table[index].eom);
    return NULL;
}

void kmem_free(void* addr, size_t pages)
{
    size_t index = 0;
    for (index = 0; kmem_table[index].page != (uint64_t)(addr)/0x1000; index++);
#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\r\nkm_f: freeing entry[%x]pages[%x]", index, pages);
#endif
    for (size_t i = index; i < index + pages; i++)
        kmem_table[i].used = 0;
    memset(addr, 0, pages * 0x1000);
    kmem_unpage((uint64_t)addr, pages * 0x1000);
    if (index + pages < kmem_lowestfree)
        kmem_lowestfree = index + pages - 1;
}

extern uint64_t kmem_heap;

void kmem_physinit()
{
    efi_memory_descriptor *mmap = k_boottable.mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = k_boottable.mmap_enteries * k_boottable.mmap_size;

    uint64_t pages = 0;
    
    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + k_boottable.mmap_size))
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
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + k_boottable.mmap_size))
    {
        if (mmap_entries->type == 7)
        {
            if (mmap_entries->physical_start < 0x100000)
                continue; //ignored, pagetables
            uint64_t size = mmap_entries->num_pages;
            for (size_t i = index; i < index + size; i++)
            {
                kmem_table[i].page = (uint32_t)(mmap_entries->physical_start/0x1000) + i;
                if (mmap_entries->physical_start >= 0x100000 && (mmap_entries->physical_start <= kmem_heap - kernel_virtual))
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
    kdebug_outf("\r\nkm_i: total indexed [%x]", index);

    k_infotable.kmem_table = kmem_table;
}