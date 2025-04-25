#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// MIDI constants
#define MAX_TRACKS      100
#define MAX_FILENAME    256

// MIDI event types
#define NOTE_OFF        0x80
#define NOTE_ON         0x90
#define POLY_PRESSURE   0xA0
#define CONTROL_CHANGE  0xB0
#define PROGRAM_CHANGE  0xC0
#define CHAN_PRESSURE   0xD0
#define PITCH_BEND      0xE0
#define SYSTEM_MESSAGE  0xF0
#define META_EVENT      0xFF

// MIDI meta event types
#define META_END_OF_TRACK   0x2F
#define META_TEMPO          0x51
#define META_TEXT           0x01

// Audio settings
#define SAMPLE_RATE     44100
#define AUDIO_CHANNELS  2
#define AUDIO_BUFFER    1024

// FM Instrument data structure
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

// Global variable declarations
extern struct FMInstrument adl[181];
extern int globalVolume;

// Function prototypes
void initFMInstruments(void);
bool initSB(void);
void cleanup(void);
bool loadMidiFile(const char* filename);
void playMidiFile(void);
void updateVolume(int change);
void toggleNormalization(void);
void sbAudioCallback(void* userdata, uint8_t* stream, int len);
uint32_t getTickCount(void);

#endif // MIDIPLAYER_H
