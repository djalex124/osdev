#include <output/screen.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <mm/mem.h>

#include <stdint.h>

typedef struct kmem_heapblock
{
    size_t size;
    uint8_t used;
    struct kmem_heapblock *next;
    struct kmem_heapblock *prev;
}__attribute__((packed)) kmem_heapblock;

kmem_heapblock *heap_start = 0;

//heap alloc
void* kmem_kalloc(uint64_t size)
{
    kmem_heapblock *heap_ptr = heap_start;
    while (heap_ptr != NULL)
    {
        if (heap_ptr->used == 0 && heap_ptr->size >= size + sizeof(kmem_heapblock))
        {
            if (heap_ptr->size > size + sizeof(kmem_heapblock) * 2)
            {
                kmem_heapblock *split_block = (kmem_heapblock *)((uint64_t)heap_ptr + size + sizeof(kmem_heapblock));
                split_block->size = heap_ptr->size - (size + sizeof(kmem_heapblock));
                split_block->used = 0;
                split_block->next = heap_ptr->next;
                split_block->prev = heap_ptr;

                heap_ptr->size = size + sizeof(kmem_heapblock);
                heap_ptr->next = split_block;
            }

            heap_ptr->used = 1;
            return (void *)((uint64_t)heap_ptr + sizeof(kmem_heapblock));
        }

        heap_ptr = heap_ptr->next;
    }

    return NULL;
}

//heap free
void kmem_kfree(void *addr)
{
    kmem_heapblock *free_block = (kmem_heapblock *)((uint64_t)addr - sizeof(kmem_heapblock));
    free_block->used = 0;

    if (free_block->next != NULL && free_block->next->used == 0)
    {
        free_block->size += free_block->next->size;
        free_block->next = free_block->next->next;
    }

    if (free_block->prev != NULL && free_block->prev->used == 0)
    {
        free_block->prev->size += free_block->size;
        free_block->prev->next = free_block->next;
    }
}

void kmem_heapinit()
{
    heap_start = (kmem_heapblock *)kmem_alloc(0x100);
    // 1mb in total size
    
    heap_start->size = 0x100000 - sizeof(kmem_heapblock);
    heap_start->used = 0;
    heap_start->next = 0;
    heap_start->prev = 0;
}

//gets pages in kernel space
void* kmem_alloc(uint64_t pages)
{
    uint64_t *phys = kmem_palloc(pages);
    void *addr = kmem_page((uint64_t)phys, pages * 0x1000, 0b11);
#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\nkm_a: returning %x", addr);
#endif
    return addr;
}

//frees pages in kernel space
void kmem_free(void *addr, uint64_t pages)
{
    void *phys = kmem_getphysical(addr);
    kmem_pfree(phys, pages);
    kmem_unpage(addr, pages * 0x1000);
}

void kmem_printinfo()
{
    uint64_t total_free = 0;
    kmem_heapblock *ptr = heap_start;
    while (ptr != NULL)
    {
        if (ptr->used == 0)
            total_free += ptr->size;
        ptr = ptr->next;
    }
    kscreen_putf("\nkheap stats: 0x%6x/0x100000 free", total_free);

    kmem_printpmminfo();
}

void kmem_init(boot_table *table)
{
    memcpy(&k_boottable, (uint64_t*)virt_from_phys((uint64_t)table), sizeof(boot_table));

    efi_memory_descriptor *mmap = k_boottable.mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = k_boottable.mmap_enteries * k_boottable.mmap_size;

    uint64_t total_pages = 0;
    uint64_t last_end = 0;

    uint64_t bitmap_low = 0, bitmap_hi = 0;

#ifdef AQUA_DEBUG_MEM
    uint64_t first_free_page = (k_boottable.safe_mem + 0xFFF) & ~0xFFF;
    kdebug_outf("\nkm_i: first page after kernel at 0x%x", first_free_page);
    kdebug_outf("\nkm_i: processing memory map");
#endif

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t)mmap_entries + k_boottable.mmap_size))
    {
        uint64_t end_addr = mmap_entries->physical_start + 0x1000 * mmap_entries->num_pages;

        //kdebug_outf("\nkm_i: mmap type %2d range 0x%8x-0x%8x", mmap_entries->type,
        //    mmap_entries->physical_start, end_addr);
        total_pages += mmap_entries->num_pages;

        if (mmap_entries->type == 11 || mmap_entries->type == 0) // if mmio or unusable, skip
            continue;

        if (mmap_entries->physical_start < 0x100000000 && (mmap_entries->physical_start == last_end ||
            mmap_entries->physical_start == 0x100000))
            bitmap_low = end_addr / 0x1000;
        else if (mmap_entries->physical_start == 0x100000000)
            bitmap_hi = end_addr / 0x1000;

        last_end = end_addr;
    }

#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\nkm_i: bitmaps will address low:0x0-0x%x", bitmap_low * 0x1000);
    if (bitmap_hi)
        kdebug_outf(" high:0x%x-0x%x", 0x100000000, bitmap_hi);

    kdebug_outf("\nkm_i: bitmaps size low:0x%x", bitmap_low / 8);
    if (bitmap_hi)
        kdebug_outf(" high:0x%x", bitmap_hi / 8);
#endif

    kmem_pmminit(bitmap_low, bitmap_hi);
    kmem_vmminit(bitmap_low, bitmap_hi);

    kmem_heapinit();
}