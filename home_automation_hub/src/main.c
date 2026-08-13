#include "lpc24xx.h"
#include "light.h"
#include "led.h"
#include "button.h"
#include "speaker.h"
#include "clock.h"
#include "automation.h"

/* ==========================================================================
 *  Main loop
 * ========================================================================== */
int main(void) {
    unsigned int light;

    // Set up Section 1
    light_init();
    led_init();
    // Set up Section 2
    speaker_init();
    clock_init();

	// Set up Section 3
	automation_init();

    // Set P0.11 as GPIO
    PINSEL0 &= ~(3u << 22);
    // Set P0.11 as input
    FIO0DIR &= ~(1u << 11);

    while (1) {
		// Keep the software clock updated
        clock_update();
		
        // Check the doorbell button
        if ((FIO0PIN & (1u << 11)) != 0)
        {
            chime();

            // Wait until the button is released
            while ((FIO0PIN & (1u << 11)) != 0)
            {
            }
        }

        light = light_read();
        // section 3
		automation_update(light);
    }

}
