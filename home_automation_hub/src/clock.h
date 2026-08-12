#ifndef CLOCK_H
#define CLOCK_H

void clock_init(void);
void clock_update(void);

unsigned int clock_get_hour(void);
unsigned int clock_get_minute(void);
unsigned int clock_get_second(void);

#endif
