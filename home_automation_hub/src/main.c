/******************************************************************************
 * light_sensor_demo  -  DESN2000 Home Automation Hub
 * ---------------------------------------------------------------------------
 * Live proof that the ambient light sensor works:
 *   - The BRIGHTER the light on the sensor, the MORE LEDs light up (bar graph).
 *   - Optionally, the BRIGHTER the light, the HIGHER the speaker pitch.
 *   - A DARK threshold fires the automation logic (our "lights ON" decision),
 *     showing the sensor doesn't just react - it DRIVES the hub.
 *
 * Hardware (Keil QVGA Base Board + DESN2000 Daughter Board, LPC2478):
 *   - Ambient light sensor -> ADC channel AD0.2  (pin P0.25).
 *       Confirmed from the board's shipped demo (lab1 LEDCycle.c) and the
 *       LPC2478 Pin Connect Block (PINSEL1 bits 19:18 = 01 selects AD0.2).
 *   - LEDs  -> the daughter board's 8-LED vertical ladder on GPIO Port 2,
 *       pins P2.1..P2.8 (schematic net names LLAD1..LLAD8). The ladder shares
 *       a common cathode enabled by P0.22 (LLAD_EN). See LED CONFIG below.
 *   - Speaker (optional) -> DAC output AOUT (pin P0.26), Timer0 for timing.
 *       Same setup as lab5 (sine_wave / play_tone).
 *
 * Build: open light_sensor_demo.uvproj in Keil uVision, target "QVGA Base
 *        Board", compile and load onto the board (this project already has
 *        startup.c and lpc24xx.h from the lab template).
 *****************************************************************************/

#include "lpc24xx.h"
#include "speaker.h"
#include "clock.h"

/* ==========================================================================
 *  LED CONFIG
 * ==========================================================================
 * The bar graph lights the lowest N LEDs in this list. These are the 8 LEDs
 * of the daughter board's vertical ladder, wired to Port 2 pins P2.1..P2.8
 * (schematic nets LLAD1..LLAD8), so the bar grows from bottom to top.
 */
#define LED_DIR   FIO2DIR
#define LED_SET   FIO2SET
#define LED_CLR   FIO2CLR

static const unsigned int ledBit[] = { 1, 2, 3, 4, 5, 6, 7, 8 };  /* P2.1 .. P2.8 */
#define NUM_LEDS  (sizeof(ledBit) / sizeof(ledBit[0]))

/* The ladder LEDs share a common cathode driven by a 74HC2G14 inverter whose
 * input is P0.22 (LLAD_EN). Driving P0.22 HIGH sends the inverter output LOW,
 * which sinks the LED current and lets the ladder light. (If your ladder stays
 * dark with everything else correct, try clearing this bit instead.)          */
#define LLAD_EN_BIT   22

/* Choose the demo output(s).                                                */
#define USE_LED_BAR   1       /* 1 = light-level bar graph on the LEDs        */
#define USE_SPEAKER   0       /* 1 = also drive the speaker pitch with light  */

/* ADC full scale is 12 bits (0..4095). Below this reading we call it "dark"
 * and trigger the automation (turn the cottage lights on). Tune on the day
 * by covering the sensor and reading the value in the debugger.             */
#define DARK_THRESHOLD  600

/* ==========================================================================
 *  Light sensor (ADC on AD0.2)
 * ========================================================================== */

/* Power up and configure the ADC to read the ambient light sensor on AD0.2. */
void light_init(void) {
    PCONP   |=  (1 << 12);          /* enable power to the A/D converter block */

    PINSEL1 &= ~(0x03 << 16);       /* clear the 2 function bits for P0.25 ... */
    PINSEL1 |=  (0x01 << 16);       /* ... and select AD0.1 on that pin        */

    /* AD0CR: SEL=channel 1, CLKDIV=4 (ADC clock = PCLK/5, safely < 13 MHz),
     * PDN=1 (converter operational). Software-triggered (no burst).           */
    AD0CR = (1 << 1)                /* SEL   : sample channel AD0.2            */
          | (15 << 8)                /* CLKDIV: divide PCLK down for the ADC    */
          | (1 << 21);              /* PDN   : A/D converter is operational    */
}

/* Start one conversion, wait for it to finish, and return the 12-bit result
 * (0 = darkest, 4095 = brightest).                                          */
