#include "lpc24xx.h"
#include "light.h"
#include "led.h"
#include "button.h"

/* ADC full scale is 12 bits (0..4095). Below this reading we call it "dark"
 * and trigger the automation (turn the cottage lights on). Tune on the day
 * by covering the sensor and reading the value in the debugger.             */
#define DARK_THRESHOLD  600

/* ==========================================================================
 *  Main loop
 * ========================================================================== */
int main(void) {
    unsigned int light;

    light_init();
    led_init();

    while (1) {
        light = light_read();                       /* 0 (dark) .. 4095 (bright) */

    }
}
