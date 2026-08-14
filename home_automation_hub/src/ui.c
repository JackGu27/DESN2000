/* ==========================================================================
 *  ui.c - Section 4: LCD dashboard for the Home Automation Hub
 *  Board: Keil QVGA Base Board (LPC2478) - 240 x 320 portrait, 16 bpp
 *
 *  WHAT THIS FILE DOES (feature map for the README / tutor)
 *  -------------------------------------------------------
 *    LCD brought up (SDRAM + panel)      ui.c . ui_init
 *    Show ambient light level            ui.c . draw_light   (value + bar)
 *    Show time of day                    ui.c . draw_clock   (HH:MM:SS)
 *    Show both blind positions           ui.c . draw_blind   (colour + text)
 *    Show smart plug state               ui.c . draw_plug
 *    Manual blind override (touch)       ui.c . handle_touch / draw_blind_buttons
 *    Manual plug override (touch)        ui.c . handle_touch / draw_plug_buttons
 *
 *  The dashboard displays state produced by Section 1 (light sensor),
 *  Section 2 (clock) and Section 3 (automation_state). The one thing it
 *  commands are the two manual overrides, through automation_set_override()
 *  and automation_set_plug_override().
 *
 *  The system starts and stays in AUTOMATIC until somebody touches a manual
 *  button, and AUTO hands control straight back. Automatic is never a mode
 *  you have to select - it is what the hub does if the screen is ignored.
 *
 *  DRAWING STRATEGY
 *  ----------------
 *  ui_init() paints the labels and boxes that never change. ui_update()
 *  keeps a copy of the last value drawn for each field and repaints a field
 *  only when its value differs. Repainting the whole screen every loop pass
 *  would flicker badly and waste time the main loop needs elsewhere.
 *
 *  The stock font is 6 x 8 pixels, which is small on a 240 x 320 panel, so
 *  the clock is drawn with draw_char_scaled() - the same font5x7 glyphs with
 *  every pixel expanded into an N x N block.
 * ========================================================================== */

#include "ui.h"

#include "lcd/lcd_grph.h"
#include "lcd/lcd_hw.h"
#include "lcd/lcd_cfg.h"     /* defines lcd_config for this panel */
#include "lcd/sdram.h"
#include "lcd/font5x7.h"

#include "automation.h"      /* automation_state, PLUG_ON / PLUG_OFF */
#include "touch.h"           /* touchscreen controller (SPI0)        */
#include "clock.h"           /* clock_get_hour / minute / second     */
#include "led.h"             /* UP / MID / DOWN                      */

/* --------------------------------------------------------------------------
 *  Colour scheme and layout constants
 *  All coordinates are pixels: x = 0..239 (left to right), y = 0..319 (top
 *  to bottom). Keeping them here means the layout can be nudged in one place.
 * ------------------------------------------------------------------------ */
#define BG              BLACK       /* page background                      */
#define FG              WHITE       /* normal text                          */
#define LABEL_COL       LIGHT_GRAY  /* field labels                         */
#define HEADER_COL      NAVY        /* title bar fill                       */
#define RULE_COL        DARK_GRAY   /* divider lines                        */

#define HEADER_H        26          /* height of the title bar              */

#define CLOCK_X         33          /* big clock: 8 chars x 18 px = 144 wide */
#define CLOCK_Y         32
#define CLOCK_SCALE     3           /* each font pixel -> 3 x 3 block        */

#define LIGHT_LABEL_Y   68
#define LIGHT_VAL_X     150         /* light reading, right of the label     */
#define BAR_X0          14          /* light bar outline                     */
#define BAR_X1          225
#define BAR_Y0          82
#define BAR_Y1          98
#define BAR_MAX         (BAR_X1 - BAR_X0 - 2)   /* usable pixels inside it   */

#define RULE1_Y         108         /* sensors above, fixtures below         */
#define RULE2_Y         200         /* fixtures above, controls below        */

#define ROW_B1_Y        116         /* blind 1 row                           */
#define ROW_B2_Y        144         /* blind 2 row                           */
#define ROW_PLUG_Y      172         /* smart plug row                        */
#define SWATCH_X0       120         /* colour swatch for the rows            */
#define SWATCH_X1       160
#define SWATCH_H        18
#define STATE_TEXT_X    172         /* state word, right of the swatch       */

/* Two rows of touch buttons.
 *   blinds: AUTO | UP | MID | DOWN   4 x 52 px, 5 px gaps, 8 px margins
 *   plug:   AUTO | ON | OFF          3 x 71 px, same margins
 * Both rows are 30 px tall, which is a comfortable finger target. */
