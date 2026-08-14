#ifndef TOUCH_H
#define TOUCH_H

/* ==========================================================================
 *  touch.h - TSC2046 resistive touchscreen (QVGA Base Board)
 *
 *  The panel is two transparent resistive layers separated by a small gap.
 *  Pressing shorts them together; the TSC2046 measures where by driving a
 *  voltage across one layer and reading the voltage on the other. It talks
 *  to the LPC2478 over the legacy SPI0 port, sharing P0.15/17/18 with the
 *  LCD panel controller (different chip select: touch is P0.20, LCD P0.16).
 *
 *  All readings are 8-bit (0..255), differential mode, with the controller
 *  powering down between conversions - the mode the DESN2000 simulator
 *  supports and the mode the datasheet recommends for pressure.
 * ========================================================================== */

/* Configure SPI0 and the chip select. Safe to call at any point after
 * lcdInit(); unlike the lab 6 version this sets the pin functions
 * explicitly rather than assuming the LCD driver got there first. */
void touch_init(void);

/* Non-zero while the screen is being pressed.
 *
 * Uses Z1, which the TSC2046 measures across the two layers: it sits near
 * zero when nothing is touching (open circuit) and rises on contact. This
 * is the opposite sense to lab 6's z2 - z1, which is at its LARGEST when
 * untouched - the reason the original never registered a press. */
unsigned int touch_pressed(void);

/* Read a touch position in screen pixels: x = 0..239, y = 0..319.
 * Returns 0 and leaves x and y alone if the screen is not being pressed.
 * Each axis is the median of three samples, which throws out the single
 * wild reading that resistive panels produce as contact settles. */
unsigned int touch_get_point(unsigned short *x, unsigned short *y);

/* Raw controller values, for calibration and debugging. */
void touch_read_raw(unsigned char *x,
                    unsigned char *y,
                    unsigned char *z1,
                    unsigned char *z2);

#endif
