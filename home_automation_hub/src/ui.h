#ifndef UI_H
#define UI_H

/* ==========================================================================
 *  Section 4 - LCD Dashboard (display only)
 *
 *  ui_init()   : brings up the SDRAM frame buffer and the LCD panel, then
 *                paints the static parts of the dashboard (title bar, field
 *                labels, dividers). Call it ONCE at start-up, and call it
 *                BEFORE anything else touches the LCD.
 *
 *  ui_update() : call once per pass of the main super-loop. It reads the
 *                shared state (automation_state + clock) and repaints only
 *                the fields whose value has actually changed, so the screen
 *                does not flicker and the loop stays fast.
 * ========================================================================== */

void ui_init(void);
void ui_update(void);

#endif
