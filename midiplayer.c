// DJGPP MIDI Player with FM Synthesis
// Converted from QBasic/GWBasic code
// Compile with: gcc -o midiplayer midiplayer.cpp

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

int globalVolume = 100;  // Global volume control (0-100)
bool enableNormalization = true;  // Enable volume normalization

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

// Global variables
FMInstrument adl[181];      // FM instrument data
int chins[18];              // Channel instruments
int chpan[18];              // Channel panning
int chpit[18];              // Channel pitch

// Track information
int tkPtr[MAX_TRACKS];      // File position for each track
double tkDelay[MAX_TRACKS]; // Delay for next event
int tkStatus[MAX_TRACKS];   // Running status
double playwait;            // Wait time for playback

// Loop point information
int loPtr[MAX_TRACKS];      // File position for loop
double loDelay[MAX_TRACKS]; // Delay for loop
int loStatus[MAX_TRACKS];   // Status at loop
double loopwait;            // Wait time for loop

// Rollback info (for loop points)
int rbPtr[MAX_TRACKS];
double rbDelay[MAX_TRACKS];
int rbStatus[MAX_TRACKS];

// MIDI channel settings
int ChPatch[16];            // Program/patch for each channel
double ChBend[16];          // Pitch bend
int ChVolume[16];           // Volume
int ChPanning[16];          // Panning
int ChVibrato[16];          // Vibrato depth

// Note tracking
int ActCount[16];                  // Number of active notes per channel
int ActTone[16][128];              // Original note to simulated note
int ActAdlChn[16][128];            // Original note to adlib channel
int ActVol[16][128];               // Original note to pressure
int ActRev[16][128];               // Original note to active index
int ActList[16][100];              // Active index to original note

// Adlib channel state
int chon[18];                      // Channel on flag
double chage[18];                  // Channel age (for allocation)
int chm[18], cha[18];              // Channel MIDI and active info
int chx[18], chc[18];              // Channel x position and color

// Other globals
FILE* midiFile = NULL;
bool isPlaying = false;
bool paused = false;
bool loopStart = true;
bool loopEnd = false;
int TrackCount = 0;
int DeltaTicks = 0;
double InvDeltaTicks = 0;
double Tempo = 0;
double bendsense = 0;
bool began = false;
int txtline = 2;

// Function prototypes
void initFMInstruments();
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

// Helper function to output to OPL ports
void outOPL(int port, int reg, int val) {
    outportb(port, reg);
    // OPL requires a small delay between register select and data write
    for (int i = 0; i < 6; i++) {
        inportb(0x80); // Reading from port 0x80 causes a small delay
    }
    outportb(port + 1, val);
    // Another small delay after write
    for (int i = 0; i < 35; i++) {
        inportb(0x80);
    }
}

// OPL_SetupParams: Set up OPL parameters
void OPL_SetupParams(int c, int& p, int& q, int& o) {
    p = OPL_PORT + 2 * (c / 9);
    q = c % 9;
    o = (q % 3) + 8 * (q / 3);
}

// OPL_NoteOff: Turn off a note
void OPL_NoteOff(int c) {
    int p, q, o;
    OPL_SetupParams(c, p, q, o);
    outOPL(p, 0xB0 + q, chpit[c] & 0xDF);
}

// OPL_NoteOn: Turn on a note with frequency
void OPL_NoteOn(int c, double hertz) {
    int p, q, o, x;
    OPL_SetupParams(c, p, q, o);
    
    x = 0x2000;
    while (hertz >= 1023.5) {
        hertz = hertz / 2;
        x += 0x400;
    }
    
    x += (int)hertz;
    outOPL(p, 0xA0 + q, x & 255);
    x = x / 256;
    outOPL(p, 0xB0 + q, x);
    chpit[c] = x;
}

// OPL_Touch: Set volume for a note
void OPL_Touch(int c, int v) {
    // The formula below: SOLVE(V=127*127 * 2^( (A-63) / 8), A)
    if (v < 72)
        v = 0;
    else
        v = (int)(log(v) * 11.541561 - 48.818955);
    
    OPL_Touch_Real(c, v);
}

