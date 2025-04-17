#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include "midiplayer.h"

#define M_PI 3.1415926154

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

// Channel state
int chins[18] = {0};  // Instrument for each channel
int chpan[18] = {0};  // Panning
int chpit[18] = {0};  // Pitch
int chon[18] = {0};   // Channel on flag
double chage[18] = {0};  // Channel age for note allocation

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

// Active note tracking
int ActCount[16] = {0};
int ActTone[16][128] = {{0}};
int ActAdlChn[16][128] = {{0}};
int ActVol[16][128] = {{0}};
int ActRev[16][128] = {{0}};
int ActList[16][100] = {{0}};
int chm[18] = {0}, cha[18] = {0};
int chx[18] = {0}, chc[18] = {0};

// SDL Audio
SDL_AudioDeviceID audioDevice;
SDL_AudioSpec audioSpec;
FMSynth synth;
pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

float calculateEnvelope(FMSynth* synth, int ch, int op) {
    int ins = chins[ch];
    
    // Get attack, decay, sustain, release values from instrument
    int attack, decay, sustain, release;
    if (op == 0) {
        // Modulator
        attack = (adl[ins].modChar4 >> 4) & 0x0F;
        decay = adl[ins].modChar4 & 0x0F;
        sustain = (adl[ins].modChar3 >> 4) & 0x0F;
        release = adl[ins].modChar3 & 0x0F;
    } else {
        // Carrier
        attack = (adl[ins].carChar4 >> 4) & 0x0F;
        decay = adl[ins].carChar4 & 0x0F;
        sustain = (adl[ins].carChar3 >> 4) & 0x0F;
        release = adl[ins].carChar3 & 0x0F;
    }
    
    // Convert from OPL parameter to time values
    float attackTime = attack > 0 ? pow(2, -attack * 0.25) : 0.001;
    float decayTime = decay > 0 ? pow(2, -decay * 0.25) * 4.0 : 0.001;
    float sustainLevel = (15 - sustain) / 15.0;
    float releaseTime = release > 0 ? pow(2, -release * 0.25) * 8.0 : 0.001;
    
    // Calculate envelope value based on note age
    double noteAge = chage[ch];
    float envValue;
    
    if (noteAge < attackTime) {
        // Attack phase
        envValue = noteAge / attackTime;
    } else if (noteAge < attackTime + decayTime) {
        // Decay phase
        float decayPhase = (noteAge - attackTime) / decayTime;
        envValue = 1.0 - decayPhase * (1.0 - sustainLevel);
    } else {
        // Sustain phase
        envValue = sustainLevel;
    }
    
    // Apply key scaling and level
    int keyScaleLevel = op == 0 ? (adl[ins].modChar2 >> 6) & 0x03 : (adl[ins].carChar2 >> 6) & 0x03;
    int level = op == 0 ? adl[ins].modChar2 & 0x3F : adl[ins].carChar2 & 0x3F;
    
    // Apply key scaling (higher notes decay faster)
    int note = ActTone[chm[ch]][ActList[chm[ch]][cha[ch]]];
    float keyScaleFactor = 1.0;
    if (keyScaleLevel > 0) {
        keyScaleFactor = 1.0 - (note - 60) * (keyScaleLevel * 0.05) / 127.0;
        keyScaleFactor = fmax(0.1, keyScaleFactor);
    }
    
    envValue *= keyScaleFactor;
    
    // Apply total level attenuation
    float levelAttenuation = (63 - level) / 63.0;
    envValue *= levelAttenuation;
    
    return envValue;
}

