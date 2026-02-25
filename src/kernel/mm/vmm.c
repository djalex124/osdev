#include <kernel/kstring.h>
#include <kernel/debug.h>
#include <kernel/crash.h>

#include <mm/mem.h>

#include <stdint.h>

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

void* kmem_virtmapalloc(uint64_t size, uint8_t align)
{
    kmem_virtmap *free_space = 0;
	uint64_t aligned_start = 0;

    for (uint32_t entry = 0; (entry < kernel_vmmap_max) && ((uint64_t)free_space == 0); entry++)
    {
		if (kernel_vmmap_start[entry].used)
			continue;

		switch (align)
		{
			case kmem_paging_1kb:
				aligned_start = ((kernel_vmmap_start[entry].start + 0xFFF) / 0x1000) * 0x1000;
				break;
			case kmem_paging_2mb:
				aligned_start = ((kernel_vmmap_start[entry].start + 0x1FFFFF) / 0x200000) * 0x200000;
				break;
			case kmem_paging_1gb:
				aligned_start = ((kernel_vmmap_start[entry].start + 0x3FFFFFFF) / 0x40000000) * 0x40000000;
				break;
			default:
				kcrash("Requested paging mode not implemented!");
				break;
		}

		uint64_t aligned_space = kernel_vmmap_start[entry].start + kernel_vmmap_start[entry].length - aligned_start;

		if (aligned_space >= size)
            free_space = &kernel_vmmap_start[entry];
    }

    if ((uint64_t)free_space == 0)
    {
		kcrash("Ran out of virtual memory!");
	}

	size_t front_gap = aligned_start - free_space->start;
	if (front_gap)
	{
		uint32_t front_gap_index = kmem_virtmapnew(free_space->prev, free_space->current, 0,
			free_space->start, front_gap);
		if (free_space->prev != 0)
			kernel_vmmap_start[free_space->prev].next = front_gap_index;
		free_space->prev = front_gap_index;
	}
    
	size_t split_length = free_space->start + free_space->length - (aligned_start + size);
    free_space->start = aligned_start;
    free_space->length = size;
    free_space->used = 1;

    uint32_t leftover_index = kmem_virtmapnew(free_space->current, free_space->next, 
        0, aligned_start + size, split_length);
    free_space->next = leftover_index;

    return (void *)free_space->start;
}

