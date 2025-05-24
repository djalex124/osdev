#include <screen.h>
#include <debug.h>
#include <port.h>
#include <desc.h>
#include <kbd.h>

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
}

void kmouse_interrupt()
{
    uint8_t in = inb(0x60);
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
    kscreen_putf("\nkmouse_test: mb0 %8b mb1 %8b mb2 %8b", mbyte[0], mbyte[1], mbyte[2]);
    kscreen_putf("\nkmouse_test: left %b right %b middle %b", mbyte[0] & 1, (mbyte[0] >> 1) & 1, (mbyte[0] >> 2) & 1);
    kscreen_putf("\nkmouse_test: cursor x %4d cursor y %4d", mx, my);
}

void kmouse_test()
{
    kkeyboard_setinput(kmouse_testinput);
    kscreen_putf("\nkmouse_test: graphics x%4d y%4d ", kgraphics.horizontal_res, kgraphics.vertical_res);
    kmouse_print();
    stay = kscreen_getpos();
    stay.y -= 3;
    while (kkeyboard_keymapUSqwerty[check->scancode] != '\e')
    {
        kscreen_setpos(stay);
        kmouse_print();
    }
}


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

void kmouse_cmd(uint8_t cmd)
{
    kmouse_wait(1);
    outb(0x64, 0xD4);

    kmouse_wait(1);
    outb(0x60, cmd);

    kmouse_wait(0);
    inb(0x60);
}

void kmouse_init()
{
    kmouse_cmd(0xF5); //if enabled, disable packets

    kmouse_wait(1);
    outb(0x64, 0xA8); //enable 2nd ps2 port

    kmouse_wait(1);
    outb(0x64, 0x20);
    kmouse_wait(0);
    uint8_t status = inb(0x60) | 2;
    kmouse_wait(1);
    outb(0x64, 0x60);
    kmouse_wait(1);
    outb(0x60, status); //ensure both ports can send data
    
    kmouse_cmd(0xFF);
    while (inb(0x60) != 0); //reset mouse and clear buffer
    
    //kmouse_cmd(0xF2); //get mouse type, can enable 4th/5th buttons later
    //uint8_t mouse_type = inb(0x60);

    kmouse_cmd(0xF6); //set defaults
    kmouse_cmd(0xF4); //enable packets

    mx = kgraphics.horizontal_res / 2;
    my = kgraphics.vertical_res / 2;
    
    kdesc_setinterruptfunc(12, *kmouse_interrupt);
}