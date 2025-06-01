#include <stdint.h>

#include <output/screen.h>

#include <x86_64/desc.h>
#include <x86_64/port.h>

static volatile uint64_t ticks;
static uint8_t waiting = 0;

void ksleep(unsigned int ms)
{
    ticks = 0;
    waiting = 1;
    while (ticks < ms)
    {
        asm("hlt");
        continue;
    }
    waiting = 0;
}

void kpit_interrupt()
{
    if (waiting)
        ticks++;
}

void kpit_init(int hz)
{
    int divisor = 1193182 / hz;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    
    kdesc_setinterruptfunc(0, *kpit_interrupt);
}