#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "led.h"

// Smart plug states
#define PLUG_OFF  0
#define PLUG_ON   1

/* Manual override of the blinds, set from the touchscreen (Section 4).
 * AUTOMATIC is the default and the state the system starts in: the assembly
 * decision logic drives the blinds. Any other value pins them where the user
 * asked until AUTOMATIC is selected again.
 * The three manual values are deliberately UP/MID/DOWN + 1, so the position
 * is just (override - 1). */
#define OVERRIDE_AUTO  0
#define OVERRIDE_UP    1
#define OVERRIDE_MID   2
#define OVERRIDE_DOWN  3

/* Manual override of the smart plug, also from the touchscreen. Same idea:
 * PLUG_AUTO is the default and follows the preheat schedule. */
#define PLUG_AUTO      0
#define PLUG_MAN_ON    1
#define PLUG_MAN_OFF   2

// Shared automation state
// This stores the latest automation result
typedef struct {

    unsigned int light;
    unsigned int time;

    int blind1_state;
    int blind2_state;
    int plug_state;

    // OVERRIDE_AUTO, or one of OVERRIDE_UP / MID / DOWN
    int override_mode;

    // PLUG_AUTO, PLUG_MAN_ON or PLUG_MAN_OFF
    int plug_override;

} AutomationState;

// Global automation state
extern AutomationState automation_state;

// light = ADC light sensor reading
// time  = minutes after midnight
extern int decision_asm(
    unsigned int light,
    unsigned int time
);

// Initialise automation system
void automation_init(void);

// Run one automation cycle
void automation_update(unsigned int light);

/* Select the blind override. Called by the touchscreen UI; pass
 * OVERRIDE_AUTO to hand control back to the automation. */
void automation_set_override(int mode);

/* Select the smart plug override. PLUG_AUTO returns it to the schedule. */
void automation_set_plug_override(int mode);

#endif