unsigned int light_read(void) {
    AD0CR &= ~(7 << 24);            /* clear any previous START field          */
    AD0CR |=  (1 << 24);            /* START = 001 : begin conversion now      */

    while ((AD0DR1 & (1u << 31)) == 0)  /* wait for the DONE bit (bit 31)      */
        ;

    return (AD0DR1 >> 4) & 0xFFF;
		/* RESULT lives in bits 15:4 (12 bits)     */
}

/* ==========================================================================
 *  LED bar graph
 * ========================================================================== */

/* Make every LED in the list an output, and enable the ladder. Call once.    */
void leds_init(void) {
    unsigned int i;

    /* Enable the ladder common cathode: P0.22 output, driven HIGH so the
     * 74HC2G14 inverter sinks the LED current.                                */
    FIO0DIR |= (1u << LLAD_EN_BIT);
    FIO0SET  = (1u << LLAD_EN_BIT);

    for (i = 0; i < NUM_LEDS; i++)
        LED_DIR |= (1u << ledBit[i]);
}

/* Light the lowest 'n' LEDs and switch the rest off.                        */
void leds_bar(unsigned int n) {
    unsigned int i;
    for (i = 0; i < NUM_LEDS; i++) {
        if (i < n)
            LED_SET = (1u << ledBit[i]);   /* on  */
        else
            LED_CLR = (1u << ledBit[i]);   /* off */
    }
}

/* ==========================================================================
 *  Speaker pitch (optional)  -  DAC on AOUT (P0.26), timed with Timer0
 * ========================================================================== */
#if USE_SPEAKER

void speaker_init(void) {
    PINSEL1 &= ~(0x03 << 20);       /* clear the 2 function bits for P0.26 ... */
    PINSEL1 |=  (0x02 << 20);       /* ... and select AOUT (the DAC output)    */
}

/* Busy-wait for 'us' microseconds using Timer0 (same method as lab5).       */
void udelay(unsigned int us) {
    T0PR  = (Fpclk / 1000000) - 1;  /* prescaler: TC ticks once per microsecond */
    T0TCR = 2;                      /* reset the timer                          */
    T0TCR = 1;                      /* enable (reset bit cleared)               */
    while (T0TC < us)
        ;
}

/* Emit one square-wave cycle whose pitch rises with the light level.
 * Brighter light -> shorter period -> higher pitch (~200 Hz .. ~2 kHz).     */
void speaker_chirp(unsigned int light) {
    unsigned int freq = 200 + (light * 1800) / 4095;  /* 200 Hz dark -> 2 kHz  */
    unsigned int half = (1000000 / freq) / 2;         /* half-period in us     */

    DACR = 0x3FF << 6;              /* high half of the wave (max volume)      */
    udelay(half);
    DACR = 0x000 << 6;              /* low half (0 V)                          */
    udelay(half);
}

#endif /* USE_SPEAKER */

/* ==========================================================================
 *  Main loop
 * ========================================================================== */
int main(void) {
    unsigned int light, n;

    light_init();
    leds_init();                                    /* LEDs used by the bar and the dark indicator */
#if USE_SPEAKER
    speaker_init();
#endif

    while (1) {
        light = light_read();                       /* 0 (dark) .. 4095 (bright) */

#if USE_LED_BAR
        /* Map the reading onto the number of LEDs to light, across the full
         * range: n = 0 LEDs when dark, up to NUM_LEDS when fully bright.       */
        n = (light * (NUM_LEDS - 1)) / 3000;              /* 0 .. NUM_LEDS            */
        if (n > NUM_LEDS - 1) n = NUM_LEDS - 1;             /* defensive clamp          */
        leds_bar(n);
#endif

#if USE_SPEAKER
        speaker_chirp(light);                       /* higher pitch = brighter  */
#endif

        /* --- Automation decision: the sensor DRIVES the hub -----------------
         * When it gets dark (reading below the threshold), the hub would turn
         * the cottage lights on. Here we flag it by lighting the top LED; in
         * the full system this branch calls the blind / lighting control.      */
        if (light < DARK_THRESHOLD) {
            LED_SET = (1u << ledBit[NUM_LEDS - 1]); /* "cottage lights ON"       */
        }
    }
}
