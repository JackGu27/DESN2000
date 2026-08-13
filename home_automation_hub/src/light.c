#include "lpc24xx.h"
#include "light.h"

/* ==========================================================================
 *  Light sensor (ADC on AD0.1)
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
