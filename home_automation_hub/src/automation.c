#include "automation.h"
#include "clock.h"
#include "led.h"

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

    // Start under automatic control - the overrides are opt-in
    automation_state.override_mode = OVERRIDE_AUTO;
    automation_state.plug_override = PLUG_AUTO;
}

/* Select the smart plug override (touchscreen, Section 4). */
void automation_set_plug_override(int mode)
{
    if ((mode >= PLUG_AUTO) && (mode <= PLUG_MAN_OFF))
    {
        automation_state.plug_override = mode;
    }
}

/* Select the blind override (touchscreen, Section 4). */
void automation_set_override(int mode)
{
    if ((mode >= OVERRIDE_AUTO) && (mode <= OVERRIDE_DOWN))
    {
        automation_state.override_mode = mode;
    }
}

// Called once during every main loop.
void automation_update(unsigned int light)
{
    int result;
    int blind;
    int plug;

// Store light value (input is light)
automation_state.light = light;

// Read current time
automation_state.time = clock_now() / 60;

// Call decision.s and ARM Assembly decision logic.
result = decision_asm(automation_state.light, automation_state.time);

// Decode result
// Lowest two bits are the blind: 0 = UP, 1 = MID, 2 = DOWN (as led.h)
blind = result & 0x3;

// Extract smart plug state, Bit 2 contains 0 = OFF, 1 = ON
plug = (result >> 2) & 0x1;

/* A manual override replaces the assembly logic's blind decision, but only
 * for the blinds - the smart plug stays on its schedule either way.
 * OVERRIDE_UP/MID/DOWN are UP/MID/DOWN + 1, hence the - 1. */
if (automation_state.override_mode != OVERRIDE_AUTO)
{
    blind = automation_state.override_mode - 1;
}

/* The plug override works the same way: it replaces the schedule's answer
 * until the user selects AUTO again. */
if (automation_state.plug_override != PLUG_AUTO)
{
    plug = (automation_state.plug_override == PLUG_MAN_ON) ? PLUG_ON : PLUG_OFF;
}

// Store blind and plug states
// Both blinds currently follow the same light decision.
automation_state.blind1_state = blind;
automation_state.blind2_state = blind;
automation_state.plug_state = plug;

// Send blind state to S1
blind_set(BLIND_1, automation_state.blind1_state);
blind_set(BLIND_2, automation_state.blind2_state);

}
