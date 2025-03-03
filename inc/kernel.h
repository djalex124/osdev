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

typedef struct {
    uint32_t horizontal_res;
    uint32_t vertical_res;
    uint32_t ppsl;
    uint64_t *framebuffer_base;
} graphics_info;

typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t num_pages;
    uint64_t attribute;
} memory_descriptor;

typedef struct {
    graphics_info graphics;
    memory_descriptor *mmap;
    uint64_t mmap_enteries;
    uint64_t mmap_size;
    uint64_t safe_mem;
} kernel_table;