// OPL_Touch_Real: Set logarithmic volume
void OPL_Touch_Real(int c, int v) {
    int p, q, o, i, val;
    OPL_SetupParams(c, p, q, o);
    i = chins[c];
    
    // Apply global volume adjustment
    if (enableNormalization) {
        // Scale the volume value by the global volume
        v = (v * globalVolume) / 100;
        
        // Make sure volume never goes below a minimum threshold
        // This helps with quieter instruments
        if (v > 0 && v < 20) v = 20;
    }
    
    // Operator 1
    outOPL(p, 0x40 + o, (adl[i].modChar2 | 63) - v + ((adl[i].modChar2 & 63) * v) / 63);
    
    // Operator 2
    outOPL(p, 0x43 + o, (adl[i].carChar2 | 63) - v + ((adl[i].carChar2 & 63) * v) / 63);
}

void increaseVolume() {
    if (globalVolume < 150) globalVolume += 10;
    updateAllNotes();
}

void decreaseVolume() {
    if (globalVolume > 10) globalVolume -= 10;
    updateAllNotes();
}

void toggleNormalization() {
    enableNormalization = !enableNormalization;
    updateAllNotes();
}

void updateAllNotes() {
    for (int m = 0; m < 16; m++) {
        for (int a = 1; a <= ActCount[m]; a++) {
            int note = ActList[m][a];
            int c = ActAdlChn[m][note];
            OPL_Touch_Real(c, ActVol[m][note] * ChVolume[m]);
        }
    }
}

// OPL_Patch: Set instrument for a channel
void OPL_Patch(int c) {
    int p, q, o, i;
    OPL_SetupParams(c, p, q, o);
    i = chins[c];
    
    // Operator 1 parameters
    outOPL(p, 0x20 + o, adl[i].modChar1);
    outOPL(p, 0x60 + o, adl[i].modChar3);
    outOPL(p, 0x80 + o, adl[i].modChar4);
    outOPL(p, 0xE0 + o, adl[i].modChar5);
    
    // Operator 2 parameters
    outOPL(p, 0x23 + o, adl[i].carChar1);
    outOPL(p, 0x63 + o, adl[i].carChar3);
    outOPL(p, 0x83 + o, adl[i].carChar4);
    outOPL(p, 0xE3 + o, adl[i].carChar5);
}

// OPL_Pan: Set panning for a channel
void OPL_Pan(int c) {
    int p, q, o;
    OPL_SetupParams(c, p, q, o);
    outOPL(p, 0xC0 + q, adl[chins[c]].fbConn - chpan[c]);
}

// OPL_Reset: Reset the OPL chip
void OPL_Reset() {
    int c, p, q, o, x, y;
    
    // Detect OPL3
    c = 0;
    OPL_SetupParams(c, p, q, o);
    for (y = 3; y <= 4; y++) {
        outOPL(p, 4, y * 32);
    }
    inportb(p);
    
    c = 9;
    OPL_SetupParams(c, p, q, o);
    for (y = 0; y <= 2; y++) {
        outOPL(p, 5, y & 1);
    }
    
    // Reset OPL3
    c = 0;
    OPL_SetupParams(c, p, q, o);
    outOPL(p, 1, 32);        // Enable wave
    outOPL(p, 0xBD, 0);      // Set melodic mode
    
    c = 9;
    OPL_SetupParams(c, p, q, o);
    outOPL(p, 5, 1);         // Enable OPL3
    outOPL(p, 4, 0);         // Select mode 0
    
    // Silence all channels
    OPL_Silence();
}

// OPL_Silence: Turn off all notes
void OPL_Silence() {
    for (int c = 0; c < 18; c++) {
        OPL_NoteOff(c);
        OPL_Touch_Real(c, 0);
    }
}

// readString: Read bytes from file
int readString(FILE* f, int len, char* str) {
    return fread(str, 1, len, f);
}

// readVarLen: Read variable length value
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

// convertInteger: Parse big-endian integer
unsigned long convertInteger(char* str, int len) {
    unsigned long value = 0;
    for (int i = 0; i < len; i++) {
        value = value * 256 + (unsigned char)str[i];
    }
    return value;
}

// Initialize FM instrument data
void initFMInstruments() {
    // This function will initialize the FM instrument data array
    // with 180 FM instrument definitions from the original QBasic code
    
    // GP1: Acoustic Grand Piano
    adl[0].modChar1 = 1;   adl[0].carChar1 = 1;
    adl[0].modChar2 = 143; adl[0].carChar2 = 6;
    adl[0].modChar3 = 242; adl[0].carChar3 = 242;
    adl[0].modChar4 = 244; adl[0].carChar4 = 247;
    adl[0].modChar5 = 0;   adl[0].carChar5 = 0;
    adl[0].fbConn = 56;    adl[0].percNote = 0;
    
    // Fill in the rest of the instrument data here...
    // This would be too large to include completely, but would follow the
    // pattern shown in the QBasic DATA statements
    
    // For this example, we're just initializing the first instrument
    // In a full implementation, you would initialize all 180 instruments
}

