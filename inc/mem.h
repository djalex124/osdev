#pragma once

#define kernel_virtual   0xFFFFFF8000000000 
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)

#ifndef ASSEMBLY

#include <kernel.h>

#define kernel_space 0x200000

void kmem_init(kernel_table *table);

void kmem_page(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpage(uint64_t address, uint64_t size);

void* kmem_alloc(uint64_t size);
void kmem_free(void* addr);

#endif