#include <screen.h>
#include <debug.h>
#include <port.h>
#include <desc.h>
#include <kbd.h>

static inline void kps2_wait(uint8_t type)
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

    kdebug_outf("\r\nkmouse_test: mdx %8x mdy %8x", (mbyte[1] - ((mbyte[0] << 4) & 0x100)), (mbyte[2] - ((mbyte[0] << 3) & 0x100)));
    kdebug_outf("\r\nkmouse_test: left %b right %b middle %b", mbyte[0] & 1, (mbyte[0] >> 1) & 1, (mbyte[0] >> 2) & 1);
    kdebug_outf("\r\nkmouse_test: cursor x %4d cursor y %4d", mx, my);
}

void kmouse_interrupt()
{
    uint8_t in = inb(0x60);
    kdebug_outf("\r\nkmouse_test: in %x cycle %x", in, mcycle);
    switch (mcycle)
    {
        case 0:
            mbyte[0] = in;
            mcycle++;
            break;
        case 1:
            mbyte[1] = in;
            mcycle++;
            break;
        case 2:
            mbyte[2] = in;
            mcycle = 0;
            kmouse_calc();
            break;
    }
}

kkeyboard_state *check;
kscreen_pos stay;

void kmouse_testinput(kkeyboard_state *k)
{
    check = k;
}

void kmouse_print()
{
    kscreen_putf("\nkmouse_test: mb0 %x mb1 %x mb2 %x", mbyte[0], mbyte[1], mbyte[2]);
    kscreen_putf("\nkmouse_test: left %b right %b middle %b", mbyte[0] & 1, (mbyte[0] >> 1) & 1, (mbyte[0] >> 2) & 1);
    kscreen_putf("\nkmouse_test: cursor x %x cursor y %x", mx, my);
}

void kmouse_test()
{
    kkeyboard_setinput(kmouse_testinput);
    kscreen_putf("\nkmouse_test: graphics x%d y%d ", kgraphics.horizontal_res, kgraphics.vertical_res);
    kmouse_print();
    stay = kscreen_getpos();
    stay.y -= 3;
    while (kkeyboard_keymapUSqwerty[check->scancode] != '\e')
    {
        kscreen_setpos(stay);
        kmouse_print();
    }
}

void kmouse_command(uint8_t cmd)
{
    kps2_wait(1);
    outb(0x64, 0xD4);

    kps2_wait(1);
    outb(0x60, cmd);

    kps2_wait(0);
    inb(0x60);
}

void kps2_command(uint8_t cmd)
{
    kps2_wait(1);
    outb(0x60, cmd);
    kps2_wait(0);
}

void kmouse_init()
{
    kps2_command(0xAD);
    kps2_command(0xA7);

    while (inb(0x64) & 1) inb(0x60);
    
    kps2_wait(1);
    outb(0x64, 0x20);

    kps2_wait(0);
    uint8_t s = inb(0x60) | 2;

    kps2_wait(1);
    outb(0x64, 0x60);

    kps2_wait(1);
    outb(0x60, s);

    kps2_command(0xAE);
    kps2_command(0xA8);

    kmouse_command(0xFF);

    kmouse_command(0xF6);
    kmouse_command(0xF4);

    //kmouse_command(0xF3);

    //kps2_wait(1);
    //outb(0x64, 0xD4);

    //kps2_wait(1);
    //outb(0x60, 1);

    //kps2_wait(0);
    //inb(0x60);

    kdesc_setinterruptfunc(12, *kmouse_interrupt);
}