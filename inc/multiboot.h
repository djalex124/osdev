#pragma once

#define multiboot2_header_magic 0xE85250D6
#define multiboot2_boot_magic   0x36D76289

#ifndef ASSEMBLY

#include <stdint.h>

struct mutliboot_acpi_tag
{
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[];
};

struct multiboot_framebuffer_tag
{
    uint32_t type;
    uint32_t size;

    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  fb_type;
    uint16_t reserved;
}; // it is extended, but may not be required for this usage

struct multiboot_mmap_entry
{
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t zero;
};

struct multiboot_mmap_tag
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
};

struct multiboot_string_tag
{
    uint32_t type;
    uint32_t size;
    char string[];
};

struct multiboot_tag
{
    uint32_t type;
    uint32_t size;
};

#endif