#define BLIND_BTN_COUNT 4
#define BLIND_BTN_W     52
#define BLIND_BTN_PITCH 57
#define BLIND_BTN_Y0    218
#define BLIND_BTN_Y1    248

#define PLUG_BTN_COUNT  3
#define PLUG_BTN_W      71
#define PLUG_BTN_PITCH  76
#define PLUG_BTN_Y0     266
#define PLUG_BTN_Y1     296

#define BTN_X0          8           /* left margin, both rows                */
#define BTN_TEXT_DY     11          /* text offset inside a 30 px button     */

#define BTN_ON_COL      NAVY        /* the mode currently in force           */
#define BTN_OFF_COL     BLACK       /* the modes on offer                    */
#define BTN_EDGE_COL    LIGHT_GRAY

/* Light sensor is a 12-bit ADC reading: 0 = darkest, 4095 = brightest. */
#define LIGHT_MAX       4095u

/* --------------------------------------------------------------------------
 *  Last-drawn values. Initialised to impossible values so that the first
 *  ui_update() after ui_init() draws every field once.
 * ------------------------------------------------------------------------ */
static unsigned int prev_secs   = 0xFFFFFFFFu;   /* seconds from midnight   */
static unsigned int prev_light  = 0xFFFFFFFFu;
static unsigned int prev_bar    = 0xFFFFFFFFu;   /* bar length in pixels    */
static int          prev_blind1 = -1;
static int          prev_blind2 = -1;
static int          prev_plug   = -1;
static int          prev_mode   = -1;
static int          prev_pmode  = -1;

/* Touch edge detection: act once when the screen goes from untouched to
 * touched, not on every pass while a finger is held down. */
static unsigned int touch_was_down = 0;

/* --------------------------------------------------------------------------
 *  Small helpers
 * ------------------------------------------------------------------------ */

/* Write an unsigned value into buf as a fixed number of digits, zero padded
 * and NUL terminated (e.g. 42 with digits = 4 gives "0042").
 *
 * A fixed width matters: lcd_putChar draws the character background as well
 * as the glyph, so a string of constant length always paints over whatever
 * was there before. A shrinking string would leave stale digits on screen.
 *
 * Doing it by hand rather than with sprintf keeps printf out of the image -
 * it costs several KB of flash and a lot of stack for a job this small. */
static void u2str(unsigned int value, unsigned char *buf, unsigned int digits)
{
    unsigned int i;

    buf[digits] = '\0';

    for (i = digits; i > 0; i--)
    {
        buf[i - 1] = (unsigned char)('0' + (value % 10u));
        value /= 10u;
    }
}

/* Draw one font5x7 character with every pixel blown up into a scale x scale
 * block, so the clock is readable from across the demo bench.
 *
 * font5x7[ch][row] holds 8 rows; the 6 columns live in bits 7..2 of each row
 * (the same masks lcd_putChar uses). Background pixels are painted too, so a
 * redraw cleanly replaces the previous character. */
static void draw_char_scaled(unsigned short x,
                             unsigned short y,
                             unsigned char ch,
                             unsigned int scale,
                             lcd_color_t fg,
                             lcd_color_t bg)
{
    unsigned int row, col;
    unsigned char bits;
    lcd_color_t colour;

    /* The font table starts at space (0x20); anything outside it is blank. */
    if ((ch < 0x20) || (ch > 0x7F))
    {
        ch = 0x20;
    }
    ch -= 0x20;

    for (row = 0; row < 8; row++)
    {
        bits = font5x7[ch][row];

        for (col = 0; col < 6; col++)
        {
            colour = ((bits & (0x80u >> col)) != 0) ? fg : bg;

            lcd_fillRect((unsigned short)(x + col * scale),
                         (unsigned short)(y + row * scale),
                         (unsigned short)(x + col * scale + scale - 1),
                         (unsigned short)(y + row * scale + scale - 1),
                         colour);
        }
    }
}

/* Draw a whole string with draw_char_scaled(). Characters are 6 px wide in
 * the font, so each one advances 6 * scale pixels. */
static void draw_string_scaled(unsigned short x,
                               unsigned short y,
                               unsigned char *str,
                               unsigned int scale,
                               lcd_color_t fg,
                               lcd_color_t bg)
{
    while (*str != '\0')
    {
        draw_char_scaled(x, y, *str++, scale, fg, bg);
        x = (unsigned short)(x + 6 * scale);
    }
}

