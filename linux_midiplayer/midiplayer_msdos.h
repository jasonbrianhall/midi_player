#ifndef MIDIPLAYER_MSDOS_H
#define MIDIPLAYER_MSDOS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dpmi.h>

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
#define SAMPLE_RATE     22050   // Lower than Linux version for DOS compatibility
#define AUDIO_CHANNELS  2

// FM Instrument data structure - define this BEFORE the extern declaration
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
extern volatile int dma_buffer_half;

// Function prototypes - Sound Blaster specific
void sb_dsp_write(unsigned char value);
unsigned char sb_dsp_read();
int detect_sound_blaster();
int allocate_dma_buffer();
void setup_dma(unsigned char channel, unsigned char *buffer, unsigned int length);
void start_sb_playback(int sample_rate);
void setup_irq();
void setup_timer();
int init_sound_blaster();
void fill_audio_buffer();

// Function prototypes - MIDI player
int init_dos();
void cleanup();
bool loadMidiFile(const char* filename);
void playMidiFile();
void processEvents();
void handleMidiEvent(int tk);
void updateVolume(int change);
void toggleNormalization();

// Function prototypes - Utility
unsigned long readVarLen(FILE* f);
int readString(FILE* f, int len, char* str);
unsigned long convertInteger(char* str, int len);

#endif // MIDIPLAYER_MSDOS_H
