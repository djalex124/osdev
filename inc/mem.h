#pragma once

#define kernel_virtual   0xFFFFFF8000000000
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)

#ifndef ASSEMBLY

#include <multiboot.h>

void kmem_init(struct multiboot_mmap_entry *mmap);

void kmem_page(uint64_t physical, uint64_t address, uint16_t flags);
void kmem_unpage(uint64_t address);

#endif