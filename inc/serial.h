#pragma once

#include <stdint.h>

void kserial_init();

void kserial_outf(const char *fmt, ...);

static inline uint8_t inb(uint16_t p)
{
    uint8_t ret;
    asm volatile (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(p)
        : "memory");
    return ret;
}

static inline void outb(uint16_t p, uint8_t v)
{
    asm volatile ( "outb %0, %1" : : "a"(v), "Nd"(p) :"memory" );
}

#define PORT1 0x3F8