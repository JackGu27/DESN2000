#include "lpc24xx.h"
#include "speaker.h"
#include "songs.h"

#define DOORBELL_RATE 100000
#define DOORBELL_NOTES 6


// Set up DAC output on P0.26
void speaker_init(void)
{
    // Set P0.26 to AOUT
    PINSEL1 &= ~(0x03 << 20);
    PINSEL1 |=  (0x02 << 20);

    // Start with speaker off
    DACR = 0;
}


// Delay in microseconds using Timer0
static void udelay(unsigned int us)
{
    unsigned int start = T0TC;

    while ((T0TC - start) < us)
    {
    }
}


// Play one tone
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

    // Make a square wave
    while (time_played < duration)
    {
        // High
        DACR = ((unsigned int)(vol & 0x3FF) << 6);
        udelay(period / 2);

        // Low
        DACR = 0;
        udelay(period / 2);

        time_played += period;
    }

    // Make sure speaker is off
    DACR = 0;
}


// Optional light sensor sound
void speaker_chirp(unsigned int light)
{
    unsigned int freq;
    unsigned int half;

    // Brighter light gives a higher pitch
    freq = 200 + (light * 1800) / 4095;

    // Half period in microseconds
    half = (1000000 / freq) / 2;

    DACR = 0x3FF << 6;
    udelay(half);

    DACR = 0;
    udelay(half);
}


// Play a short doorbell sound
void chime(void)
{
    int i;

    // Use the first few notes from songs.c
    for (i = 0; i < DOORBELL_NOTES; i++)
    {
        play_tone(
            song_data[i].duration * DOORBELL_RATE,
            song_data[i].pitch,
            song_data[i].volume
        );
    }
}