// Helper function to get waveform based on waveform select
double getWaveform(double phase, int waveform) {
    switch (waveform & 0x07) {
        case 0: // Sine
            return sin(2.0 * M_PI * phase);
            
        case 1: // Half-sine (positive only)
            return fmax(0.0, sin(2.0 * M_PI * phase));
            
        case 2: // Absolute sine
            return fabs(sin(2.0 * M_PI * phase));
            
        case 3: // Quarter sine (first quarter of sine wave)
            {
                double sinVal = sin(2.0 * M_PI * phase);
                if (phase < 0.25) return sinVal;
                else if (phase < 0.5) return 1.0;
                else if (phase < 0.75) return 0.0;
                else return 0.0;
            }
            
        case 4: // Alternating sine
            return sin(2.0 * M_PI * phase) * (phase < 0.5 ? 1 : -1);
            
        case 5: // Camel sine (abs of alternating sine)
            return fabs(sin(2.0 * M_PI * phase)) * (phase < 0.5 ? 1 : -1);
            
        case 6: // Square
            return phase < 0.5 ? 1.0 : -1.0;
            
        case 7: // Logarithmic sawtooth
            return 1.0 - 2.0 * phase;
    }
    
    return sin(2.0 * M_PI * phase); // Default to sine
}

// FM Synthesis implementation
void initFMSynth(FMSynth* synth, int sampleRate) {
    synth->channels = 18;  // 18 FM channels (OPL3 has 18 channels)
    synth->sampleRate = sampleRate;
    memset(synth->registers, 0, sizeof(synth->registers));
    synth->buffer = (float*)malloc(AUDIO_BUFFER * AUDIO_CHANNELS * sizeof(float));
    
    // Initialize channel state
    for (int i = 0; i < 18; i++) {
        synth->channelState[i].phase[0] = 0.0;
        synth->channelState[i].phase[1] = 0.0;
        synth->channelState[i].env[0] = 0.0;
        synth->channelState[i].env[1] = 0.0;
        synth->channelState[i].freq = 0.0;
        synth->channelState[i].algorithm = 0;
        synth->channelState[i].keyOn = false;
        synth->channelState[i].feedback = 0.0;
        synth->channelState[i].output[0] = 0.0;
        synth->channelState[i].output[1] = 0.0;
    }
}

