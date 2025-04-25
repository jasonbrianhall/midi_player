// DJGPP MIDI Player with FM Synthesis
// Header file for midiplayer.c
// Contains function declarations and data structures

#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <time.h>

// OPL3 I/O ports (may need to be adjusted based on card configuration)
#define OPL_PORT        0x388   // FM synthesis port

// MIDI constants
#define MAX_TRACKS      100
#define MAX_FILENAME    256
#define BUFFER_SIZE     8192

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

// Constants for play mode
#define PLAY_MODE_TICKS     255 * 64
#define PLAY_BUFFER_LEN     2

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

// Global variables - declared as extern to be defined in the C file
extern int globalVolume;               // Global volume control (0-100)
extern bool enableNormalization;       // Enable volume normalization
extern FMInstrument adl[181];          // FM instrument data
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
extern bool began;                     // Playback began flag
extern int txtline;                    // Text line for display

// Function prototypes
void initFMInstruments();
void outOPL(int port, int reg, int val);
void OPL_SetupParams(int c, int& p, int& q, int& o);
void OPL_NoteOff(int c);
void OPL_NoteOn(int c, double hertz);
void OPL_Touch(int c, int v);
void OPL_Touch_Real(int c, int v);
void OPL_Patch(int c);
void OPL_Pan(int c);
void OPL_Reset();
void OPL_Silence();
int readString(FILE* f, int len, char* str);
unsigned long readVarLen(FILE* f);
unsigned long convertInteger(char* str, int len);
bool loadMidiFile(const char* filename);
void processEvents();
void handleMidiEvent(int tk);
void deallocateActiveNote(int m, int n);
void handleTimer();
void playMidiFile(const char* filename);
void updateAllNotes();
void increaseVolume();
void decreaseVolume();
void toggleNormalization();

#endif // MIDIPLAYER_H
