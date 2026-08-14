#pragma once

#define kernel_virtual   0xFFFFFF8000000000 
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)

#ifndef ASSEMBLY

#include <stddef.h>
#include <stdint.h>

void kmem_pmminit(uint64_t low_size, uint64_t high_size);
void kmem_heapinit();
void kmem_vmminit(uint64_t low_size, uint64_t high_size);

//#define AQUA_DEBUG_MEM

#define kmem_paging_1gb 0x2
#define kmem_paging_2mb 0x1
#define kmem_paging_1kb 0x0

#define kmem_paging_present  (1 << 0)
#define kmem_paging_writable (1 << 1)
#define kmem_paging_user     (1 << 2)
#define kmem_paging_no_cache (1 << 4)

#include <kernel/kernel.h>
void kmem_init(boot_table *table);

uint64_t kmem_pagegetcr3();
void kmem_pagesetcr3(uint64_t ptab4);

void kmem_pageentry(uint64_t ptab4, uint64_t physical, uint64_t address, uint64_t size, uint16_t flags, uint8_t sizing);
void kmem_unpageentry(uint64_t ptab4, uint64_t address, uint64_t size);
void* kmem_getphysical(uint64_t ptab4, uint64_t *virt);

void* kmem_page(uint64_t address, uint64_t size, uint16_t flags, uint8_t sizing);
void kmem_unpage(void *address, uint64_t size);

void* kmem_kalloc(uint64_t size);
void kmem_kfree(void *addr);

void* kmem_palloc(size_t pages, uint8_t align);
void kmem_pfree(void* addr, size_t pages);

void* kmem_alloc(size_t pages);
void kmem_free(void* addr, size_t pages);

void kmem_printpmminfo();
void kmem_printinfo();

#ifdef AQUA_DEBUG_MEM
void kmem_vmm_traverse(uint64_t cr2);
#endif

#endif