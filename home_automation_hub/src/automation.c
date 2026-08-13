#include "automation.h"

// Functions supplied by other team sections
extern void blind_set(int which, int position);
extern unsigned int clock_now(void);

// Shared automation state
AutomationState automation_state;

// Set safe/default values when the system starts.
void automation_init(void)
{
    automation_state.light = 0;

    automation_state.time = 0;

    automation_state.blind1_state = UP;

    automation_state.blind2_state = UP;

    automation_state.plug_state = PLUG_OFF;
}

/* =========================================================
 * automation_update()
 * =========================================================
 *
 * Called once during every main loop.
 *
 * Input:
 *
 * light = light value already read by main.c
 *
 * Steps:
 *
 * 1. Store light value
 * 2. Read current time
 * 3. Call decision.s
 * 4. Decode result
 * 5. Store blind and plug states
 * 6. Send blind state to S1
 */






