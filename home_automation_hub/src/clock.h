#ifndef CLOCK_H
#define CLOCK_H

// Start the software clock
void clock_init(void);

// Update the current time
void clock_update(void);

// Set the current time
void clock_set_time(unsigned int h,
                    unsigned int m,
                    unsigned int s);

// Get the current time
unsigned int clock_get_hour(void);
unsigned int clock_get_minute(void);
unsigned int clock_get_second(void);

#endif