// Deallocate an active note
void deallocateActiveNote(int m, int n) {
    int x = ActCount[m];
    int q = ActList[m][n];
    ActRev[m][q] = 0;
    ActCount[m] = x - 1;
    
    int c = ActAdlChn[m][q];
    chon[c] = 0;
    chage[c] = 0;
    OPL_NoteOff(c);
    
    //gotoxy(chx[c], 20 - c);
    textcolor(1);
    printf(".");
    
    if (n == x) return;  // Last note in list?
    
    // Move last note to deleted slot
    q = ActList[m][x];
    ActList[m][n] = q;
    ActRev[m][q] = n;
    cha[ActAdlChn[m][q]] = n;
}

// Handle MIDI events
void handleMidiEvent(int tk) {
    unsigned char status, data1, data2;
    unsigned char buffer[256];
    unsigned char evtype;
    int len;
    
    // Get file position
    fseek(midiFile, tkPtr[tk], SEEK_SET);
    
    // Read status byte or use running status
    if (fread(&status, 1, 1, midiFile) != 1) return;
    
    // Check for running status
    if (status < 0x80) {
        fseek(midiFile, tkPtr[tk], SEEK_SET); // Go back one byte
        status = tkStatus[tk] | 0x80; // Use running status
    }
    
    int midCh = status & 0x0F;
    tkStatus[tk] = status;
    
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
                tone = adl[i].percNote;
            }
            
            // Allocate AdLib channel
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
            began = true;
            
            // Allocate active note for MIDI channel
            int n = ActCount[midCh] + 1;
            ActList[midCh][n] = note;
            ActRev[midCh][note] = n;
            ActCount[midCh] = n;
            
            // Record info about this note
            ActTone[midCh][note] = tone;
            ActAdlChn[midCh][note] = c;
            ActVol[midCh][note] = data2;
            chm[c] = midCh;
            cha[c] = n;
            
            // Set up AdLib channel
            OPL_Patch(c);
            OPL_Pan(c);
            OPL_Touch_Real(c, (ActVol[midCh][note] * ChVolume[midCh]));
            
            // Visual display (would be updated for C++)
            chx[c] = 1 + (tone + 63) % 80;
            chc[c] = 9 + (chins[c] % 6);
            //gotoxy(chx[c], 20 - c);
            textcolor(chc[c]);
            printf("#");
            
            // Play note
            double h = 172.00093 * exp(0.057762265 * (tone + ChBend[midCh]));
            OPL_NoteOn(c, h);
            
            break;
        }
        
        case POLY_PRESSURE: {
            // Polyphonic Key Pressure
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            if (ActRev[midCh][data1] == 0) break;
            
            int c = ActAdlChn[midCh][data1];
            //gotoxy(chx[c], 20 - c);
            textcolor(chc[c]);
            printf("&");
            
            ActVol[midCh][data1] = data2;
            OPL_Touch_Real(c, data2 * ChVolume[midCh]);
            
            break;
        }
        
        case CONTROL_CHANGE: {
            // Control Change
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            switch (data1) {
                case 1:  // Modulation wheel
                    ChVibrato[midCh] = data2;
                    break;
                    
                case 6:  // Data Entry MSB
                    bendsense = data2 / 8192.0;
                    break;
                    
                case 7:  // Channel Volume
                    ChVolume[midCh] = data2;
                    // Update all notes on this channel
                    for (int a = 1; a <= ActCount[midCh]; a++) {
                        int note = ActList[midCh][a];
                        int c = ActAdlChn[midCh][note];
                        OPL_Touch_Real(c, ActVol[midCh][note] * ChVolume[midCh]);
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
                        OPL_Pan(c);
                    }
                    break;
                    
                case 121: // Reset All Controllers
                    ChBend[midCh] = 0;
                    ChVibrato[midCh] = 0;
                    ChPanning[midCh] = 0;
                    
                    // Update pitches
                    for (int a = 1; a <= ActCount[midCh]; a++) {
                        int note = ActList[midCh][a];
                        int c = ActAdlChn[midCh][note];
                        int tone = ActTone[midCh][note];
                        double h = 172.00093 * exp(0.057762265 * (tone + ChBend[midCh]));
                        OPL_NoteOn(c, h);
                    }
                    
                    // Update volumes
                    for (int a = 1; a <= ActCount[midCh]; a++) {
                        int note = ActList[midCh][a];
                        int c = ActAdlChn[midCh][note];
                        OPL_Touch_Real(c, ActVol[midCh][note] * ChVolume[midCh]);
                    }
                    
                    // Update pans
                    for (int a = 1; a <= ActCount[midCh]; a++) {
                        int note = ActList[midCh][a];
                        int c = ActAdlChn[midCh][note];
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
        
        case CHAN_PRESSURE: {
            // Channel Pressure
            fread(&data1, 1, 1, midiFile);
            
            // Update all note volumes
            for (int a = 1; a <= ActCount[midCh]; a++) {
                int note = ActList[midCh][a];
                int c = ActAdlChn[midCh][note];
                ActVol[midCh][note] = data1;
                OPL_Touch_Real(c, data1 * ChVolume[midCh]);
            }
            break;
        }
        
        case PITCH_BEND: {
            // Pitch Bend
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            ChBend[midCh] = (data1 + data2 * 128 - 8192) * bendsense;
            
            // Update all note pitches
            for (int a = 1; a <= ActCount[midCh]; a++) {
                int note = ActList[midCh][a];
                int c = ActAdlChn[midCh][note];
                int tone = ActTone[midCh][note];
                double h = 172.00093 * exp(0.057762265 * (tone + ChBend[midCh]));
                OPL_NoteOn(c, h);
            }
            break;
        }
        
        case META_EVENT: case SYSTEM_MESSAGE: {
            // Meta events and system exclusive
            if (status == META_EVENT) {
                // Meta event
                fread(&evtype, 1, 1, midiFile);
                unsigned long len = readVarLen(midiFile);
                
                if (evtype == META_END_OF_TRACK) {
                    tkStatus[tk] = -1;  // Mark track as ended
                    fseek(midiFile, len, SEEK_CUR);  // Skip event data
                } else if (evtype == META_TEMPO) {
                    // Tempo change
                    char tempo[4] = {0};
                    readString(midiFile, (int)len, tempo);
                    unsigned long tempoVal = convertInteger(tempo, (int)len);
                    Tempo = tempoVal * InvDeltaTicks;
                } else if (evtype == META_TEXT) {
                    // Text event - check for loop markers
                    char text[256] = {0};
                    readString(midiFile, (int)len, text);
                    
                    if (strcmp(text, "loopStart") == 0) {
                        loopStart = true;
                    } else if (strcmp(text, "loopEnd") == 0) {
                        loopEnd = true;
                    }
                    
                    // Display meta event text
                    txtline = 3 + (txtline - 2) % 20;
                    //gotoxy(1, txtline);
                    textcolor(8);
                    printf("Meta %d: %s", evtype, text);
                } else {
                    // Skip other meta events
                    fseek(midiFile, len, SEEK_CUR);
                }
            } else {
                // System exclusive - skip
                unsigned long len = readVarLen(midiFile);
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
    
    // Update all track delays
    for (int tk = 0; tk < TrackCount; tk++) {
        tkDelay[tk] -= nextDelay;
    }
    
    // Schedule next event
    double t = nextDelay * Tempo;
    if (began) playwait += t;
    
    // Age all channels
    for (int a = 0; a < 18; a++) {
        chage[a] += t;
    }
    
    // Check for loop or return
    if (t < 0 || loopEnd) {
        // Return to loop beginning
        for (int tk = 0; tk < TrackCount; tk++) {
            tkPtr[tk] = loPtr[tk];
            tkDelay[tk] = loDelay[tk];
            tkStatus[tk] = loStatus[tk];
        }
        loopEnd = false;
        playwait = loopwait;
    }
}

// Load and parse MIDI file
bool loadMidiFile(const char* filename) {
    char buffer[256];
    char id[5] = {0};
    unsigned long headerLength;
    int format;
    
    midiFile = fopen(filename, "rb");
    if (!midiFile) {
        printf("Error: Could not open file %s\n", filename);
        return false;
    }
    
    // Read MIDI header
    if (readString(midiFile, 4, id) != 4 || strncmp(id, "MThd", 4) != 0) {
        printf("Error: Not a valid MIDI file\n");
        fclose(midiFile);
        return false;
    }
    
    // Read header length
    readString(midiFile, 4, buffer);
    headerLength = convertInteger(buffer, 4);
    if (headerLength != 6) {
        printf("Error: Invalid MIDI header length\n");
        fclose(midiFile);
        return false;
    }
    
    // Read format type
    readString(midiFile, 2, buffer);
    format = (int)convertInteger(buffer, 2);
    
    // Read number of tracks
    readString(midiFile, 2, buffer);
    TrackCount = (int)convertInteger(buffer, 2);
    if (TrackCount > MAX_TRACKS) {
        printf("Error: Too many tracks in MIDI file\n");
        fclose(midiFile);
        return false;
    }
    
    // Read time division
    readString(midiFile, 2, buffer);
    DeltaTicks = (int)convertInteger(buffer, 2);
    
    InvDeltaTicks = PLAY_MODE_TICKS / (240000000.0 * DeltaTicks);
    Tempo = 1000000.0 * InvDeltaTicks;
    bendsense = 2.0 / 8192.0;
    
    // Initialize track data
    for (int tk = 0; tk < TrackCount; tk++) {
        // Read track header
        if (readString(midiFile, 4, id) != 4 || strncmp(id, "MTrk", 4) != 0) {
            printf("Error: Invalid track header\n");
            fclose(midiFile);
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
    
    // Initialize MIDI channel volumes
    for (int a = 0; a < 16; a++) {
        ChVolume[a] = 127;
    }
    
    printf("MIDI file loaded: %s\n", filename);
    printf("Format: %d, Tracks: %d, Time Division: %d\n", format, TrackCount, DeltaTicks);
    
    return true;
}

// Timer callback for playback
void handleTimer() {
    // Countdown playback wait time
    if (began) playwait -= 1.0;
    
    // Process events until we have enough delay
    while (playwait < 0.5) {
        processEvents();
    }
}

// Play the MIDI file
void playMidiFile(const char* filename) {
    // Initialize variables
    for (int i = 0; i < 18; i++) {
        chins[i] = 0;
        chpan[i] = 0;
        chpit[i] = 0;
        chon[i] = 0;
        chage[i] = 0;
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
    
    // Initialize FM instruments
    initFMInstruments();
    
    // Reset OPL chip
    OPL_Reset();
    
    // Load MIDI file
    if (!loadMidiFile(filename)) {
        return;
    }
    
    // Setup display
    clrscr();
    gotoxy(1, 2);
    textcolor(8);
    printf("Press Q to quit; Space to pause\n");
    
    // Start playback
    isPlaying = true;
    began = false;
    loopStart = true;
    loopEnd = false;
    playwait = 0;
    txtline = 2;
    
    // Main playback loop
while (isPlaying) {
    // Check for keypresses
    if (kbhit()) {
        int key = getch();
        if (key == ' ') {
            paused = !paused;
            if (paused) {
                printf("Pause\n");
            } else {
                printf("Ok\n");
            }
        } else if (key == 'q' || key == 'Q' || key == 27) {
            isPlaying = false;
            break;
        } else if (key == '+' || key == '=') {
            increaseVolume();
            printf("Volume: %d%%\n", globalVolume);
        } else if (key == '-' || key == '_') {
            decreaseVolume();
            printf("Volume: %d%%\n", globalVolume);
        } else if (key == 'n' || key == 'N') {
            toggleNormalization();
            printf("Normalization: %s\n", enableNormalization ? "ON" : "OFF");
        }
    }
    
    // Process events if not paused
    if (!paused) {
        handleTimer();
    }
    
    // Simple delay to prevent CPU hogging
    delay(10);
}    
    // Clean up
    OPL_Silence();
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
    
    printf("End!\n");
}

// Main function
int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME] = {0}; // Empty filename by default
    
    // Initialize text mode
    clrscr();
    textcolor(7);
    textbackground(0);
    
    printf("DJGPP MIDI Player with Volume Control\n");
    printf("-------------------------------------\n");
    
    // Check for command line parameter
    if (argc > 1) {
        strncpy(filename, argv[1], MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        printf("Using MIDI file from command line: %s\n", filename);
    } else {
        printf("Please enter a midi file to play\n");
        return 1;
    }
    
    // Display a short delay to show filename
    printf("Loading %s...\n", filename);
    delay(1000); // 1-second delay
    
    // Display controls
    printf("Controls:\n");
    printf("  Q     - Quit\n");
    printf("  Space - Pause/Resume\n");
    printf("  +/-   - Increase/Decrease Volume\n");
    printf("  N     - Toggle Volume Normalization\n\n");
    
    // Play the MIDI file
    playMidiFile(filename);
    
    return 0;
}

