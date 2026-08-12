#include "lpc24xx.h"
#include "speaker.h"
#include "songs.h"

#define DOORBELL_NOTES 6
#define DOORBELL_RATE  100000


// Set up P0.26 as DAC output
static void setup_DAC(void)
{
    // Clear bits 21:20
    PINSEL1 &= ~(3 << 20);

    // Set P0.26 to AOUT (10)
    PINSEL1 |= (2 << 20);

    // Start with speaker off
    DACR = 0;
}


// Delay in microseconds using Timer0
static void udelay(unsigned int delay_in_us)
{
    // Reset Timer0
    T0TCR = 0x02;

    // Make Timer0 count every 1 us
    T0PR = (Fpclk / 1000000) - 1;

    // Finish reset
    T0TCR = 0x00;

    // Start Timer0
    T0TCR = 0x01;

    // Wait for the required time
    while (T0TC < delay_in_us)
    {
    }

    // Stop Timer0
    T0TCR = 0x00;
}


// Play one tone
static void play_tone(unsigned int duration,
                      int period,
                      int volume)
{
    unsigned int time_played = 0;

    // Volume 0 means silence
    if (volume == 0)
    {
        DACR = 0;
        udelay(duration);
        return;
    }

    // Avoid invalid periods
    if (period <= 0)
    {
        return;
    }

    // Make a square wave
    while (time_played < duration)
    {
        // Speaker high
        DACR = ((unsigned int)volume << 6);
        udelay(period / 2);

        // Speaker low
        DACR = 0;
        udelay(period / 2);

        time_played += period;
    }

    // Turn speaker off after the tone
    DACR = 0;
}


// Set up speaker hardware
void speaker_init(void)
{
    setup_DAC();
}


// Play a short part of songs.c as the doorbell
void speaker_doorbell(void)
{
    int i;

    for (i = 0; i < DOORBELL_NOTES; i++)
    {
        play_tone(
            song_data[i].duration * DOORBELL_RATE,
            song_data[i].pitch,
            song_data[i].volume
        );
    }
}
