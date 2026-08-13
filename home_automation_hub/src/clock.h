#ifndef CLOCK_H
#define CLOCK_H

// Start the software clock
void clock_init(void);

// Update the software clock
void clock_update(void);

// Set the current time
void clock_set_time(unsigned int hour,
                    unsigned int minute,
                    unsigned int second);

// Return current time in seconds from midnight
unsigned int clock_now(void);

// Get individual time values
unsigned int clock_get_hour(void);
unsigned int clock_get_minute(void);
unsigned int clock_get_second(void);

#endif
