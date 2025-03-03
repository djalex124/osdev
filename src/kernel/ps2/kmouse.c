#include <screen.h>
#include <port.h>
#include <desc.h>

static inline void kmouse_wait(uint8_t type)
{
    unsigned wait = 100000;
    if (type == 0)
    {
        while (wait--)
        {
            if ((inb(0x64) & 1) == 1)
                return;
        }
        return;
    }
    else
    {
        while (wait--)
        {
            if ((inb(0x64) & 2) == 0)
                return;
        }
        return;
    }
}

uint8_t mcycle = 0;
uint8_t mbyte[3];
uint16_t mx = 0, my = 0;

void kmouse_interrupt()
{
    switch (mcycle)
    {
        case 0:
            mbyte[0] = inb(0x60);
            mcycle++;
            break;
        case 1:
            mbyte[1] = inb(0x60);
            mcycle++;
            break;
        case 2:
            mbyte[2] = inb(0x60);
            mcycle = 0;
    }
    
    kscreen_pos pos;
    pos.x = 0;
    pos.y = 10;
    kscreen_setpos(pos);

    kscreen_putf("kmouse_interrupt: mdx %8x mdy %8x", (mbyte[1] - ((mbyte[0] << 4) & 0x100)), (mbyte[2] - ((mbyte[0] << 3) & 0x100)));
    kscreen_putf("\r\nkmouse_interrupt: left %b right %b middle %b", mbyte[0] & 1, (mbyte[0] >> 1) & 1, (mbyte[0] >> 2) & 1);
}

void kmouse_init()
{
    kmouse_wait(1);
    outb(0x64, 0xA8);

    kmouse_wait(1);
    outb(0x64, 0x20);

    kmouse_wait(0);
    uint8_t s = inb(0x60) | 2;

    kmouse_wait(1);
    outb(0x64, 0x60);

    kmouse_wait(1);
    outb(0x60, s);

    kmouse_wait(1);
    outb(0x64, 0xD4);

    kmouse_wait(1);
    outb(0x60, 0xF6);

    kmouse_wait(0);
    inb(0x60);

    kmouse_wait(1);
    outb(0x64, 0xD4);

    kmouse_wait(1);
    outb(0x60, 0xF4);

    kmouse_wait(0);
    inb(0x60);

    kdesc_setinterruptfunc(12, *kmouse_interrupt);
}