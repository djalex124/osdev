#include <output/screen.h>

#include <kernel/debug.h>

#include <x86_64/desc.h>
#include <x86_64/port.h>

#include <ps2/kbd.h>

const char kkeyboard_keymapUSqwerty[] =
{
    0, '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\r', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
    '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
}; //standard keys up to f12

const char kkeyboard_keymapUSqwerty_upper[] =
{
    0, '\e', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\r', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"',
    '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
}; //standard keys up to f12

kkeyboard_handler kkeyboard_input;

void kkeyboard_setinput(kkeyboard_handler input)
{
    kkeyboard_input = input;
}

kkeyboard_state keyboard_state;

void kkeyboard_interrupt()
{
    uint8_t scan = inb(0x60);

    if (scan == 0xE0)
    {
        keyboard_state.lastE0 = 1;
        return;
    }

    if (keyboard_state.lastE0 == 1)
    {
        if (scan == 0x1D)
            keyboard_state.rctrl = 1;
        else if (scan == 0x9D)
            keyboard_state.rctrl = 0;
        else if (scan == 0x38)
            keyboard_state.ralt = 1;
        else if (scan == 0xB8)
            keyboard_state.ralt = 0;
        keyboard_state.lastE0 = 0;
    }
    else
    {
        switch (scan)
        {
            case 0x1D:
                keyboard_state.lctrl = 1;
                break;
            case 0x9D:
                keyboard_state.lctrl = 0;
                break;
            case 0x2A:
                keyboard_state.lshift = 1;
                break;
            case 0xAA:
                keyboard_state.lshift = 0;
                break;
            case 0x36:
                keyboard_state.rshift = 1;
                break;
            case 0xB6:
                keyboard_state.rshift = 0;
                break;
            case 0x38:
                keyboard_state.lalt = 1;
                break;
            case 0xB8:
                keyboard_state.lalt = 0;
                break;
            case 0x3A:
                keyboard_state.capslk ^= 1;
                break;
            case 0x45:
                keyboard_state.numlk ^= 1;
                break;
            case 0x46:
                keyboard_state.scrlk ^= 1;
                break;
        }

        if (scan > 0x80 && kkeyboard_keymapUSqwerty[released(scan)])
            keyboard_state.pressed = 0;
        //    kscreen_putf("kkeyboard_interrupt: released [%c]", kkeyboard_keymapUSqwerty[released(scan)]);
        else if (scan < 0x80 && kkeyboard_keymapUSqwerty[scan])
            keyboard_state.pressed = 1;
        //    kscreen_putf("kkeyboard_interrupt: pressed  [%c]", kkeyboard_keymapUSqwerty[scan]);
    }

    keyboard_state.scancode = scan;

    if (keyboard_state.pressed)
        kkeyboard_input(&keyboard_state);
}

void kkeyboard_init()
{
    kdesc_setinterruptfunc(1, *kkeyboard_interrupt);
}