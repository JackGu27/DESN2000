#ifndef LED_H
#define LED_H

#define UP    0
#define MID   1
#define DOWN  2

void led_init(void);
void blind_set(unsigned int which, unsigned int state);

#endif
