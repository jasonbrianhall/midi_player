#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include "midiplayer.h"
#include "dbopl_wrapper.h"

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

// SDL Audio
SDL_AudioDeviceID audioDevice;
SDL_AudioSpec audioSpec;
pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

// SDL Audio initialization
bool initSDL() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16;
    want.channels = AUDIO_CHANNELS;
    want.samples = AUDIO_BUFFER;
    want.callback = generateAudio;
    want.userdata = NULL;
    
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &audioSpec, 0);
    if (audioDevice == 0) {
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        return false;
    }
    
    // Initialize the OPL emulator
    OPL_Init(SAMPLE_RATE);
    
    // Initialize the FM instruments
    OPL_LoadInstruments();
    
    return true;
}

// Cleanup when shutting down
void cleanup() {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();
    
    // Cleanup OPL
    OPL_Shutdown();
    
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
}

// Helper: Read variable length value from MIDI file
unsigned long readVarLen(FILE* f) {
    unsigned char c;
    unsigned long value = 0;
    
    if (fread(&c, 1, 1, f) != 1) return 0;
    
    value = c;
    if (c & 0x80) {
        value &= 0x7F;
        do {
            if (fread(&c, 1, 1, f) != 1) return value;
            value = (value << 7) + (c & 0x7F);
        } while (c & 0x80);
    }
    
    return value;
}

// Helper: Read bytes from file
int readString(FILE* f, int len, char* str) {
    return fread(str, 1, len, f);
}

// Helper: Parse big-endian integer
unsigned long convertInteger(char* str, int len) {
    unsigned long value = 0;
    for (int i = 0; i < len; i++) {
        value = value * 256 + (unsigned char)str[i];
    }
    return value;
}

// Load and parse MIDI file
bool loadMidiFile(const char* filename) {
    char buffer[256];
    char id[5] = {0};
    unsigned long headerLength;
    int format;
    
    // Open file
    midiFile = fopen(filename, "rb");
    if (!midiFile) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return false;
    }
    
    // Read MIDI header
    if (readString(midiFile, 4, id) != 4 || strncmp(id, "MThd", 4) != 0) {
        fprintf(stderr, "Error: Not a valid MIDI file\n");
        fclose(midiFile);
        midiFile = NULL;
        return false;
    }
    
    // Read header length
    readString(midiFile, 4, buffer);
    headerLength = convertInteger(buffer, 4);
    if (headerLength != 6) {
        fprintf(stderr, "Error: Invalid MIDI header length\n");
        fclose(midiFile);
        midiFile = NULL;
        return false;
    }
    
    // Read format type
    readString(midiFile, 2, buffer);
    format = (int)convertInteger(buffer, 2);
    
    // Read number of tracks
    readString(midiFile, 2, buffer);
    TrackCount = (int)convertInteger(buffer, 2);
    if (TrackCount > MAX_TRACKS) {
        fprintf(stderr, "Error: Too many tracks in MIDI file\n");
        fclose(midiFile);
        midiFile = NULL;
        return false;
    }
    
    // Read time division
    readString(midiFile, 2, buffer);
    DeltaTicks = (int)convertInteger(buffer, 2);
    
    // Initialize track data
    for (int tk = 0; tk < TrackCount; tk++) {
        // Read track header
        if (readString(midiFile, 4, id) != 4 || strncmp(id, "MTrk", 4) != 0) {
            fprintf(stderr, "Error: Invalid track header\n");
            fclose(midiFile);
            midiFile = NULL;
            return false;
        }
        
        // Read track length
        readString(midiFile, 4, buffer);
        unsigned long trackLength = convertInteger(buffer, 4);
        long pos = ftell(midiFile);
        
        // Read first event delay
        tkDelay[tk] = readVarLen(midiFile);
        tkPtr[tk] = ftell(midiFile);
        
        // Skip to next track
        fseek(midiFile, pos + (long)trackLength, SEEK_SET);
    }
    
    // Reset file position for playback
    fseek(midiFile, 0, SEEK_SET);
    
    printf("MIDI file loaded: %s\n", filename);
    printf("Format: %d, Tracks: %d, Time Division: %d\n", format, TrackCount, DeltaTicks);
    
    return true;
}

