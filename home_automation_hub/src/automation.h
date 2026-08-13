#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "led.h"

// Smart plug states
#define PLUG_OFF  0
#define PLUG_ON   1

// Shared automation state
// This stores the latest automation result
typedef struct {

    unsigned int light;
    unsigned int time;

    int blind1_state;
    int blind2_state;
    int plug_state;

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

#endif
