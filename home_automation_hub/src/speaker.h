#ifndef SPEAKER_H
#define SPEAKER_H

// Set up the speaker
void speaker_init(void);

// Play the doorbell sound
void chime(void);

// Optional sound for the light sensor demo
void speaker_chirp(unsigned int light);

#endif
