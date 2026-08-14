#include <output/kterm.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <kernel/crash.h>

#include <sched/sync.h>

#include <mm/mem.h>

#include <stdint.h>

typedef struct kmem_heapblock
{
    size_t size;
    uint8_t used;
    struct kmem_heapblock *next;
    struct kmem_heapblock *prev;
}kmem_heapblock;

#define kmem_heapsize 0x80
kmem_heapblock *heap_start = 0;

atomic_flag kmem_heap_lock;

/*
void kmem_heap_print()
{
    kmem_heapblock *heap_ptr = heap_start;
    while (heap_ptr != NULL)
    {
        uint64_t heap_round = (uint64_t)heap_ptr & 0xFFFF;
        kdebug_outf("[%3x](%d)", heap_round, heap_ptr->used);
        kdebug_outf("-[%3x-%3x]", heap_round + sizeof(kmem_heapblock), heap_round + sizeof(kmem_heapblock) + heap_ptr->size);
        heap_ptr = heap_ptr->next;
    }
}
*/

void kmem_simplify(kmem_heapblock *block)
{
    kmem_heapblock* free_block = block;
    if (block->used == 1)
        return;

    if (free_block->next != NULL && free_block->next->used == 0)
    {
        free_block->size += free_block->next->size + sizeof(kmem_heapblock);
        free_block->next = free_block->next->next;
        if (free_block->next != NULL)
            free_block->next->prev = free_block;
    }

    if (free_block->prev != NULL && free_block->prev->used == 0)
    {
        free_block = free_block->prev;
        free_block->size += free_block->next->size + sizeof(kmem_heapblock);
        free_block->next = free_block->next->next;
        if (free_block->next != NULL)
            free_block->next->prev = free_block;
    }
}

//heap alloc
void* kmem_kalloc(uint64_t size)
{
    ksync_mutex_acq(&kmem_heap_lock);

    kmem_heapblock *heap_ptr = heap_start;
    while ((uint64_t)heap_ptr + size < (uint64_t)heap_ptr + 0x1000 * kmem_heapsize)
    {
        if (heap_ptr->used == 0 && heap_ptr->size >= size)
        {
            if (heap_ptr->size >= size + sizeof(kmem_heapblock))
            {
                kmem_heapblock *split_block = (kmem_heapblock *)((uint64_t)heap_ptr + size + sizeof(kmem_heapblock));
                split_block->size = heap_ptr->size - (size + sizeof(kmem_heapblock));
                split_block->used = 0;
                split_block->next = heap_ptr->next;
                split_block->prev = heap_ptr;

                if (split_block->next != NULL)
                    split_block->next->prev = split_block;

                heap_ptr->size = size;
                heap_ptr->next = split_block;
            }
            else if (heap_ptr->size > size)
                size = heap_ptr->size;

            if (heap_ptr->next != NULL)
                heap_ptr->next->prev = heap_ptr;
            heap_ptr->used = 1;
            
            void *return_addr = (void *)((uint64_t)heap_ptr + sizeof(kmem_heapblock));
            memset(return_addr, 0, size);

            ksync_mutex_rel(&kmem_heap_lock);

            return return_addr;
        }

        heap_ptr = heap_ptr->next;
    }

    kcrash("Heap OOM");

    ksync_mutex_rel(&kmem_heap_lock);
    return NULL;
}

//heap free
void kmem_kfree(void *addr)
{
    ksync_mutex_acq(&kmem_heap_lock);

    kmem_heapblock *free_block = (kmem_heapblock *)((uint64_t)addr - sizeof(kmem_heapblock));
    if (free_block->used == 0)
        kcrash("Heap freeing unused memory");
    else
    {
        free_block->used = 0;
        kmem_simplify(free_block);
    }

    ksync_mutex_rel(&kmem_heap_lock);
}

void kmem_heapinit()
{
    heap_start = (kmem_heapblock *)kmem_alloc(kmem_heapsize);
    // 512kb in total size

    heap_start->size = (kmem_heapsize * 0x1000) - sizeof(kmem_heapblock);
    heap_start->used = 0;
    heap_start->next = 0;
    heap_start->prev = 0;
}

//gets pages in kernel space
void* kmem_alloc(size_t pages)
{
    uint8_t align = (pages >= 1024) ? kmem_paging_2mb : kmem_paging_1kb;
    uint64_t *phys = kmem_palloc(pages, align);

    void *addr = kmem_page((uint64_t)phys, pages * 0x1000, kmem_paging_present | kmem_paging_writable, kmem_paging_1kb);
#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\nkm_a: returning %x - %x", addr, addr + pages * 0x1000);
#endif

    memset(addr, 0, pages * 0x1000);

    return addr;
}

//frees pages in kernel space
void kmem_free(void *addr, size_t pages)
{
    void *phys = kmem_getphysical(k_ptab4, addr);
#ifdef AQUA_DEBUG_MEM
    kdebug_outf("\nkm_f: freeing %x", phys);
#endif
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
    kterm_putf("\nkheap stats: 0x%x/0x%x free", total_free, (kmem_heapsize * 0x1000));

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

#ifdef AQUA_DEBUG_MEM
        kdebug_outf("\nkm_i: mmap type %2d range 0x%8x-0x%8x", mmap_entries->type,
            mmap_entries->physical_start, end_addr);
#endif

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
        kdebug_outf(" high:0x%x-0x%x", 0x100000000, bitmap_hi * 0x1000);

    kdebug_outf("\nkm_i: bitmaps size low:0x%x", bitmap_low / 8);
    if (bitmap_hi)
        kdebug_outf(" high:0x%x", bitmap_hi / 8);
#endif

    kmem_pmminit(bitmap_low, bitmap_hi);
    kmem_vmminit(bitmap_low, bitmap_hi);

    kmem_heapinit();
}