/* Convenience wrapper: set the font colours, then draw normal 6x8 text. */
static void put_text(unsigned short x,
                     unsigned short y,
                     unsigned char *str,
                     lcd_color_t fg,
                     lcd_color_t bg)
{
    lcd_fontColor(fg, bg);
    lcd_putString(x, y, str);
}

/* Map a blind position onto the colour its tricolor LED shows, so the screen
 * and the fixture agree at a glance.
 * ProjectBrief Activity 2: rolled up = red, mid-way = green, down = blue. */
static lcd_color_t blind_colour(int state)
{
    if (state == UP)   return RED;
    if (state == MID)  return GREEN;
    if (state == DOWN) return BLUE;
    return DARK_GRAY;                    /* unknown / not yet decided */
}

/* The matching word for the same position, padded to 4 characters so a
 * redraw always covers the previous word ("DOWN" is the longest). */
static unsigned char *blind_word(int state)
{
    if (state == UP)   return (unsigned char *)"UP  ";
    if (state == MID)  return (unsigned char *)"MID ";
    if (state == DOWN) return (unsigned char *)"DOWN";
    return (unsigned char *)"??  ";
}

/* --------------------------------------------------------------------------
 *  Field painters - each one draws a single value
 * ------------------------------------------------------------------------ */

/* Big HH:MM:SS clock. */
static void draw_clock(unsigned int h, unsigned int m, unsigned int s)
{
    unsigned char str[9];

    u2str(h, &str[0], 2);
    str[2] = ':';
    u2str(m, &str[3], 2);
    str[5] = ':';
    u2str(s, &str[6], 2);
    str[8] = '\0';

    draw_string_scaled(CLOCK_X, CLOCK_Y, str, CLOCK_SCALE, FG, BG);
}

/* Ambient light: the raw 12-bit reading plus a bar showing it as a
 * proportion of full scale. */
static void draw_light(unsigned int light)
{
    unsigned char str[5];

    if (light > LIGHT_MAX)
    {
        light = LIGHT_MAX;               /* clamp, just in case */
    }

    u2str(light, str, 4);
    put_text(LIGHT_VAL_X, LIGHT_LABEL_Y, str, YELLOW, BG);
}

/* The bar itself. Split from draw_light() because the number changes far
 * more often than the bar length does. */
static void draw_light_bar(unsigned int bar_px)
{
    /* Filled part of the bar. */
    if (bar_px > 0)
    {
        lcd_fillRect(BAR_X0 + 1, BAR_Y0 + 1,
                     (unsigned short)(BAR_X0 + bar_px), BAR_Y1 - 1, YELLOW);
    }

    /* Empty remainder, cleared so the bar can shrink as well as grow. */
    if (bar_px < BAR_MAX)
    {
        lcd_fillRect((unsigned short)(BAR_X0 + 1 + bar_px), BAR_Y0 + 1,
                     BAR_X1 - 1, BAR_Y1 - 1, BG);
    }
}

/* One blind row: colour swatch + position word. */
static void draw_blind(unsigned short y, int state)
{
    lcd_fillRect(SWATCH_X0, y, SWATCH_X1,
                 (unsigned short)(y + SWATCH_H), blind_colour(state));

    put_text(STATE_TEXT_X, (unsigned short)(y + 5), blind_word(state), FG, BG);
}

/* Smart plug row: green when energised, grey when not. */
static void draw_plug(int state)
{
    lcd_color_t colour = (state == PLUG_ON) ? GREEN : DARK_GRAY;

    lcd_fillRect(SWATCH_X0, ROW_PLUG_Y, SWATCH_X1,
                 (unsigned short)(ROW_PLUG_Y + SWATCH_H), colour);

    put_text(STATE_TEXT_X, (unsigned short)(ROW_PLUG_Y + 5),
             (state == PLUG_ON) ? (unsigned char *)"ON " : (unsigned char *)"OFF",
             FG, BG);
}

/* --------------------------------------------------------------------------
 *  Touch buttons
 * ------------------------------------------------------------------------ */

/* Draw one button: filled when it is the mode in force, outlined otherwise,
 * with its label centred (the font is 6 px per character). */
