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
int16_t mx = 0, my = 0;

extern graphics_info kgraphics;

void kmouse_calc()
{
    kscreen_pos pos;
    pos.x = 0;
    pos.y = 10;
    kscreen_setpos(pos);

    mx += (mbyte[1] - ((mbyte[0] << 4) & 0x100));
    my += (mbyte[2] - ((mbyte[0] << 3) & 0x100));

    if (mx < 0)
        mx = 0;
    else if (mx > kgraphics.horizontal_res)
        mx = kgraphics.horizontal_res;
    if (my < 0)
        my = 0;
    else if (my > kgraphics.vertical_res)
        my = kgraphics.vertical_res;

    kscreen_putf("kmouse_interrupt: mdx %8x mdy %8x", (mbyte[1] - ((mbyte[0] << 4) & 0x100)), (mbyte[2] - ((mbyte[0] << 3) & 0x100)));
    kscreen_putf("\r\nkmouse_interrupt: left %b right %b middle %b", mbyte[0] & 1, (mbyte[0] >> 1) & 1, (mbyte[0] >> 2) & 1);
    kscreen_putf("\r\nkmouse_interrupt: cursor x %4d cursor y %4d", mx, my);
}

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
            kmouse_calc();
            break;
    }
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

    kmouse_wait(1);
    outb(0x64, 0xD4);

    kmouse_wait(1);
    outb(0x60, 0xF3);

    kmouse_wait(0);
    inb(0x60);

    kmouse_wait(1);
    outb(0x64, 0xD4);

    kmouse_wait(1);
    outb(0x60, 1);

    kmouse_wait(0);
    inb(0x60);

    kdesc_setinterruptfunc(12, *kmouse_interrupt);
}