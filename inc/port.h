#pragma once

#include <stdint.h>

inline uint8_t inb(uint16_t p)
{
    uint8_t ret;
    asm volatile (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(p)
        : "memory");
    return ret;
}

inline void outb(uint16_t p, uint8_t v)
{
    asm volatile ( "outb %0, %1" : : "a"(v), "Nd"(p) :"memory" );
}