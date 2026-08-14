#include "lpc24xx.h"
#include "clock.h"

// Current time
static unsigned int hours = 0;
static unsigned int minutes = 0;
static unsigned int seconds = 0;

// Used to check how much time has passed
static unsigned int last_tick = 0;

void clock_init(void)
{
    // Start the clock from 00:00:00
    hours = 0;
    minutes = 0;
    seconds = 0;
	
    /* Timer0, not Timer1: NyanSim only implements Timer0. Free-running
       microsecond counter, shared with speaker.c's udelay(). */
    T0TCR = 0x02;                     /* hold in reset while configuring */
    T0PR  = (Fpclk / 1000000) - 1;    /* one count per microsecond       */
    T0TCR = 0x00;
    T0TCR = 0x01;                     /* run                             */

    last_tick = T0TC;
}

void clock_update(void)
{
    unsigned int current_tick;

    // Read the current Timer1 count
    current_tick = T0TC;

    // 1000 counts = 1 second
    while ((current_tick - last_tick) >= 1000)
    {
        last_tick += 1000;
        seconds++;

        // Change 60 seconds into 1 minute
        if (seconds >= 60)
        {
            seconds = 0;
            minutes++;
        }

        // Change 60 minutes into 1 hour
        if (minutes >= 60)
        {
            minutes = 0;
            hours++;
        }

        // Reset after 24 hours
        if (hours >= 24)
        {
            hours = 0;
        }
    }
}

void clock_set_time(unsigned int h,
                    unsigned int m,
                    unsigned int s)
{
    // Only accept a valid time
    if (h < 24 && m < 60 && s < 60)
    {
        hours = h;
        minutes = m;
        seconds = s;
    }
}
        


// Return seconds from midnight

unsigned int clock_now(void)

{

    return (hours * 3600) + (minutes * 60) + seconds;

}

        
unsigned int clock_get_hour(void)
{
    return hours;
}

unsigned int clock_get_minute(void)
{
    return minutes;
}

unsigned int clock_get_second(void)
{
    return seconds;
}