static void draw_button(unsigned short x,
                        unsigned short y0,
                        unsigned short y1,
                        unsigned short w,
                        unsigned char *label,
                        unsigned int active)
{
    lcd_color_t fill = active ? BTN_ON_COL : BTN_OFF_COL;
    unsigned int len = 0;

    while (label[len] != '\0')
    {
        len++;
    }

    lcd_fillRect(x, y0, (unsigned short)(x + w), y1, fill);
    lcd_drawRect(x, y0, (unsigned short)(x + w), y1, BTN_EDGE_COL);

    put_text((unsigned short)(x + ((w - (len * 6)) / 2)),
             (unsigned short)(y0 + BTN_TEXT_DY), label, FG, fill);
}

static unsigned short blind_btn_x(unsigned int i)
{
    return (unsigned short)(BTN_X0 + (i * BLIND_BTN_PITCH));
}

static unsigned short plug_btn_x(unsigned int i)
{
    return (unsigned short)(BTN_X0 + (i * PLUG_BTN_PITCH));
}

static unsigned char *blind_btn_label(unsigned int i)
{
    if (i == OVERRIDE_AUTO) return (unsigned char *)"AUTO";
    if (i == OVERRIDE_UP)   return (unsigned char *)"UP";
    if (i == OVERRIDE_MID)  return (unsigned char *)"MID";
    return (unsigned char *)"DOWN";
}

static unsigned char *plug_btn_label(unsigned int i)
{
    if (i == PLUG_AUTO)    return (unsigned char *)"AUTO";
    if (i == PLUG_MAN_ON)  return (unsigned char *)"ON";
    return (unsigned char *)"OFF";
}

static void draw_blind_buttons(int mode)
{
    unsigned int i;

    for (i = 0; i < BLIND_BTN_COUNT; i++)
    {
        draw_button(blind_btn_x(i), BLIND_BTN_Y0, BLIND_BTN_Y1, BLIND_BTN_W,
                    blind_btn_label(i), ((int)i == mode) ? 1u : 0u);
    }
}

static void draw_plug_buttons(int mode)
{
    unsigned int i;

    for (i = 0; i < PLUG_BTN_COUNT; i++)
    {
        draw_button(plug_btn_x(i), PLUG_BTN_Y0, PLUG_BTN_Y1, PLUG_BTN_W,
                    plug_btn_label(i), ((int)i == mode) ? 1u : 0u);
    }
}

/* Read the touchscreen and act on a new press.
 *
 * touch.c does the hardware work and hands back screen pixels, so all this
 * has to do is edge-detect and hit-test. Acting only on the transition from
 * untouched to touched means holding a finger down fires once. */
static void handle_touch(void)
{
    unsigned short px = 0, py = 0;
    unsigned int i;

    if (!touch_get_point(&px, &py))
    {
        touch_was_down = 0;              /* nothing there - re-arm */
        return;
    }

    if (touch_was_down)
    {
        return;                          /* still the same press   */
    }
    touch_was_down = 1;

    /* Only the two button rows are touch sensitive; everything above them is
     * display, so a stray touch on the clock does nothing. */
    if ((py >= BLIND_BTN_Y0) && (py <= BLIND_BTN_Y1))
    {
        for (i = 0; i < BLIND_BTN_COUNT; i++)
        {
            if ((px >= blind_btn_x(i)) &&
                (px < (unsigned short)(blind_btn_x(i) + BLIND_BTN_W)))
            {
                automation_set_override((int)i);
                return;
            }
        }
    }
    else if ((py >= PLUG_BTN_Y0) && (py <= PLUG_BTN_Y1))
    {
        for (i = 0; i < PLUG_BTN_COUNT; i++)
        {
            if ((px >= plug_btn_x(i)) &&
                (px < (unsigned short)(plug_btn_x(i) + PLUG_BTN_W)))
            {
                automation_set_plug_override((int)i);
                return;
            }
        }
    }
}

/* --------------------------------------------------------------------------
 *  Public functions
 * ------------------------------------------------------------------------ */

/* Bring up the display and paint everything that never changes.
 *
 * ORDER MATTERS: the external SDRAM is the frame buffer, so it has to be
 * running before the LCD controller is pointed at it, and the panel has to
 * be configured before it is switched on. lcd_config comes from lcd_cfg.h
 * and describes this particular 240 x 320 panel. */
