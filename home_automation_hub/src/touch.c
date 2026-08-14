#include "touch.h"
#include "lpc24xx.h"

/* --------------------------------------------------------------------------
 *  Hardware
 * ------------------------------------------------------------------------ */
#define CS_PIN            0x00100000u   /* P0.20, touch chip select         */

/* TSC2046 control bytes. Bit 7 starts a conversion, bits 6:4 select the
 * measurement, bit 3 = 1 selects 8-bit mode, bit 2 = 0 selects differential
 * reference, bits 1:0 = 00 powers down between conversions. */
#define CMD_READ_Y        0x98u
#define CMD_READ_X        0xD8u
#define CMD_READ_Z1       0xB8u
#define CMD_READ_Z2       0xC8u

/* --------------------------------------------------------------------------
 *  Calibration - the only things worth changing if the touch lands wrong
 *
 *  TOUCH_SWAP_XY   the panel is mounted rotated relative to the controller
 *  TOUCH_INVERT_*  an axis counts the opposite way to the screen
 *  TOUCH_*_MIN/MAX the raw range actually reachable; resistive panels never
 *                  quite reach 0 or 255, so trimming these stops the edges
 *                  of the screen being unreachable
 *  TOUCH_Z1_MIN    how hard a press has to be before it counts
 * ------------------------------------------------------------------------ */
#define TOUCH_SWAP_XY     0
#define TOUCH_INVERT_X    0
#define TOUCH_INVERT_Y    0

#define TOUCH_X_MIN       0u
#define TOUCH_X_MAX       255u
#define TOUCH_Y_MIN       0u
#define TOUCH_Y_MAX       255u

#define TOUCH_Z1_MIN      8u

#define SCREEN_W          240u
#define SCREEN_H          320u

/* --------------------------------------------------------------------------
 *  One SPI conversion
 * ------------------------------------------------------------------------ */
static unsigned char touch_xfer(unsigned char command)
{
    unsigned short result;

    FIO0CLR = CS_PIN;                 /* select the controller             */

    S0SPDR = command;                 /* send the command, ignore what     */
    while ((S0SPSR & 0x80u) == 0u) {} /* comes back on MISO                */
    result = S0SPDR;

    S0SPDR = 0x00u;                   /* clock out zeros to shift the      */
    while ((S0SPSR & 0x80u) == 0u) {} /* answer in                         */
    result = S0SPDR;

    FIO0SET = CS_PIN;                 /* release the controller            */

    return (unsigned char)result;
}

/* Middle value of three - cheap noise rejection without a running average,
 * which would lag behind a finger that has just landed. */
static unsigned char median3(unsigned char a, unsigned char b, unsigned char c)
{
    unsigned char t;

    if (a > b) { t = a; a = b; b = t; }
    if (b > c) { t = b; b = c; c = t; }
    if (a > b) { t = a; a = b; b = t; }

    return b;
}

static unsigned char sample(unsigned char command)
{
    unsigned char a = touch_xfer(command);
    unsigned char b = touch_xfer(command);
    unsigned char c = touch_xfer(command);

    return median3(a, b, c);
}

/* Scale a raw reading onto a screen axis, clamping at both ends. */
static unsigned short map_axis(unsigned char raw,
                               unsigned int lo,
                               unsigned int hi,
                               unsigned int span,
                               unsigned int invert)
{
    unsigned int v = (unsigned int)raw;
    unsigned int out;

    if (v <= lo) { v = lo; }
    if (v >= hi) { v = hi; }

    out = ((v - lo) * (span - 1u)) / (hi - lo);

    if (invert)
    {
        out = (span - 1u) - out;
    }

    return (unsigned short)out;
}

/* --------------------------------------------------------------------------
 *  Public
 * ------------------------------------------------------------------------ */
void touch_init(void)
{
    /* Make sure the SPI peripheral is powered (PCONP bit 8). */
    PCONP |= (1u << 8);

    /* P0.15 = SCK, P0.17 = MISO, P0.18 = MOSI - function 11 in each case,
     * which is the LEGACY SPI port. Function 10 would select SSP0, a
     * different peripheral with different registers.
     *
     * These are assignments, not the |= that lab 6 uses: OR-ing 10 onto
     * whatever was already there only lands on SPI if something else set
     * the pins first, which made touch_init() silently depend on lcdInit()
     * having run before it. */
    PINSEL0 = (PINSEL0 & ~(3u << 30)) | (3u << 30);
    PINSEL1 = (PINSEL1 & ~(3u <<  2)) | (3u <<  2);
    PINSEL1 = (PINSEL1 & ~(3u <<  4)) | (3u <<  4);

    /* Chip select is plain GPIO, idle high. */
    PINSEL1 &= ~(3u << 8);            /* P0.20 = GPIO                      */
    FIO0DIR |= CS_PIN;
    FIO0SET  = CS_PIN;

    S0SPCR  = 0x093Cu;                /* master, 8 bits, CPOL = CPHA = 1   */
    S0SPCCR = 0x24u;                  /* SCLK = PCLK / 36 = 2 MHz          */
}

unsigned int touch_pressed(void)
{
    return (sample(CMD_READ_Z1) > TOUCH_Z1_MIN) ? 1u : 0u;
}

unsigned int touch_get_point(unsigned short *x, unsigned short *y)
{
    unsigned char raw_x, raw_y;
    unsigned short sx, sy;

    if (!touch_pressed())
    {
        return 0u;
    }

    raw_y = sample(CMD_READ_Y);
    raw_x = sample(CMD_READ_X);

    /* Pressure can collapse between the Z1 check and here if the finger is
     * already lifting; that would give a wild coordinate, so check again. */
    if (!touch_pressed())
    {
        return 0u;
    }

    sx = map_axis(raw_x, TOUCH_X_MIN, TOUCH_X_MAX, SCREEN_W, TOUCH_INVERT_X);
    sy = map_axis(raw_y, TOUCH_Y_MIN, TOUCH_Y_MAX, SCREEN_H, TOUCH_INVERT_Y);

#if TOUCH_SWAP_XY
    *x = (unsigned short)(((unsigned int)sy * SCREEN_W) / SCREEN_H);
    *y = (unsigned short)(((unsigned int)sx * SCREEN_H) / SCREEN_W);
#else
    *x = sx;
    *y = sy;
#endif

    return 1u;
}

void touch_read_raw(unsigned char *x,
                    unsigned char *y,
                    unsigned char *z1,
                    unsigned char *z2)
{
    *y  = sample(CMD_READ_Y);
    *x  = sample(CMD_READ_X);
    *z1 = sample(CMD_READ_Z1);
    *z2 = sample(CMD_READ_Z2);
}
