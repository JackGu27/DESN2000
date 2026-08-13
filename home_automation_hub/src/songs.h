#ifndef SONGS_H
#define SONGS_H

// Note datatype
struct tone {
    int duration;
    int pitch;
    int volume;
};

// Song data stored in songs.c
extern struct tone song_data[];
extern int song_duration;

#endif
