#include <screen.h>
#include <port.h>
#include <desc.h>

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
    outb(0x64, 0xA8);
    outb(0x64, 0x20);
    uint8_t s = inb(0x60) | 2;
    outb(0x64, 0x60);
    outb(0x60, s);

    outb(0x64, 0xD4);
    outb(0x60, 0xF6);
    inb(0x60);

    outb(0x64, 0xD4);
    outb(0x60, 0xF4);
    inb(0x60);

    kdesc_setinterruptfunc(12, *kmouse_interrupt);
}