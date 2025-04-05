#pragma once

#define released(k) (k-0x80)

extern const char kkeyboard_keymapUSqwerty[];
extern const char kkeyboard_keymapUSqwerty_upper[];

typedef struct
{
    uint8_t scancode;
    uint8_t pressed : 1;
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
}kkeyboard_state;

typedef void (*kkeyboard_handler)(kkeyboard_state k);

void kkeyboard_setinput(kkeyboard_handler input);
void kkeyboard_init();