// SDL Audio initialization
bool initSDL() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32;
    want.channels = AUDIO_CHANNELS;
    want.samples = AUDIO_BUFFER;
    want.callback = generateAudio;
    want.userdata = &synth;
    
    initFMSynth(&synth, SAMPLE_RATE);
    
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &audioSpec, 0);
    if (audioDevice == 0) {
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

// Cleanup when shutting down
void cleanup() {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
    free(synth.buffer);
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

// Simulated OPL functions for FM synthesis
void applyFMRegister(int c, int reg, int val) {
    // Store register value
    synth.registers[reg] = val;
    
    // Update synthesis parameters based on register
    if (reg >= 0xA0 && reg <= 0xA8) {
        // Low 8 bits of frequency
        int ch = reg - 0xA0;
        if (ch == c) { // Make sure we're modifying the correct channel
            int fnum = ((int)synth.channelState[ch].freq & 0x300) | val;

            synth.channelState[ch].freq = fnum;
        }
    }
    else if (reg >= 0xB0 && reg <= 0xB8) {
        // Key on/off bit and high 2 bits of frequency
        int ch = reg - 0xB0;
        if (ch == c) { // Make sure we're modifying the correct channel
            int keyOn = (val & 0x20) != 0;
            int block = (val >> 2) & 7;
            int fnum_hi = val & 3;
            
            synth.channelState[ch].keyOn = keyOn;
            
            // Combined frequency number
            int fnum = ((fnum_hi << 8) | ((int)synth.channelState[ch].freq & 0xFF));
            
            // Calculate actual frequency in Hz
            double baseFreq = 49716.0 / synth.sampleRate;
            synth.channelState[ch].freq = baseFreq * fnum * pow(2, block - 20);
        }
    }
    else if (reg >= 0xC0 && reg <= 0xC8) {
        // Algorithm and feedback
        int ch = reg - 0xC0;
        if (ch == c) { // Make sure we're modifying the correct channel
            synth.channelState[ch].algorithm = val & 1;
            synth.channelState[ch].feedback = (val >> 1) & 7;
            if (synth.channelState[ch].feedback > 0) {
                synth.channelState[ch].feedback = pow(2, synth.channelState[ch].feedback - 1) * 0.15;
            } else {
                synth.channelState[ch].feedback = 0;
            }
        }
    }
    // For a complete implementation, you'd handle more registers here
}

void OPL_NoteOn(int c, double hertz) {
    // Convert frequency to parameters
    int fnum = (int)hertz;
    int block = 0;
    
    // Scale to appropriate range
    while (fnum >= 1024) {
        fnum /= 2;
        block++;
    }
    
    // Update registers - in a real implementation this would be more complex
    int baseReg = c;
    applyFMRegister(c, 0xA0 + baseReg, fnum & 0xFF);
    applyFMRegister(c, 0xB0 + baseReg, ((block & 7) << 2) | ((fnum >> 8) & 3) | 0x20); // Note on bit
    
    // Store key-on state
    chpit[c] = ((block & 7) << 2) | ((fnum >> 8) & 3) | 0x20;
    
    // Reset phases
    synth.channelState[c].phase[0] = 0.0;
    synth.channelState[c].phase[1] = 0.0;
    synth.channelState[c].output[0] = 0.0;
    synth.channelState[c].output[1] = 0.0;
}

void OPL_NoteOff(int c) {
    int baseReg = c;
    // Clear the key-on bit
    applyFMRegister(c, 0xB0 + baseReg, chpit[c] & 0xDF);
}

void OPL_Touch_Real(int c, int v) {
    // Scale volume for FM synthesis
    v = (v * globalVolume) / 100;
    if (enableNormalization && v > 0 && v < 20) v = 20;
    
    int i = chins[c];
    int baseReg = c;
    
    // Apply volume to carriers
    int modLevel = (adl[i].modChar2 | 63) - v + ((adl[i].modChar2 & 63) * v) / 63;
    int carLevel = (adl[i].carChar2 | 63) - v + ((adl[i].carChar2 & 63) * v) / 63;
    
    applyFMRegister(c, 0x40 + baseReg, modLevel);
    applyFMRegister(c, 0x43 + baseReg, carLevel);
    
    // Update envelope levels for synthesis
    synth.channelState[c].env[0] = 1.0 - (modLevel & 63) / 63.0;
    synth.channelState[c].env[1] = 1.0 - (carLevel & 63) / 63.0;
}

void OPL_Patch(int c) {
    int i = chins[c];
    int baseReg = c;
    
    // Set up FM parameters for the instrument
    // We'll properly apply the parameters to our synthesis state
    
    // Modulator parameters
    applyFMRegister(c, 0x20 + baseReg, adl[i].modChar1);
    applyFMRegister(c, 0x40 + baseReg, adl[i].modChar2);
    applyFMRegister(c, 0x60 + baseReg, adl[i].modChar3);
    applyFMRegister(c, 0x80 + baseReg, adl[i].modChar4);
    applyFMRegister(c, 0xE0 + baseReg, adl[i].modChar5);
    
    // Carrier parameters
    applyFMRegister(c, 0x23 + baseReg, adl[i].carChar1);
    applyFMRegister(c, 0x43 + baseReg, adl[i].carChar2);
    applyFMRegister(c, 0x63 + baseReg, adl[i].carChar3);
    applyFMRegister(c, 0x83 + baseReg, adl[i].carChar4);
    applyFMRegister(c, 0xE3 + baseReg, adl[i].carChar5);
    
    // Set connection and feedback
    applyFMRegister(c, 0xC0 + baseReg, adl[i].fbConn);
    
    // Update our synthesis state directly
    synth.channelState[c].algorithm = adl[i].fbConn & 1;
    int fb = (adl[i].fbConn >> 1) & 7;
    synth.channelState[c].feedback = fb > 0 ? pow(2, fb - 1) * 0.15 : 0;
    
    // Store more detailed envelope parameters
    synth.channelState[c].op[0].mult = adl[i].modChar1 & 0x0F;
    synth.channelState[c].op[0].ksr = (adl[i].modChar1 >> 4) & 0x01;
    synth.channelState[c].op[0].trem = (adl[i].modChar1 >> 7) & 0x01;
    
    synth.channelState[c].op[1].mult = adl[i].carChar1 & 0x0F;
    synth.channelState[c].op[1].ksr = (adl[i].carChar1 >> 4) & 0x01;
    synth.channelState[c].op[1].trem = (adl[i].carChar1 >> 7) & 0x01;
}

void OPL_Pan(int c) {
    int baseReg = c;
    int panValue = adl[chins[c]].fbConn - chpan[c];
    applyFMRegister(c, 0xC0 + baseReg, panValue);
}

void deallocateActiveNote(int m, int n) {
    int x = ActCount[m];
    int q = ActList[m][n];
    ActRev[m][q] = 0;
    ActCount[m] = x - 1;
    
    int c = ActAdlChn[m][q];
    chon[c] = 0;
    chage[c] = 0;
    OPL_NoteOff(c);
    
    if (n == x) return;  // Last note in list?
    
    // Move last note to deleted slot
    q = ActList[m][x];
    ActList[m][n] = q;
    ActRev[m][q] = n;
    cha[ActAdlChn[m][q]] = n;
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
            int n = ActRev[midCh][data1];
            if (n == 0) break;
            
            // Deallocate note
            deallocateActiveNote(midCh, n);
            break;
        }
        
        case NOTE_ON: {
            // Note On event
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            // Note on with velocity 0 is treated as note off
            if (data2 == 0) {
                ChBend[midCh] = 0;
                int n = ActRev[midCh][data1];
                if (n == 0) break;
                
                // Deallocate note
                deallocateActiveNote(midCh, n);
                break;
            }
            
            // Ignore repeat notes without note off
            if (ActRev[midCh][data1]) break;
            
            // Determine instrument and note
            int note = data1;
            int tone = note;
            int i = ChPatch[midCh];
            
            // MIDI channel 9 always plays percussion
            if (midCh == 9) {
                i = 128 + note - 35;
                if (i >= 128 && i < 181) { // Check valid percussion range
                    tone = adl[i].percNote;
                } else {
                    // Invalid percussion note
                    break;
                }
            }
            
            // Allocate FM channel
            double bs = -9;
            int c = -1;
            
            for (int a = 0; a < 18; a++) {
                double s = chage[a];
                if (chon[a] == 0) s += 3e3;             // Empty channel = privileged
                if (chins[a] == i) s += 0.2;            // Same instrument = good
                if (i < 128 && chins[a] > 127) s = s*2+9; // Percussion is inferior
                if (s > bs) { bs = s; c = a; }           // Best candidate wins
            }
            
            // Handle collision with existing note
            if (chon[c]) {
                int m = chm[c];
                int n = cha[c];
                deallocateActiveNote(m, n);
            }
            
            // Set up new note
            chon[c] = 1;
            chins[c] = i;
            chage[c] = 0;
            
            // Allocate active note for MIDI channel
            int n = ActCount[midCh] + 1;
            if (n < 100) { // Check array bounds
                ActList[midCh][n] = note;
                ActRev[midCh][note] = n;
                ActCount[midCh] = n;
                
                // Record info about this note
                ActTone[midCh][note] = tone;
                ActAdlChn[midCh][note] = c;
                ActVol[midCh][note] = data2;
                chm[c] = midCh;
                cha[c] = n;
                
                // Set up FM channel
                OPL_Patch(c);
                OPL_Pan(c);
                OPL_Touch_Real(c, (ActVol[midCh][note] * ChVolume[midCh]) / 127);
                
                // Play note
                double h = 172.00093 * exp(0.057762265 * (tone + ChBend[midCh]));
                OPL_NoteOn(c, h);
            }
            
            break;
        }
        
        case CONTROL_CHANGE: {
            // Control Change
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            switch (data1) {
                case 7:  // Channel Volume
                    ChVolume[midCh] = data2;
                    // Update all notes on this channel
                    for (int a = 1; a <= ActCount[midCh]; a++) {
                        int note = ActList[midCh][a];
                        int c = ActAdlChn[midCh][note];
                        OPL_Touch_Real(c, ActVol[midCh][note] * ChVolume[midCh] / 127);
                    }
                    break;
                    
                case 10: // Pan
                    if (data2 < 48)
                        ChPanning[midCh] = 32;
                    else if (data2 > 79)
                        ChPanning[midCh] = 16;
                    else
                        ChPanning[midCh] = 0;
                    
                    // Update all notes on this channel
                    for (int a = 1; a <= ActCount[midCh]; a++) {
                        int note = ActList[midCh][a];
                        int c = ActAdlChn[midCh][note];
                        chpan[c] = ChPanning[midCh];
                        OPL_Pan(c);
                    }
                    break;
                    
                case 123: // All Notes Off
                    // Turn off all notes on this channel
                    for (int a = ActCount[midCh]; a >= 1; a--) {
                        deallocateActiveNote(midCh, a);
                    }
                    break;
            }
            break;
        }
        
        case PROGRAM_CHANGE: {
            // Program Change
            fread(&data1, 1, 1, midiFile);
            ChPatch[midCh] = data1;
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
    
    // Age all channels
    for (int a = 0; a < 18; a++) {
        chage[a] += t;
    }
}

// SDL audio callback function
void generateAudio(void* userdata, Uint8* stream, int len) {
    FMSynth* synth = (FMSynth*)userdata;
    float* output = (float*)stream;
    int samples = len / (sizeof(float) * AUDIO_CHANNELS);
    
    // Clear buffer
    memset(stream, 0, len);
    
    if (!isPlaying || paused) {
        return;
    }
    
    pthread_mutex_lock(&audioMutex);
    
    // FM synthesis implementation
    for (int i = 0; i < samples; i++) {
        float leftSample = 0;
        float rightSample = 0;
        
        // Process each active FM channel
        for (int ch = 0; ch < 18; ch++) {
            if (chon[ch] && synth->channelState[ch].keyOn) {
                // Get instrument parameters for this channel
                int ins = chins[ch];
                
                // Calculate ADSR envelope values - adding proper envelopes
                float modEnv = calculateEnvelope(synth, ch, 0);
                float carEnv = calculateEnvelope(synth, ch, 1);
                
                // Calculate modulator output with feedback
                double fbVal = 0.0;
                if (synth->channelState[ch].feedback > 0) {
                    fbVal = synth->channelState[ch].output[0] * synth->channelState[ch].feedback;
                }
                
                // Advance modulator phase
                synth->channelState[ch].phase[0] += synth->channelState[ch].freq;
                if (synth->channelState[ch].phase[0] >= 1.0) 
                    synth->channelState[ch].phase[0] -= 1.0;
                    
                // Calculate modulator output using selected waveform
                double modOut = getWaveform(synth->channelState[ch].phase[0] + fbVal, 
                                          adl[ins].modChar5 & 0x07);
                modOut *= modEnv;
                
                // Save modulator output for feedback
                synth->channelState[ch].output[0] = modOut;
                
                // Calculate carrier phase
                double phaseInc = synth->channelState[ch].freq;
                
                // If algorithm connects modulator to carrier
                if (synth->channelState[ch].algorithm == 0) {
                    // Modulate carrier phase - use modulation index based on modulator level
                    double modIndex = (adl[ins].modChar2 & 0x3F) / 63.0;
                    phaseInc *= (1.0 + modOut * modIndex);
                }
                
                // Advance carrier phase
                synth->channelState[ch].phase[1] += phaseInc;
                if (synth->channelState[ch].phase[1] >= 1.0) 
                    synth->channelState[ch].phase[1] -= 1.0;
                
                // Calculate carrier output using selected waveform
                double carOut = getWaveform(synth->channelState[ch].phase[1], 
                                          adl[ins].carChar5 & 0x07);
                carOut *= carEnv;
                
                // Final mix based on algorithm
                float finalOut = 0.0f;
                if (synth->channelState[ch].algorithm == 0) {
                    // FM algorithm: modulator modulates carrier
                    finalOut = carOut;
                } else {
                    // Additive algorithm: modulator and carrier are added
                    finalOut = (carOut + modOut * 0.5) * 0.75;
                }
                
                // Scale based on volume
                float vol = (float)ChVolume[chm[ch]] / 127.0f * globalVolume / 100.0f;
                finalOut *= vol * 0.3f;
                
                // Save carrier output
                synth->channelState[ch].output[1] = finalOut;
                
                // Apply panning
                int m = chm[ch]; // MIDI channel
                int pan = ChPanning[m];
                float panLeft = (pan <= 0) ? 1.0f : (64 - pan) / 64.0f;
                float panRight = (pan >= 0) ? 1.0f : (64 + pan) / 64.0f;
                
                leftSample += finalOut * panLeft;
                rightSample += finalOut * panRight;
            }
        }
        
        // Limit to prevent clipping
        leftSample = fmax(-1.0f, fmin(1.0f, leftSample));
        rightSample = fmax(-1.0f, fmin(1.0f, rightSample));
        
        // Write to output buffer
        output[i * 2] = leftSample;
        output[i * 2 + 1] = rightSample;
    }
    
    // Update playback time
    playTime += samples / (double)SAMPLE_RATE;
    
    // Process MIDI events based on timing
    playwait -= samples / (double)SAMPLE_RATE;
    while (playwait <= 0.1 && isPlaying) {
        processEvents();
    }
    
    pthread_mutex_unlock(&audioMutex);
}

// Initialize everything and start playback
void playMidiFile() {
    // Initialize variables
    for (int i = 0; i < 18; i++) {
        chins[i] = 0;
        chpan[i] = 0;
        chpit[i] = 0;
        chon[i] = 0;
        chage[i] = 0;
        
        // Reset synth channel state
        synth.channelState[i].phase[0] = 0.0;
        synth.channelState[i].phase[1] = 0.0;
        synth.channelState[i].env[0] = 0.0;
        synth.channelState[i].env[1] = 0.0;
        synth.channelState[i].freq = 0.0;
        synth.channelState[i].algorithm = 0;
        synth.channelState[i].keyOn = false;
        synth.channelState[i].feedback = 0.0;
        synth.channelState[i].output[0] = 0.0;
        synth.channelState[i].output[1] = 0.0;
    }
    
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 0;
        ChVibrato[i] = 0;
        ActCount[i] = 0;
        
        for (int j = 0; j < 128; j++) {
            ActRev[i][j] = 0;
        }
    }
    
    // Reset playback state
    playTime = 0;
    isPlaying = true;
    paused = false;
    loopStart = false;
    loopEnd = false;
    playwait = 0;
    loopwait = 0;
    
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
    
    // Update all active notes
    for (int m = 0; m < 16; m++) {
        for (int a = 1; a <= ActCount[m]; a++) {
            int note = ActList[m][a];
            int c = ActAdlChn[m][note];
            OPL_Touch_Real(c, ActVol[m][note] * ChVolume[m] / 127);
        }
    }
    
    pthread_mutex_unlock(&audioMutex);
    
    printf("Volume: %d%%\n", globalVolume);
}

// Toggle volume normalization
void toggleNormalization() {
    pthread_mutex_lock(&audioMutex);
    
    enableNormalization = !enableNormalization;
    
    // Update all active notes
    for (int m = 0; m < 16; m++) {
        for (int a = 1; a <= ActCount[m]; a++) {
            int note = ActList[m][a];
            int c = ActAdlChn[m][note];
            OPL_Touch_Real(c, ActVol[m][note] * ChVolume[m] / 127);
        }
    }
    
    pthread_mutex_unlock(&audioMutex);
    
    printf("Normalization: %s\n", enableNormalization ? "ON" : "OFF");
}
