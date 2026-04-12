#pragma once

#define AQUA_VER_MAJOR 0
#define AQUA_VER_MINOR 0
#define AQUA_VER_REV   1
#define AQUA_VER_STR   alpha

#define string2(s) #s
#define string1(s) string2(s)

#ifdef AQUA_DEBUG
#define AQUA_VER_STRING string1(AQUA_VER_MAJOR) "." string1(AQUA_VER_MINOR) "." string1(AQUA_VER_REV) "-" string1(AQUA_VER_STR) "+" string1(AQUA_VER_BUILD) ".debug"
#else
#define AQUA_VER_STRING string1(AQUA_VER_MAJOR) "." string1(AQUA_VER_MINOR) "." string1(AQUA_VER_REV) "-" string1(AQUA_VER_STR) "+" string1(AQUA_VER_BUILD)
#endif

#include <stdint.h>

typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t num_pages;
    uint64_t attribute;
} efi_memory_descriptor;

typedef struct {
    uint32_t horizontal_res;
    uint32_t vertical_res;
    uint32_t ppsl;
    uint64_t *framebuffer_base;
} graphics_info;

typedef struct {
    graphics_info graphics;
    efi_memory_descriptor *mmap;
    uint64_t mmap_enteries;
    uint64_t mmap_size;
    uint64_t safe_mem;
    uint8_t acpi_ver;
    uint64_t rsdp;
} boot_table;

#include <stddef.h>
#include <mm/mem.h>
#include <x86_64/pci.h>
#include <fs/fs.h>

typedef struct {
    graphics_info *k_graphics;
    
    uint64_t *mcfg_table;

    size_t kpci_tablesize;
    kpci_device *kpci_table;

    kfs_partition *kfs_partitions[16];
} info_table;

#ifndef kernel_file
extern boot_table k_boottable;
extern info_table k_infotable;
#endif