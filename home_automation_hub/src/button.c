#include "lpc24xx.h"
#include "button.h"

void button_init(void) {
    FIO0DIR &= ~((1 << 10) | (1 << 11));                /* S1, S2 as inputs */
}


unsigned int button_read(unsigned int id) {
    unsigned int pin;
    if (id == BTN_PLUG) pin = 10;                       /* plug button is on P0.10 */
    else pin = 11;                                      /* doorbell button is on P0.11 */

    return (FIO0PIN >> pin) & 1;                        /* 1 = pressed */
}

unsigned int button_edge(unsigned int id) {
    static unsigned int prev[2] = {0, 0};
    unsigned int now  = button_read(id);
    unsigned int edge = (now && !prev[id - 1]);             /* 1 only on the 0 -> 1 transition */
    prev[id - 1] = now;                                     /* remember for next time */
    return edge;
}
