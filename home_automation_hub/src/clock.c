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
    last_tick = 0;

    // Reset Timer1
    T1TCR = 0x02;

    // Make Timer1 count once every 1 ms
    T1PR = (Fpclk / 1000) - 1;

    // Finish reset
    T1TCR = 0x00;

    // Start Timer1
    T1TCR = 0x01;
}

void clock_update(void)
{
    unsigned int current_tick;

    // Read the current Timer1 count
    current_tick = T1TC;

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
