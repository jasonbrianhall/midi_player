# sfmidi.h - Header file for the combined SoundFont MIDI Player
#ifndef SFMIDI_H
#define SFMIDI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <dos.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <time.h>

// OPL3 I/O ports (may need to be adjusted based on card configuration)
#define OPL_PORT        0x388   // FM synthesis port

// Maximum number of instruments and samples to support
#define MAX_PRESETS 1024
#define MAX_INSTRUMENTS 4096
#define MAX_SAMPLES 8192

// MIDI/Player constants
#define MAX_TRACKS      100
#define MAX_FILENAME    256
#define BUFFER_SIZE     8192

// [Rest of the existing header content remains the same]

// Global variables declarations
SF2Sample g_samples[MAX_SAMPLES];
SF2Instrument g_instruments[MAX_INSTRUMENTS];
SF2Preset g_presets[MAX_PRESETS];
FMInstrument g_fmInstruments[181];
ActiveNote g_activeNotes[32];

int g_sampleCount = 0;
int g_instrumentCount = 0;
int g_presetCount = 0;
unsigned long g_sampleDataOffset = 0;

// Sound Blaster variables
int sb_port = SB_DEFAULT_PORT;
int sb_irq = SB_DEFAULT_IRQ;
int sb_dma = SB_DEFAULT_DMA;
int sb_hdma = SB_DEFAULT_HDMA;
int sb_version = 0;
short* dma_buffer = NULL;
int current_buffer = 0;
int sb_initialized = 0;

// MIDI playback variables
FILE* midiFile = NULL;
FILE* sf2File = NULL;
int isPlaying = 0;
int paused = 0;
int loopStart = 0;
int loopEnd = 0;
double playwait = 0;
int txtline = 0;
int TrackCount = 0;
int DeltaTicks = 0;
double InvDeltaTicks = 0;
double Tempo = 0;
double bendsense = 0;
int began = 0;
int globalVolume = 100;
int enableSamplePlayback = 1;

// Track variables
int tkPtr[MAX_TRACKS] = {0};
double tkDelay[MAX_TRACKS] = {0};
int tkStatus[MAX_TRACKS] = {0};
int loPtr[MAX_TRACKS] = {0};
double loDelay[MAX_TRACKS] = {0};
int loStatus[MAX_TRACKS] = {0};
int rbPtr[MAX_TRACKS] = {0};
double rbDelay[MAX_TRACKS] = {0};
int rbStatus[MAX_TRACKS] = {0};

// Channel state
int ChPatch[16] = {0};
double ChBend[16] = {0};
int ChVolume[16] = {0};
int ChPanning[16] = {0};
int ChVibrato[16] = {0};

// Main function declaration
int main(int argc, char* argv[]);

#endif // SFMIDI_H
