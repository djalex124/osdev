#pragma once

#define kernel_virtual   0xFFFFFF8000000000 
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)

#ifndef ASSEMBLY

#include <stddef.h>

typedef struct
{
    uint32_t page;
    uint8_t  used : 1;
    uint8_t  eom : 1;
    uint8_t  pad : 6;
}__attribute__((packed)) kmem_bitmap;
//use pad as needed later. maybe user/os split and process flags?

void kmem_physinit();
void kmem_virtinit();
void kmem_virtbuildmap();

//#define AQUA_DEBUG_MEM

#include <kernel/kernel.h>
void kmem_init(boot_table *table);

void kmem_pageinternal(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpageinternal(uint64_t address, uint64_t size);

void* kmem_page(uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpage(void *address, uint64_t size);

void* kmem_kalloc(uint64_t size);
void kmem_kfree(uint64_t size);

void* kmem_alloc(size_t pages);
void kmem_free(void* addr, size_t pages);

void kmem_printinfo();

#endif