void ui_init(void)
{
    sdramInit();                 /* external SDRAM = frame buffer      */
    lcdInit(&lcd_config);        /* configure the LPC2478 LCD controller */
    lcdTurnOn();                 /* backlight + panel on                 */

    lcd_fillScreen(BG);

    /* Title bar. */
    lcd_fillRect(0, 0, DISPLAY_WIDTH - 1, HEADER_H, HEADER_COL);
    put_text(24, 9, (unsigned char *)"HOME AUTOMATION HUB", WHITE, HEADER_COL);

    /* Static labels. */
    put_text(14, LIGHT_LABEL_Y, (unsigned char *)"AMBIENT LIGHT", LABEL_COL, BG);
    put_text(14, (unsigned short)(ROW_B1_Y + 5),   (unsigned char *)"BLIND 1",    LABEL_COL, BG);
    put_text(14, (unsigned short)(ROW_B2_Y + 5),   (unsigned char *)"BLIND 2",    LABEL_COL, BG);
    put_text(14, (unsigned short)(ROW_PLUG_Y + 5), (unsigned char *)"SMART PLUG", LABEL_COL, BG);

    /* Outline of the light bar (drawn once; only the fill changes). */
    lcd_drawRect(BAR_X0, BAR_Y0, BAR_X1, BAR_Y1, LABEL_COL);

    /* Dividers: sensors / fixtures / controls. */
    lcd_line(14, RULE1_Y, 225, RULE1_Y, RULE_COL);
    lcd_line(14, RULE2_Y, 225, RULE2_Y, RULE_COL);

    /* Headings for the two touch rows. */
    put_text(14, BLIND_BTN_Y0 - 12, (unsigned char *)"BLINDS", LABEL_COL, BG);
    put_text(14, PLUG_BTN_Y0  - 12, (unsigned char *)"PLUG",   LABEL_COL, BG);

    /* Touchscreen. This MUST come after lcdInit(): the panel controller and
     * the touch controller share SPI0 on P0.15/17/18, and touch_init() only
     * ORs its pin-function bits in. Called first, it would leave the pins in
     * the wrong mode and every touch would read as noise. */
    touch_init();

    draw_blind_buttons(OVERRIDE_AUTO);
    draw_plug_buttons(PLUG_AUTO);

    /* Force a full repaint of the live fields on the first ui_update(). */
    prev_secs   = 0xFFFFFFFFu;
    prev_light  = 0xFFFFFFFFu;
    prev_bar    = 0xFFFFFFFFu;
    prev_blind1 = -1;
    prev_blind2 = -1;
    prev_plug   = -1;
    prev_mode   = OVERRIDE_AUTO;   /* matches the rows just drawn */
    prev_pmode  = PLUG_AUTO;

    touch_was_down = 0;
}

/* Repaint whatever has changed since the last call. Cheap when nothing has:
 * six comparisons and no LCD traffic at all. */
void ui_update(void)
{
    unsigned int h, m, s, now;
    unsigned int light, bar_px;

    /* ---- touchscreen: may change the override mode ---- */
    handle_touch();

    if (automation_state.override_mode != prev_mode)
    {
        draw_blind_buttons(automation_state.override_mode);
        prev_mode = automation_state.override_mode;
    }

    if (automation_state.plug_override != prev_pmode)
    {
        draw_plug_buttons(automation_state.plug_override);
        prev_pmode = automation_state.plug_override;
    }

    /* ---- time of day (Section 2) ---- */
    h = clock_get_hour();
    m = clock_get_minute();
    s = clock_get_second();
    now = (h * 3600u) + (m * 60u) + s;

    if (now != prev_secs)
    {
        draw_clock(h, m, s);
        prev_secs = now;
    }

    /* ---- ambient light (Section 1, via Section 3's shared state) ---- */
    light = automation_state.light;

    if (light > LIGHT_MAX)
    {
        light = LIGHT_MAX;
    }

    if (light != prev_light)
    {
        draw_light(light);
        prev_light = light;
    }

    /* Redraw the bar only when its length actually changes by a whole pixel,
     * otherwise ADC noise would have us repainting it every single pass. */
    bar_px = (light * BAR_MAX) / LIGHT_MAX;

    if (bar_px != prev_bar)
    {
        draw_light_bar(bar_px);
        prev_bar = bar_px;
    }

    /* ---- fixtures (Section 3's decision, mirrored on screen) ---- */
    if (automation_state.blind1_state != prev_blind1)
    {
        draw_blind(ROW_B1_Y, automation_state.blind1_state);
        prev_blind1 = automation_state.blind1_state;
    }

    if (automation_state.blind2_state != prev_blind2)
    {
        draw_blind(ROW_B2_Y, automation_state.blind2_state);
        prev_blind2 = automation_state.blind2_state;
    }

    if (automation_state.plug_state != prev_plug)
    {
        draw_plug(automation_state.plug_state);
        prev_plug = automation_state.plug_state;
    }
}