// Handle a single MIDI event
void handleMidiEvent(int tk) {
    unsigned char status, data1, data2;
    unsigned char buffer[256];
    unsigned char evtype;
    unsigned long len;
    
    // Get file position
    fseek(midiFile, tkPtr[tk], SEEK_SET);
    
    // Read status byte or use running status
    if (fread(&status, 1, 1, midiFile) != 1) return;
    
    // Check for running status
    if (status < 0x80) {
        fseek(midiFile, tkPtr[tk], SEEK_SET); // Go back one byte
        status = tkStatus[tk]; // Use running status
    } else {
        tkStatus[tk] = status;
    }
    
    int midCh = status & 0x0F;
    
    // Handle different event types
    switch (status & 0xF0) {
        case NOTE_OFF: {
            // Note Off event
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            ChBend[midCh] = 0;
            OPL_NoteOff(midCh, data1);
            break;
        }
        
        case NOTE_ON: {
            // Note On event
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            // Note on with velocity 0 is treated as note off
            if (data2 == 0) {
                ChBend[midCh] = 0;
                OPL_NoteOff(midCh, data1);
                break;
            }
            
            OPL_NoteOn(midCh, data1, data2);
            break;
        }
        
        case CONTROL_CHANGE: {
            // Control Change
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            switch (data1) {
                case 7:  // Channel Volume
                    ChVolume[midCh] = data2;
                    OPL_SetVolume(midCh, data2);
                    break;
                    
                case 10: // Pan
                    ChPanning[midCh] = data2;
                    OPL_SetPan(midCh, data2);
                    break;
                    
                case 123: // All Notes Off
                    // Turn off all notes on this channel
                    for (int note = 0; note < 128; note++) {
                        OPL_NoteOff(midCh, note);
                    }
                    break;
            }
            break;
        }
        
        case PROGRAM_CHANGE: {
            // Program Change
            fread(&data1, 1, 1, midiFile);
            ChPatch[midCh] = data1;
            OPL_ProgramChange(midCh, data1);
            break;
        }
        
        case PITCH_BEND: {
            // Pitch Bend
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            // Combine LSB and MSB into a 14-bit value
            int bend = (data2 << 7) | data1;
            ChBend[midCh] = bend;
            
            OPL_SetPitchBend(midCh, bend);
            break;
        }
        
        case META_EVENT: case SYSTEM_MESSAGE: {
            // Meta events and system exclusive
            if (status == META_EVENT) {
                // Meta event
                fread(&evtype, 1, 1, midiFile);
                len = readVarLen(midiFile);
                
                if (evtype == META_END_OF_TRACK) {
                    tkStatus[tk] = -1;  // Mark track as ended
                    fseek(midiFile, len, SEEK_CUR);  // Skip event data
                } else if (evtype == META_TEMPO) {
                    // Tempo change
                    char tempo[4] = {0};
                    readString(midiFile, (int)len, tempo);
                    unsigned long tempoVal = convertInteger(tempo, (int)len);
                    Tempo = tempoVal;
                } else if (evtype == META_TEXT) {
                    // Text event - check for loop markers
                    char text[256] = {0};
                    readString(midiFile, (int)len, text);
                    
                    if (strcmp(text, "loopStart") == 0) {
                        loopStart = true;
                    } else if (strcmp(text, "loopEnd") == 0) {
                        loopEnd = true;
                    }
                } else {
                    // Skip other meta events
                    fseek(midiFile, len, SEEK_CUR);
                }
            } else {
                // System exclusive - skip
                len = readVarLen(midiFile);
                fseek(midiFile, (long)len, SEEK_CUR);
            }
            break;
        }
    }
    
    // Read next event delay
    unsigned long nextDelay = readVarLen(midiFile);
    tkDelay[tk] += nextDelay;
    
    // Save new file position
    tkPtr[tk] = ftell(midiFile);
}

