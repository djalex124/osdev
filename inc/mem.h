#pragma once

#define kernel_virtual   0xFFFFFF8000000000 
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)

#ifndef ASSEMBLY

#include <kernel.h>
#include <stddef.h>

void kmem_init(boot_table *table);

void kmem_page(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpage(uint64_t address, uint64_t size);

void* kmem_kalloc(uint64_t size);

void* kmem_alloc(size_t pages);
void kmem_free(void* addr, size_t pages);

void kmem_printinfo();

#endif