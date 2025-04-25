#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <signal.h>
#include <dos.h>
#include <conio.h>
#include "midiplayer.h"
#include "dbopl_wrapper.h"

// Removed pthread-specific variables
volatile sig_atomic_t keep_running = 1;
static bool audioMutexLocked = false;

#ifndef M_PI
#define M_PI 3.1415926154
#endif

// Global variables
struct FMInstrument adl[181];
int globalVolume = 100;
bool enableNormalization = true;
bool isPlaying = false;
bool paused = false;

// MIDI file state
FILE* midiFile = NULL;
int TrackCount = 0;
int DeltaTicks = 0;
double Tempo = 500000;  // Default 120 BPM
double playTime = 0;

// Track state
int tkPtr[MAX_TRACKS] = {0};
double tkDelay[MAX_TRACKS] = {0};
int tkStatus[MAX_TRACKS] = {0};
bool loopStart = false;
bool loopEnd = false;
int loPtr[MAX_TRACKS] = {0};
double loDelay[MAX_TRACKS] = {0};
int loStatus[MAX_TRACKS] = {0};
double loopwait = 0;
int rbPtr[MAX_TRACKS] = {0};
double rbDelay[MAX_TRACKS] = {0};
int rbStatus[MAX_TRACKS] = {0};
double playwait = 0;

// MIDI channel state
int ChPatch[16] = {0};
double ChBend[16] = {0};
int ChVolume[16] = {127};
int ChPanning[16] = {0};
int ChVibrato[16] = {0};

// DOS-specific simple mutex
void audioMutexLock() {
    while (audioMutexLocked) {
        // Busy wait
    }
    audioMutexLocked = true;
}

void audioMutexUnlock() {
    audioMutexLocked = false;
}

// DOS-specific kbhit replacement (already in dos.h)
// int kbhit() is already provided by dos.h

// Initialize audio (simplified for DOS)
bool initSDL() {
    // Initialize the OPL emulator
    OPL_Init(SAMPLE_RATE);
    
    // Initialize the FM instruments
    OPL_LoadInstruments();
    
    return true;
}

// Cleanup when shutting down
void cleanup() {
    // Cleanup OPL
    OPL_Shutdown();
    
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
}

// Read existing functions from previous implementation
// (readVarLen, readString, convertInteger, loadMidiFile, handleMidiEvent, 
//  processEvents functions remain the same)

// Simplified audio generation for DOS
void generateAudio(void* userdata, unsigned char* stream, int len) {
    // Clear buffer
    memset(stream, 0, len);
    
    if (!isPlaying || paused) {
        return;
    }
    
    audioMutexLock();
    
    // Generate OPL audio
    OPL_Generate((int16_t*)stream, len / (sizeof(int16_t) * AUDIO_CHANNELS));
    
    // Update playback time
    playTime += len / (double)(SAMPLE_RATE * sizeof(int16_t) * AUDIO_CHANNELS);
    
    // Process MIDI events based on timing
    playwait -= len / (double)(SAMPLE_RATE * sizeof(int16_t) * AUDIO_CHANNELS);
    while (playwait <= 0.1 && isPlaying) {
        processEvents();
    }
    
    audioMutexUnlock();
}

// Signal handler for DOS
void handle_sigint(int sig) {
    keep_running = 0;
    
    // Stop audio and perform cleanup
    isPlaying = false;
    
    printf("\nPlayback interrupted. Cleaning up...\n");
    cleanup();
    
    // Exit the program
    exit(0);
}

// Simplified playMidiFile for DOS
void playMidiFile() {
    // Initialize variables for all channels
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 64;
        ChVibrato[i] = 0;
    }
    
    // Reset playback state
    playTime = 0;
    isPlaying = true;
    paused = false;
    loopStart = false;
    loopEnd = false;
    playwait = 0;
    loopwait = 0;
    
    // Reset all OPL channels
    OPL_Reset();
    
    printf("Playback started. Press:\n");
    printf("  q - Quit\n");
    printf("  Space - Pause/Resume\n");
    printf("  +/- - Increase/Decrease Volume\n");
    printf("  n - Toggle Volume Normalization\n");
    printf("  Ctrl+C - Stop Playback\n");
    
    // Set up signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_sigint);
    
    // Reset keep_running flag
    keep_running = 1;
    
    // Main loop - handle console input
    while (isPlaying && keep_running) {
        // Check for key press without blocking
        if (kbhit()) {
            int ch = getch(); // Use DOS-specific getch
            switch (ch) {
                case ' ':
                    paused = !paused;
                    printf("%s\n", paused ? "Paused" : "Resumed");
                    break;
                case 'q':
                    isPlaying = false;
                    break;
                case '+':
                case '=':
                    updateVolume(10);
                    break;
                case '-':
                    updateVolume(-10);
                    break;
                case 'n':
                    toggleNormalization();
                    break;
            }
        }
        
        // Prevent CPU hogging with DOS delay
        delay(10); // 10 milliseconds
    }
    
    // Stop audio
    printf("Playback ended.\n");
}

// Update global volume
void updateVolume(int change) {
    audioMutexLock();
    globalVolume += change;
    if (globalVolume < 0) {
         globalVolume = 0;
    }
    else if (globalVolume > 5000){ 
         globalVolume = 5000;
    }
    else
    {
        if (globalVolume==0)
        {
            printf("Volume muted\n");
        } else {
             printf("Volume: %d%%\n", globalVolume);
        }
    }
    
    audioMutexUnlock();
}

// Toggle volume normalization
void toggleNormalization() {
    audioMutexLock();
    
    enableNormalization = !enableNormalization;
    
    audioMutexUnlock();
    
    printf("Normalization: %s\n", enableNormalization ? "ON" : "OFF");
}
