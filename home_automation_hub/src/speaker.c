#include "lpc24xx.h"
#include "speaker.h"
#include "songs.h"

// Length of one duration unit from songs.c
#define DOORBELL_RATE 100000

// Number of notes used for the doorbell
#define DOORBELL_NOTES 6


// Set up the DAC output
static void setup_DAC(void)
{
    // Set P0.26 to AOUT
    PINSEL1 &= ~(3 << 20);
    PINSEL1 |=  (2 << 20);
}


// Delay in microseconds using Timer0
static void udelay(unsigned int delay_in_us)
{
    // Reset Timer0
    T0TCR = 0x02;

    // Make Timer0 count once every 1 us
    T0PR = (Fpclk / 1000000) - 1;

    // Finish reset
    T0TCR = 0x00;

    // Start Timer0
    T0TCR = 0x01;

    // Wait until the delay is finished
    while (T0TC < delay_in_us)
    {
    }

    // Stop Timer0
    T0TCR = 0x00;
}


// Play one tone using the DAC
static void play_tone(unsigned int duration,
                      int period,
                      int vol)
{
    unsigned int time_played = 0;

    // Volume 0 means silence
    if (vol == 0)
    {
        DACR = 0;
        udelay(duration);
        return;
    }

    // Make a square wave for the required time
    while (time_played < duration)
    {
        // Speaker high
        DACR = ((unsigned int)(vol & 0x3FF) << 6);
        udelay(period / 2);

        // Speaker low
        DACR = 0;
        udelay(period / 2);

        time_played += period;
    }

    // Make sure the speaker is off
    DACR = 0;
}


// Set up the speaker hardware
void speaker_init(void)
{
    setup_DAC();

    // Start with no sound
    DACR = 0;
}


// Play a short part of the Lab 5 song as the doorbell
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
