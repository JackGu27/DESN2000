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

// Called once during every main loop.
void automation_update(unsigned int light)
{
    int result;
    int blind;
    int plug;

// Store light value (input is light)
automation_state.light = light;

// Read current time
automation_state.time = clock_now();

// Call decision.s and ARM Assembly decision logic.
result = decision_asm(automation_state.light, automation_state.time);

// Decode result
// Lowest two bits are about blind, 00 = DOWN; 01 = MID; 10 = UP
blind = result & 0x3;

// Extract smart plug state, Bit 2 contains 0 = OFF, 1 = ON
plug = (result >> 2) & 0x1;

// Store blind and plug states
// Both blinds currently follow the same light decision.
automation_state.blind1_state = blind;
automation_state.blind2_state = blind;
automation_state.plug_state = plug;

// Send blind state to S1
blind_set(BLIND_1, automation_state.blind1_state);
blind_set(BLIND_2, automation_state.blind2_state);

}
