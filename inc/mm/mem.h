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
    uint8_t  eos : 1;
    uint8_t  eom : 1;
    uint8_t  pad : 5;
}__attribute__((packed)) kmem_stack;
//use pad as needed later. maybe user/os split and process flags?

void kmem_physinit();
void kmem_virtinit();

//#define AQUA_DEBUG_MEM

#include <kernel/kernel.h>
void kmem_init(boot_table *table);

void kmem_page(uint64_t physical, uint64_t address, uint64_t size, uint16_t flags);
void kmem_unpage(uint64_t address, uint64_t size);

void* kmem_kalloc(uint64_t size);
void kmem_kfree(uint64_t size);

void* kmem_alloc(size_t pages);
void kmem_free(void* addr, size_t pages);

void kmem_printinfo();

#endif