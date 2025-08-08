#include <kernel/kstring.h>
#include <kernel/debug.h>
#include <kernel/crash.h>

#include <mm/mem.h>

size_t kmem_lowestfree = 0;
kmem_bitmap* kmem_table;

void* kmem_alloc(size_t pages)
{
    size_t index;
    size_t connected = 0;

    for (index = kmem_lowestfree; kmem_table[index].eom != 1; index++)
    {
        if (kmem_table[index].used == 1)
        {
            connected = 0;
            continue;
        }
        
        if (connected == pages)
        {
#ifdef AQUA_DEBUG_MEM
            kdebug_outf("\r\nkm_a: setting entry[%x] pages[%x]", index - pages, pages);
#endif
            for (size_t i = 0; i < pages; i++)
                kmem_table[index - pages + i].used = 1;
            
            uint64_t phys_addr = (kmem_table[index - pages].page * 0x1000);
            kmem_pageinternal(phys_addr, phys_addr, pages * 0x1000, 0b11);

            if (kmem_lowestfree < index - pages)
                kmem_lowestfree = index - pages;

            return (void *)phys_addr;
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
    kmem_unpageinternal((uintptr_t)addr, pages * 0x1000);
    if (index + pages - 1 < kmem_lowestfree)
        kmem_lowestfree = index + pages - 1;
}

void kmem_physinit()
{
    efi_memory_descriptor *mmap = k_boottable.mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = k_boottable.mmap_enteries * k_boottable.mmap_size;

    uint64_t total_pages = 0;

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + k_boottable.mmap_size))
    {
        switch (mmap_entries->type)
        {
            case 1 ... 4:
            case 7:
                total_pages += mmap_entries->num_pages;
#ifdef AQUA_DEBUG_MEM
                kdebug_outf("\nkm_i: aqua available mem [0x%x] type %d pages 0x%x", mmap_entries->physical_start, 
                    mmap_entries->type, mmap_entries->num_pages);
                if (mmap_entries->physical_start == 0x100000)
                    kdebug_outf(" (kernel ends 0x%x)", ((phys_from_virt(k_boottable.safe_mem) + 0xFFF) & ~0xFFF) - 1);
                else if (mmap_entries->physical_start < 0x8000 && (mmap_entries->physical_start + mmap_entries->num_pages * 0x1000) > 0x8000)
                    kdebug_outf(" (ap startup 0x8000 - 0x8FFF)");
#endif
                break;
        }
    }

    kdebug_outf("\nkm_i: total pages 0x%x", total_pages);
    kdebug_outf("\nkm_i: bitmap size 0x%x", total_pages * sizeof(kmem_bitmap));

    uint64_t bitmap_start = 0;

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + k_boottable.mmap_size))
    {
        if (mmap_entries->physical_start < 0x100000)
            continue;
        
        switch (mmap_entries->type)
        {
            case 1 ... 4:
            case 7:
                uint64_t potential_start = mmap_entries->physical_start;
                if (mmap_entries->physical_start == 0x100000)
                    potential_start = (phys_from_virt(k_boottable.safe_mem) + 0xFFF) & ~0xFFF;

                uint64_t entry_end = mmap_entries->num_pages * 0x1000 + mmap_entries->physical_start;

                if (entry_end - potential_start > total_pages * sizeof(kmem_bitmap))
                {
                    kdebug_outf("\nkm_i: can start bitmap at 0x%x", potential_start);
                    bitmap_start = potential_start;
                }
                break;
        }

        if (bitmap_start != 0)
            break;
    }

    if (bitmap_start == 0)
        kcrash("No room for mmap!");

    kmem_table = (kmem_bitmap *)bitmap_start;
    kmem_page(bitmap_start, total_pages * sizeof(kmem_bitmap), 0b11);

    uint64_t index = 0;
    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t) mmap_entries + k_boottable.mmap_size))
    {
        switch (mmap_entries->type)
        {
            case 1 ... 4:
            case 7:
                uint64_t size = mmap_entries->num_pages;
                for (size_t i = index; i < index + size; i++)
                {
                    kmem_table[i].page = (uint32_t)(mmap_entries->physical_start/0x1000) + i - index;

                    if (i == total_pages - 1)
                        kmem_table[i].eom = 1;
                    else
                        kmem_table[i].eom = 0;

                    uint64_t current_addr = mmap_entries->physical_start + (i - index) * 0x1000;
                    
                    if (current_addr < 0x10000)
                        kmem_table[i].used = 1;
                    else if (current_addr >= bitmap_start &&
                            current_addr <= bitmap_start + total_pages * sizeof(kmem_bitmap))
                        kmem_table[i].used = 1;
                    else if (current_addr >= 0x100000 &&
                            current_addr <= ((phys_from_virt(k_boottable.safe_mem) + 0xFFF) & ~0xFFF))
                        kmem_table[i].used = 1;
                    else
                        kmem_table[i].used = 0;
                }

                index += size;
                break;
        }
    }

    kdebug_outf("\nkm_i: bitmap finished");

    k_infotable.kmem_table = kmem_table;
}