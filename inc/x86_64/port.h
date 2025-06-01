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

inline uint32_t inl(uint16_t p)
{
    uint32_t ret;
    asm volatile (
        "inl %1, %0"
        : "=a"(ret)
        : "Nd"(p)
        : "memory");
    return ret;
}

inline void outb(uint16_t p, uint8_t v)
{
    asm volatile ( "outb %0, %1" : : "a"(v), "Nd"(p) :"memory" );
}

inline void outw(uint16_t p, uint16_t v)
{
    asm volatile ( "outw %0, %1" : : "a"(v), "Nd"(p) :"memory" );
}

inline void outl(uint16_t p, uint32_t v)
{
    asm volatile ( "outl %0, %1" : : "a"(v), "Nd"(p) :"memory" );
}