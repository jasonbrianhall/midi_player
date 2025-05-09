#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include <stdio.h>
#include <stdbool.h>
#include "sdl_minimal.h"  /* Our minimal SDL implementation */
#include "virtual_mixer.h"
#include "dos_utils.h"    /* DOS-specific utilities */

/* DJGPP already has kbhit() so we don't redefine it */
/* Use DOS-specific delay instead of usleep */
#include <time.h>
#define usleep(x) SDL_Delay((x)/1000)

/* MIDI constants */
#define MAX_TRACKS      100
#define MAX_FILENAME    256

/* MIDI event types */
#define NOTE_OFF        0x80
#define NOTE_ON         0x90
#define POLY_PRESSURE   0xA0
#define CONTROL_CHANGE  0xB0
#define PROGRAM_CHANGE  0xC0
#define CHAN_PRESSURE   0xD0
#define PITCH_BEND      0xE0
#define SYSTEM_MESSAGE  0xF0
#define META_EVENT      0xFF

/* MIDI meta event types */
#define META_END_OF_TRACK   0x2F
#define META_TEMPO          0x51
#define META_TEXT           0x01

/* Audio settings */
#define SAMPLE_RATE     44100
#define AUDIO_CHANNELS  2
#define AUDIO_BUFFER    1024

/* FM Instrument data structure - define this BEFORE the extern declaration */
struct FMInstrument {
    unsigned char modChar1;
    unsigned char carChar1;
    unsigned char modChar2;
    unsigned char carChar2;
    unsigned char modChar3;
    unsigned char carChar3;
    unsigned char modChar4;
    unsigned char carChar4;
    unsigned char modChar5;
    unsigned char carChar5;
    unsigned char fbConn;
    unsigned char percNote;
};

/* Global variable declarations */
extern struct FMInstrument adl[181];
extern VirtualMixer* g_midi_mixer;
extern int g_midi_mixer_channel;
extern bool isPlaying;
extern double playwait;

/* Function prototypes */
void initFMInstruments();
bool initSDL();
void cleanup();
bool loadMidiFile(const char* filename);
void playMidiFile();
void handleEvents();
void updateVolume(int change);
void toggleNormalization();
void generateAudio(void* userdata, Uint8* stream, int len);
void processEvents(void);

#endif /* MIDIPLAYER_H */
