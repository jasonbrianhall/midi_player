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

// Sound Blaster constants
#define SB_DEFAULT_PORT     0x220
#define SB_DEFAULT_IRQ      5
#define SB_DEFAULT_DMA      1
#define SB_DEFAULT_HDMA     5   // High DMA for 16-bit samples

// Maximum number of instruments and samples to support
#define MAX_PRESETS 1024
#define MAX_INSTRUMENTS 4096
#define MAX_SAMPLES 8192

// MIDI/Player constants
#define MAX_TRACKS      100
#define MAX_FILENAME    256
#define BUFFER_SIZE     8192

// Simplified sample data structure
typedef struct {
    char name[20];
    unsigned long start;      // Start offset in sample data
    unsigned long end;        // End offset in sample data
    unsigned long loopStart;  // Loop start point
    unsigned long loopEnd;    // Loop end point
    unsigned long sampleRate; // Sample rate in Hz
    unsigned char originalPitch; // Original MIDI key number
    char pitchCorrection;     // Pitch correction in cents
    unsigned short sampleLink;// Stereo link
    unsigned short sampleType;// 1=mono, 2=right, 4=left
    short* sampleData;        // Pointer to loaded sample data
    int sampleLength;         // Length of sample in frames
} SF2Sample;

// Simplified instrument data structure
typedef struct {
    char name[20];
    int sampleIndex;        // Index of first sample for this instrument
    int sampleCount;        // Number of samples for this instrument
    int keyRangeStart[128]; // First key in range for each sample
    int keyRangeEnd[128];   // Last key in range for each sample
    int sampleIndex2[128];  // Sample index for each key range
    int velRangeStart[128]; // First velocity in range for each sample
    int velRangeEnd[128];   // Last velocity in range for each sample
} SF2Instrument;

// Simplified preset data structure
typedef struct {
    char name[20];
    unsigned short preset;   // MIDI program number
    unsigned short bank;     // MIDI bank number
    int instrumentIndex;     // Index to instrument
} SF2Preset;

// FM Instrument data structure (for fallback)
typedef struct {
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
} FMInstrument;

// Active note structure
typedef struct {
    int midiChannel;         // MIDI channel
    int note;                // MIDI note number
    int velocity;            // Note velocity
    SF2Sample* sample;       // Sample being played
    double playbackRate;     // Playback rate for pitch adjustment
    double currentPos;       // Current position in sample
    int isActive;            // Whether note is active
    double volumeScale;      // Volume scaling factor
    int adlibChannel;        // OPL3 channel for FM fallback
} ActiveNote;

// Global variables declarations
extern SF2Sample g_samples[MAX_SAMPLES];
extern SF2Instrument g_instruments[MAX_INSTRUMENTS];
extern SF2Preset g_presets[MAX_PRESETS];
extern FMInstrument g_fmInstruments[181];
extern ActiveNote g_activeNotes[32];

extern int g_sampleCount;
extern int g_instrumentCount;
extern int g_presetCount;
extern unsigned long g_sampleDataOffset;

// Sound Blaster variables
extern int sb_port;
extern int sb_irq;
extern int sb_dma;
extern int sb_hdma;
extern int sb_version;
extern short* dma_buffer;
extern int current_buffer;
extern int sb_initialized;

// MIDI playback variables
extern FILE* midiFile;
extern FILE* sf2File;
extern int isPlaying;
extern int paused;
extern int loopStart;
extern int loopEnd;
extern double playwait;
extern int txtline;
extern int TrackCount;
extern int DeltaTicks;
extern double InvDeltaTicks;
extern double Tempo;
extern double bendsense;
extern int began;
extern int globalVolume;
extern int enableSamplePlayback;

// Track variables
extern int tkPtr[MAX_TRACKS];
extern double tkDelay[MAX_TRACKS];
extern int tkStatus[MAX_TRACKS];
extern int loPtr[MAX_TRACKS];
extern double loDelay[MAX_TRACKS];
extern int loStatus[MAX_TRACKS];
extern int rbPtr[MAX_TRACKS];
extern double rbDelay[MAX_TRACKS];
extern int rbStatus[MAX_TRACKS];

// Channel state
extern int ChPatch[16];
extern double ChBend[16];
extern int ChVolume[16];
extern int ChPanning[16];
extern int ChVibrato[16];

// Function prototypes
void initFMInstruments();

#endif // SFMIDI_H