// Process events for all tracks
void processEvents() {
    // Save rollback info for each track
    for (int tk = 0; tk < TrackCount; tk++) {
        rbPtr[tk] = tkPtr[tk];
        rbDelay[tk] = tkDelay[tk];
        rbStatus[tk] = tkStatus[tk];
        
        // Handle events for tracks that are due
        if (tkStatus[tk] >= 0 && tkDelay[tk] <= 0) {
            handleMidiEvent(tk);
        }
    }
    
// Handle loop points
    if (loopStart) {
        // Save loop beginning point
        for (int tk = 0; tk < TrackCount; tk++) {
            loPtr[tk] = rbPtr[tk];
            loDelay[tk] = rbDelay[tk];
            loStatus[tk] = rbStatus[tk];
        }
        loopwait = playwait;
        loopStart = false;
    } else if (loopEnd) {
        // Return to loop beginning
        for (int tk = 0; tk < TrackCount; tk++) {
            tkPtr[tk] = loPtr[tk];
            tkDelay[tk] = loDelay[tk];
            tkStatus[tk] = loStatus[tk];
        }
        loopEnd = false;
        playwait = loopwait;
    }
    
    // Find the shortest delay from all tracks
    double nextDelay = -1;
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] < 0) continue;
        if (nextDelay == -1 || tkDelay[tk] < nextDelay) {
            nextDelay = tkDelay[tk];
        }
    }
    
    // Check if all tracks are ended
    bool allEnded = true;
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] >= 0) {
            allEnded = false;
            break;
        }
    }
    
    if (allEnded) {
        // Either loop or stop playback
        if (loopwait > 0) {
            // Return to loop beginning
            for (int tk = 0; tk < TrackCount; tk++) {
                tkPtr[tk] = loPtr[tk];
                tkDelay[tk] = loDelay[tk];
                tkStatus[tk] = loStatus[tk];
            }
            playwait = loopwait;
        } else {
            // Stop playback
            isPlaying = false;
            return;
        }
    }
    
    // Update all track delays
    for (int tk = 0; tk < TrackCount; tk++) {
        tkDelay[tk] -= nextDelay;
    }
    
    // Schedule next event
    double t = nextDelay * Tempo / (DeltaTicks * 1000000.0);
    playwait += t;
}

// SDL audio callback function
void generateAudio(void* userdata, Uint8* stream, int len) {
    (void)userdata; // Unused parameter
    
    // Clear buffer
    memset(stream, 0, len);
    
    if (!isPlaying || paused) {
        return;
    }
    
    pthread_mutex_lock(&audioMutex);
    
    // Generate OPL audio
    OPL_Generate((int16_t*)stream, len / (sizeof(int16_t) * AUDIO_CHANNELS));
    
    // Update playback time
    playTime += len / (double)(SAMPLE_RATE * sizeof(int16_t) * AUDIO_CHANNELS);
    
    // Process MIDI events based on timing
    playwait -= len / (double)(SAMPLE_RATE * sizeof(int16_t) * AUDIO_CHANNELS);
    while (playwait <= 0.1 && isPlaying) {
        processEvents();
    }
    
    pthread_mutex_unlock(&audioMutex);
}

// Initialize everything and start playback
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
    
    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
    
    printf("Playback started. Press:\n");
    printf("  q - Quit\n");
    printf("  Space - Pause/Resume\n");
    printf("  +/- - Increase/Decrease Volume\n");
    printf("  n - Toggle Volume Normalization\n");
    
    // Main loop - handle SDL events
    SDL_Event event;
    while (isPlaying) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isPlaying = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        paused = !paused;
                        printf("%s\n", paused ? "Paused" : "Resumed");
                        break;
                    case SDLK_q:
                        isPlaying = false;
                        break;
                    case SDLK_PLUS:
                    case SDLK_EQUALS:
                        updateVolume(10);
                        break;
                    case SDLK_MINUS:
                        updateVolume(-10);
                        break;
                    case SDLK_n:
                        toggleNormalization();
                        break;
                }
            }
        }
        
        // Sleep to prevent CPU hogging
        SDL_Delay(10);
    }
    
    // Stop audio
    SDL_PauseAudioDevice(audioDevice, 1);
    
    printf("Playback ended.\n");
}

// Update global volume
void updateVolume(int change) {
    pthread_mutex_lock(&audioMutex);
    
    globalVolume += change;
    if (globalVolume < 10) globalVolume = 10;
    if (globalVolume > 300) globalVolume = 300;
    
    pthread_mutex_unlock(&audioMutex);
    
    printf("Volume: %d%%\n", globalVolume);
}

// Toggle volume normalization
void toggleNormalization() {
    pthread_mutex_lock(&audioMutex);
    
    enableNormalization = !enableNormalization;
    
    pthread_mutex_unlock(&audioMutex);
    
    printf("Normalization: %s\n", enableNormalization ? "ON" : "OFF");
}
