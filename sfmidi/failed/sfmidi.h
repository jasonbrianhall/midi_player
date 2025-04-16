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

// Sound Blaster DSP commands
#define SB_DSP_RESET        0x06
#define SB_DSP_SPEAKER_ON   0xD1
#define SB_DSP_SPEAKER_OFF  0xD3
#define SB_DSP_GET_VERSION  0xE1
#define SB_DSP_DMA_8BIT     0x14
#define SB_DSP_DMA_16BIT    0xB0
#define SB_DSP_DMA_PAUSE    0xD0
#define SB_DSP_DMA_CONTINUE 0xD4
#define SB_DSP_DMA_STOP     0xD0

// Playback constants
#define DMA_BUFFER_SIZE     32768
#define SB_SAMPLE_RATE      44100

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

extern int globalVolume;               // Global volume control (0-100)
extern bool enableNormalization;       // Enable volume normalization
extern int chins[18];                  // Channel instruments
extern int chpan[18];                  // Channel panning
extern int chpit[18];                  // Channel pitch
extern int tkPtr[MAX_TRACKS];          // File position for each track
extern double tkDelay[MAX_TRACKS];     // Delay for next event
extern int tkStatus[MAX_TRACKS];       // Running status
extern double playwait;                // Wait time for playback
extern int loPtr[MAX_TRACKS];          // File position for loop
extern double loDelay[MAX_TRACKS];     // Delay for loop
extern int loStatus[MAX_TRACKS];       // Status at loop
extern double loopwait;                // Wait time for loop
extern int rbPtr[MAX_TRACKS];          // Rollback position
extern double rbDelay[MAX_TRACKS];     // Rollback delay
extern int rbStatus[MAX_TRACKS];       // Rollback status
extern int ChPatch[16];                // Program/patch for each channel
extern double ChBend[16];              // Pitch bend
extern int ChVolume[16];               // Volume
extern int ChPanning[16];              // Panning
extern int ChVibrato[16];              // Vibrato depth
extern int ActCount[16];               // Number of active notes per channel
extern int ActTone[16][128];           // Original note to simulated note
extern int ActAdlChn[16][128];         // Original note to adlib channel
extern int ActVol[16][128];            // Original note to pressure
extern int ActRev[16][128];            // Original note to active index
extern int ActList[16][100];           // Active index to original note
extern int chon[18];                   // Channel on flag
extern double chage[18];               // Channel age (for allocation)
extern int chm[18], cha[18];           // Channel MIDI and active info
extern int chx[18], chc[18];           // Channel x position and color
extern FILE* midiFile;                 // MIDI file pointer
extern bool isPlaying;                 // Playback status
extern bool paused;                    // Pause status
extern bool loopStart;                 // Loop start flag
extern bool loopEnd;                   // Loop end flag
extern int TrackCount;                 // Number of tracks
extern int DeltaTicks;                 // Delta ticks
extern double InvDeltaTicks;           // Inverse delta ticks
extern double Tempo;                   // Current tempo
extern double bendsense;               // Bend sensitivity
extern int txtline;                    // Text line for display


// Chunk header structure for parsing file chunks
typedef struct {
    char id[4];       // 4-character chunk identifier
    unsigned long size;  // Size of the chunk data
} ChunkHeader;

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
