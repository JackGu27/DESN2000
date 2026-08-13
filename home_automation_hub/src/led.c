#include "lpc24xx.h"
#include "led.h"

void led_init(void) {
    FIO3DIR |= (1 << 16) | (1 << 17) | (1 << 18);   // make P3.16, P3.17, P3.18 outputs
    FIO3DIR |= (1 << 19) | (1 << 20) | (1 << 21);   // make P3.19, P3.20, P3.21 outputs
}

/* blind_set: show a blind's position on its tricolor LED.
 *   which = 1 (P3.16-18) or 2 (P3.19-21)
 *   state = UP(red) / MID(green) / DOWN(blue)
 * ProjectBrief.md Activity 2: up=red, mid=green, down=blue */
void blind_set(unsigned int which, unsigned int state) {
    unsigned int base;

    if (which == 1) {
        base = 16;
    } else {
        base = 19;
    }

    FIO3CLR = (7 << base);
    FIO3SET = (1 << (base + state));
}
