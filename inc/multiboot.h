#pragma once

#define multiboot2_magic 0xE85250D6

#define kernel_virtual   0xFFFFFF8000000000
#define phys_from_virt(x) ((x) - kernel_virtual)
#define virt_from_phys(x) ((x) + kernel_virtual)