void kmem_virtmapfree(uint64_t addr)
{
    kmem_virtmap *addr_space = 0;

    for (uint32_t entry = 0; (entry < kernel_vmmap_max) && ((uint64_t)addr_space == 0); entry++)
    {
        if (kernel_vmmap_start[entry].start <= addr && 
			addr < kernel_vmmap_start[entry].start + kernel_vmmap_start[entry].length)
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

uint64_t *ptab4;
uint8_t paging_ready = 0;

uint64_t kmem_newpagetable()
{
	void *new_pt = kmem_palloc(1, kmem_paging_1kb);
	
	if ((uint64_t)new_pt == 0)
		kcrash("Unable to allocate pagetables!");

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_npt: %x", new_pt);
#endif

	memset((void *)virt_from_phys((uint64_t)new_pt), 0, 0x1000);
	return (uint64_t)new_pt;
}

extern void kmem_flush(void *);

static inline uint64_t *kmem_pagegettable(uint64_t *pt, size_t index)
{
	uint64_t addr = pt[index] & ~0xFFF;
	
	if (paging_ready == 1)
		return (uint64_t *)virt_from_phys(addr);

	return (uint64_t *)(addr);
}

//assumes that all inputs are page aligned
void kmem_pageentry(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags, uint8_t sizing)
{
#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_pe: %x to %x size %x", physical, address, size);
#endif
	size = size + (address & 0xFFF);

	while (size)
	{
		uint64_t *pt3, *pt2, *pt1;
		size_t p4_index = (address >> 39) & 0x1FF;
		size_t p3_index = (address >> 30) & 0x1FF;
		size_t p2_index = (address >> 21) & 0x1FF;
		size_t p1_index = (address >> 12) & 0x1FF;

		if (sizing == kmem_paging_1kb)
		{
			if ((!ptab4[p4_index] & 0x1))
				ptab4[p4_index] = kmem_newpagetable() | 0x3;
			
			pt3 = kmem_pagegettable(ptab4, p4_index);
			if ((!pt3[p3_index] & 0x1))
				pt3[p3_index] = kmem_newpagetable() | 0x3;

			pt2 = kmem_pagegettable(pt3, p3_index);
			if ((!pt2[p2_index] & 0x1))
				pt2[p2_index] = kmem_newpagetable() | 0x3;

			pt1 = kmem_pagegettable(pt2, p2_index);
			pt1[p1_index] = physical | flags;

			physical += 0x1000;
			address += 0x1000;
			size -= 0x1000;
		}
		else if (sizing == kmem_paging_2mb)
		{
			if ((!ptab4[p4_index] & 0x1))
				ptab4[p4_index] = kmem_newpagetable() | 0x3;
			
			pt3 = kmem_pagegettable(ptab4, p4_index);
			if ((!pt3[p3_index] & 0x1))
				pt3[p3_index] = kmem_newpagetable() | 0x3;

			pt2 = kmem_pagegettable(pt3, p3_index);
			pt2[p2_index] = physical | (1 << 7) | flags;

			physical += 0x200000;
			address += 0x200000;
			size -= 0x200000;
		}
		else if (sizing == kmem_paging_1gb)
		{
			if ((!ptab4[p4_index] & 0x1))
				ptab4[p4_index] = kmem_newpagetable() | 0x3;
			
			pt3 = kmem_pagegettable(ptab4, p4_index);
			pt3[p3_index] = physical | (1 << 7) | flags;

			physical += 0x40000000;
			address += 0x40000000;
			size -= 0x40000000;
		}
		else
			kcrash("Requested paging mode not implemented!");
	}
}

//assumes that all inputs are page aligned
void kmem_unpageentry(uint64_t address, uint64_t size)
{
#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_upe: %x size %x", address, size);
#endif
	size += address & 0xFFF;

	while (size)
	{
		uint64_t *pt3, *pt2, *pt1;
		size_t p4_index = (address >> 39) & 0x1FF;
		size_t p3_index = (address >> 30) & 0x1FF;
		size_t p2_index = (address >> 21) & 0x1FF;
		size_t p1_index = (address >> 12) & 0x1FF;

		if (!(ptab4[p4_index] & 0x1))
		{
			kdebug_outf("\nkm_v: unpaging non-existent entry");
			break;
		}

		pt3 = kmem_pagegettable(ptab4, p4_index);
		if (!(pt3[p3_index] & 0x1))
		{
			kdebug_outf("\nkm_v: unpaging non-existent entry");
			break;
		}
		if (pt3[p3_index] & (1 << 7))
		{
			pt3[p3_index] &= ~1;
			kmem_flush((void *)address);

			address += 0x40000000;
			size -= 0x40000000;

			continue;
		}

		pt2 = kmem_pagegettable(pt3, p3_index);
		if (!(pt2[p2_index] & 0x1))
		{
			kdebug_outf("\nkm_v: unpaging non-existent entry");
			break;
		}
		if (pt2[p2_index] & (1 << 7))
		{
			pt2[p2_index] &= ~1;
			kmem_flush((void *)address);

			address += 0x40000000;
			size -= 0x40000000;

			continue;
		}

		pt1 = kmem_pagegettable(pt2, p2_index);
		if (pt1[p1_index] & 0x1)
			pt1[p1_index] &= ~1;
		else
			kdebug_outf("\nkm_v: unpaging non-existent entry");

		kmem_flush((void *)address);
		
		address += 0x1000;
		size -= 0x1000;
	}
}

//gives physical address given virtual page
void* kmem_getphysical(uint64_t *virt)
{
	uint64_t *pt3, *pt2, *pt1;
	size_t p4_index = ((uint64_t)virt >> 39) & 0x1FF;
	size_t p3_index = ((uint64_t)virt >> 30) & 0x1FF;
	size_t p2_index = ((uint64_t)virt >> 21) & 0x1FF;
	size_t p1_index = ((uint64_t)virt >> 12) & 0x1FF;

	if (!(ptab4[p4_index] & 0x1))
		kcrash("Translating non-existent page entry");

	pt3 = kmem_pagegettable(ptab4, p4_index);
	if (!(pt3[p3_index] & 0x1))
		kcrash("Translating non-existent page entry");
	if (pt3[p3_index] & (1 << 7))
		return (void*)(pt3[p3_index] & ~0x3FFFFFFF);

	pt2 = kmem_pagegettable(pt3, p3_index);
	if (!(pt2[p2_index] & 0x1))
		kcrash("Translating non-existent page entry");
	if (pt2[p2_index] & (1 << 7))
		return (void*)(pt2[p2_index] & ~0x1FFFFF);

	pt1 = kmem_pagegettable(pt2, p2_index);;
	if (pt1[p1_index] & 0x1)
		return (void*)(pt1[p1_index] & ~0xFFF);
	else
		kcrash("Translating non-existent page entry");

	return 0;
}

//pages into virtual memory
//and maps into vmem list
void* kmem_page(uint64_t address, uint64_t size, uint16_t flags, uint8_t sizing)
{
	uint64_t offset = 0;

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_vp: paging %x, %x len", address, size);
#endif

	switch (sizing)
	{
		case kmem_paging_1kb:
			offset = address & 0xFFF;
			size = (offset + size + 0xFFF) & ~0xFFF;
			address &= -0x1000ull;
			break;
		case kmem_paging_2mb:
			offset = address & 0x1FFFFF;
			size = (offset + size + 0x1FFFFF) & ~0x1FFFFF;
			address &= -0x200000ull;
			break;
		case kmem_paging_1gb:
			offset = address & 0x3FFFFFFF;
			size = (offset + size + 0x3FFFFFFF) & ~0x3FFFFFFF;
			address &= -0x40000000ull;
			break;
		default:
			kcrash("Requested paging mode not implemented!");
			break;
	}

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_vp: paging %x, %x len", address, size);
#endif

	if (kernel_vmmap_max)
    {
        uint64_t virtual_mapping = (uint64_t)kmem_virtmapalloc(size, sizing);
		kmem_pageentry(address, virtual_mapping, size, flags, sizing);
		
        return (void *)(virtual_mapping + offset);
    }

    kmem_pageentry(address, address, size, flags, sizing);

    return (void *)(address + offset);
}

void kmem_unpage(void *address, uint64_t size)
{
	size = (size + 0xFFF) & ~0xFFF;
    uint64_t addr = (uintptr_t)address;
    addr &= -0x1000ull;

    if (kernel_vmmap_max)
        kmem_virtmapfree(addr);

    kmem_unpageentry(addr, size);
}

void kmem_vmminit(uint64_t phys_low, uint64_t phys_hi)
{
	uint64_t boot_ptab4 = 0;
	asm volatile ("mov %%cr3, %0" : "=r"(boot_ptab4));

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_iv: bootcr3 %x", boot_ptab4);
	kdebug_outf("\nkm_iv: safemem %x", phys_from_virt(k_boottable.safe_mem));
#endif

	uint64_t *pt4 = (uint64_t *)kmem_palloc(1, kmem_paging_1kb);
	memset(pt4, 0, 0x1000);
	ptab4 = pt4;

	uint64_t gb_low = ((phys_low * 0x1000 + 0x40000000 - 1) / 0x40000000) * 0x40000000;
	uint64_t gb_hi = 0;
	kmem_pageentry(0, kernel_virtual, gb_low, kmem_paging_present | kmem_paging_writable, kmem_paging_1gb);
	if (phys_hi)
	{
		gb_hi = (((phys_hi * 0x1000) - 0x100000000 + 0x40000000 - 1) / 0x40000000) * 0x40000000;
		kmem_pageentry(0x100000000, kernel_virtual + 0x100000000, gb_hi, kmem_paging_present | kmem_paging_writable, kmem_paging_1gb);
	}

	kdebug_outf("\nkm_iv: pre-paging memory done");

	//when paging permissions set up, mark both as protected
	
	asm volatile("mov %0, %%cr3" ::"r"((uintptr_t)pt4));
	paging_ready = 1;

	ptab4 = (uint64_t *)virt_from_phys((uint64_t)pt4);
	
	kernel_vmmap_start = (void *)(k_boottable.safe_mem - 0x1000);
    kernel_vmmap_max = (0x1000 / sizeof(kmem_virtmap)) - 1;

    kernel_vmmap_start[0].start = kernel_virtual;
    kernel_vmmap_start[0].length = 0xFFFFFFFFFFFFF000 - kernel_virtual;
    kernel_vmmap_start[0].used = 0;
    kernel_vmmap_start[0].active = 1;
    kernel_vmmap_start[0].current = 0;

    (void)kmem_virtmapalloc(gb_low, kmem_paging_1kb);
	if (phys_hi)
	{
		void* temp = kmem_virtmapalloc(0x100000000 - gb_low, kmem_paging_1kb);
		(void)kmem_virtmapalloc(gb_hi, kmem_paging_1kb);
		kmem_virtmapfree((uint64_t)temp);
	}

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_iv: virtual memory topology");
	kmem_virtmap *vmmap = &kernel_vmmap_start[0];
	while (1)
	{
		kdebug_outf("\nkm_iv: used %x start %x len %x", vmmap->used, vmmap->start, vmmap->length);
		if (vmmap->next == 0)
			break;
		vmmap = &kernel_vmmap_start[vmmap->next];
	}
#endif

	kdebug_outf("\nkm_iv: paging done");
}