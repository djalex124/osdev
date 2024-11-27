#include <screen.h>
#include <port.h>
#include <desc.h>

#define released(k) (k-0x80)

//E = escape, B = backspace, T = tab, R = enter
static char kkeyboard_keymapUSqwerty[] =
{
    0, 'E', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 'B',
    'T', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    'R', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
    '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
}; //standard keys up to f12

typedef struct
{
    uint8_t lshift : 1;
    uint8_t rshift : 1;
    uint8_t lctrl : 1;
    uint8_t rctrl : 1;
    uint8_t lalt : 1;
    uint8_t ralt : 1;
    uint8_t numlk : 1;
    uint8_t scrlk : 1;
    uint8_t capslk : 1;
    uint8_t lastE0 : 1;
}kkeyboard_special;

static kkeyboard_special special_keys;

void kkeyboard_interrupt()
{
    uint8_t scan = inb(0x60);

    if (scan == 0xE0)
    {
        special_keys.lastE0 = 1;
        return;
    }

    kscreen_pos pos;
    pos.x = 0;
    pos.y = 15;
    kscreen_setpos(pos);

    if (special_keys.lastE0 == 1)
    {
        if (scan == 0x1D)
            special_keys.rctrl = 1;
        else if (scan == 0x9D)
            special_keys.rctrl = 0;
        else if (scan == 0x38)
            special_keys.ralt = 1;
        else if (scan == 0xB8)
            special_keys.ralt = 0;
        special_keys.lastE0 = 0;
        kscreen_putf("kkeyboard_interrupt: ext-key  [0]");
    }
    else
    {
        switch (scan)
        {
            case 0x1D:
                special_keys.lctrl = 1;
                break;
            case 0x9D:
                special_keys.lctrl = 0;
                break;
            case 0x2A:
                special_keys.lshift = 1;
                break;
            case 0xAA:
                special_keys.lshift = 0;
                break;
            case 0x36:
                special_keys.rshift = 1;
                break;
            case 0xB6:
                special_keys.rshift = 0;
                break;
            case 0x38:
                special_keys.lalt = 1;
                break;
            case 0xB8:
                special_keys.lalt = 0;
                break;
            case 0x3A:
                special_keys.capslk ^= 1;
                break;
            case 0x45:
                special_keys.numlk ^= 1;
                break;
            case 0x46:
                special_keys.scrlk ^= 1;
                break;
        }

        if (scan > 0x80 && kkeyboard_keymapUSqwerty[released(scan)])
            kscreen_putf("kkeyboard_interrupt: released [%c]", kkeyboard_keymapUSqwerty[released(scan)]);
        else if (scan < 0x80 && kkeyboard_keymapUSqwerty[scan])
            kscreen_putf("kkeyboard_interrupt: pressed  [%c]", kkeyboard_keymapUSqwerty[scan]);
    }

    kscreen_putf("\r\nkkeyboard_interrupt: left  shift %b left  ctrl %b left  alt %b caps %b", 
        special_keys.lshift, special_keys.lctrl, special_keys.lalt, special_keys.capslk);
    kscreen_putf("\r\nkkeyboard_interrupt: right shift %b right ctrl %b right alt %b numlk %b scrlk %b", 
        special_keys.rshift, special_keys.rctrl, special_keys.ralt, special_keys.numlk, special_keys.scrlk);
}

void kkeyboard_init()
{
    kdesc_setinterruptfunc(1, *kkeyboard_interrupt);
}