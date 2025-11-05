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

#include <kernel/kernel.h>
void kmem_init(boot_table *table);

void kmem_pageentry(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpageentry(uint64_t address, uint64_t size);

void* kmem_getphysical(uint64_t *virt);
void* kmem_page(uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpage(void *address, uint64_t size);

void* kmem_kalloc(uint64_t size);
void kmem_kfree(void *addr);

void* kmem_palloc(size_t pages);
void kmem_pfree(void* addr, size_t pages);

void* kmem_alloc(size_t pages);
void kmem_free(void* addr, size_t pages);

void kmem_printpmminfo();
void kmem_printinfo();

#endif