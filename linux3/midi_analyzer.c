#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// MIDI constants (same as in midi_analyzer.c)
#define NOTE_OFF        0x80
#define NOTE_ON         0x90
#define POLY_PRESSURE   0xA0
#define CONTROL_CHANGE  0xB0
#define PROGRAM_CHANGE  0xC0
#define CHAN_PRESSURE   0xD0
#define PITCH_BEND      0xE0
#define SYSTEM_MESSAGE  0xF0
#define META_EVENT      0xFF

// OPL3 limitations
#define MAX_OPL_CHANNELS 18
#define MAX_OPL_NOTES_MELODIC 9  // 9 melodic channels per FM chip (OPL3 has 2 chips)
#define MAX_OPL_NOTES_PERCUSSION 5 // 5 fixed percussion instruments

// Data structure to track active notes
typedef struct {
    bool isActive;
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
} ActiveNote;

// Function prototypes
unsigned long readVarLen(FILE* f);
void readNBytes(FILE* f, int n, unsigned char* buffer);
unsigned long getNumberFromBytes(unsigned char* buffer, int n);
bool analyzePolyphony(const char* filename);

// Global variables
ActiveNote activeNotes[MAX_OPL_CHANNELS];
int totalActiveNotes = 0;
int maxActiveNotes = 0;
int maxActiveMelodicNotes = 0;
int maxActivePercussionNotes = 0;
int maxActiveChannels = 0;
int channelActivity[16] = {0}; // Count of notes per channel

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <midi_file>\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    
    printf("MIDI Compatibility Checker for OPL3 Synthesis\n");
    printf("---------------------------------------------\n");
    printf("Analyzing file: %s\n\n", filename);
    
    if (!analyzePolyphony(filename)) {
        return 1;
    }
    
    return 0;
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

// Helper: Read N bytes from file
void readNBytes(FILE* f, int n, unsigned char* buffer) {
    fread(buffer, 1, n, f);
}

// Helper: Convert bytes to number
unsigned long getNumberFromBytes(unsigned char* buffer, int n) {
    unsigned long value = 0;
    for (int i = 0; i < n; i++) {
        value = (value << 8) | buffer[i];
    }
    return value;
}

// Analyze MIDI file for polyphony issues
bool analyzePolyphony(const char* filename) {
    FILE* midiFile = fopen(filename, "rb");
    if (!midiFile) {
        printf("Error: Could not open file %s\n", filename);
        return false;
    }
    
    // Initialize active notes
    for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
        activeNotes[i].isActive = false;
    }
    
    // Read MIDI header
    unsigned char buffer[1024];
    readNBytes(midiFile, 4, buffer);
    if (memcmp(buffer, "MThd", 4) != 0) {
        printf("Error: Not a valid MIDI file\n");
        fclose(midiFile);
        return false;
    }
    
    // Read header length
    readNBytes(midiFile, 4, buffer);
    unsigned long headerLength = getNumberFromBytes(buffer, 4);
    
    // Read format type
    readNBytes(midiFile, 2, buffer);
    int format = getNumberFromBytes(buffer, 2);
    
    // Read number of tracks
    readNBytes(midiFile, 2, buffer);
    int numTracks = getNumberFromBytes(buffer, 2);
    
    // Read time division
    readNBytes(midiFile, 2, buffer);
    int timeDivision = getNumberFromBytes(buffer, 2);
    
    printf("MIDI Format: %d, Tracks: %d, Time Division: %d\n", 
           format, numTracks, timeDivision);
    
    // Process each track
    for (int trackNum = 0; trackNum < numTracks; trackNum++) {
        // Read track header
        readNBytes(midiFile, 4, buffer);
        if (memcmp(buffer, "MTrk", 4) != 0) {
            printf("Error: Invalid track header for track %d\n", trackNum);
            fclose(midiFile);
            return false;
        }
        
        // Read track length
        readNBytes(midiFile, 4, buffer);
        unsigned long trackLength = getNumberFromBytes(buffer, 4);
        long trackStartPos = ftell(midiFile);
        long trackEndPos = trackStartPos + trackLength;
        
        uint8_t runningStatus = 0;
        
        // Process events in this track
        while (ftell(midiFile) < trackEndPos) {
            // Read delta time
            unsigned long deltaTime = readVarLen(midiFile);
            
            // Read event type
            uint8_t eventType;
            if (fread(&eventType, 1, 1, midiFile) != 1) {
                printf("Error: Unexpected end of file\n");
                break;
            }
            
            // Check for running status
            if (eventType < 0x80) {
                // Use running status
                fseek(midiFile, -1, SEEK_CUR);  // Rewind one byte
                eventType = runningStatus;
            } else {
                runningStatus = eventType;
            }
            
            // Get channel number for channel voice messages
            int channel = eventType & 0x0F;
            
            // Process based on event type
            switch (eventType & 0xF0) {
                case NOTE_OFF: {
                    uint8_t note, velocity;
                    fread(&note, 1, 1, midiFile);
                    fread(&velocity, 1, 1, midiFile);
                    
                    // Remove note from active list
                    for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                        if (activeNotes[i].isActive && 
                            activeNotes[i].channel == channel && 
                            activeNotes[i].note == note) {
                            activeNotes[i].isActive = false;
                            totalActiveNotes--;
                            break;
                        }
                    }
                    break;
                }
                
                case NOTE_ON: {
                    uint8_t note, velocity;
                    fread(&note, 1, 1, midiFile);
                    fread(&velocity, 1, 1, midiFile);
                    
                    if (velocity == 0) {
                        // Note-on with velocity 0 is effectively a note-off
                        // Remove note from active list
                        for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                            if (activeNotes[i].isActive && 
                                activeNotes[i].channel == channel && 
                                activeNotes[i].note == note) {
                                activeNotes[i].isActive = false;
                                totalActiveNotes--;
                                break;
                            }
                        }
                    } else {
                        // Add note to active list
                        int freeSlot = -1;
                        for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                            if (!activeNotes[i].isActive) {
                                freeSlot = i;
                                break;
                            }
                        }
                        
                        if (freeSlot >= 0) {
                            activeNotes[freeSlot].isActive = true;
                            activeNotes[freeSlot].channel = channel;
                            activeNotes[freeSlot].note = note;
                            activeNotes[freeSlot].velocity = velocity;
                            totalActiveNotes++;
                            channelActivity[channel]++;
                            
                            // Update statistics
                            if (totalActiveNotes > maxActiveNotes) {
                                maxActiveNotes = totalActiveNotes;
                            }
                            
                            // Count active melodic and percussion notes
                            int melodicNotes = 0;
                            int percussionNotes = 0;
                            for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                                if (activeNotes[i].isActive) {
                                    if (activeNotes[i].channel == 9) {
                                        percussionNotes++;
                                    } else {
                                        melodicNotes++;
                                    }
                                }
                            }
                            
                            if (melodicNotes > maxActiveMelodicNotes) {
                                maxActiveMelodicNotes = melodicNotes;
                            }
                            
                            if (percussionNotes > maxActivePercussionNotes) {
                                maxActivePercussionNotes = percussionNotes;
                            }
                            
                            // Count active channels
                            int activeChannels = 0;
                            bool channelActive[16] = {false};
                            for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                                if (activeNotes[i].isActive) {
                                    channelActive[activeNotes[i].channel] = true;
                                }
                            }
                            
                            for (int i = 0; i < 16; i++) {
                                if (channelActive[i]) {
                                    activeChannels++;
                                }
                            }
                            
                            if (activeChannels > maxActiveChannels) {
                                maxActiveChannels = activeChannels;
                            }
                        } else {
                            // No free slots - OPL channel limit exceeded
                            // This is expected in many files, as OPL can't handle full MIDI polyphony
                        }
                    }
                    break;
                }
                
                case CONTROL_CHANGE: {
                    uint8_t controller, value;
                    fread(&controller, 1, 1, midiFile);
                    fread(&value, 1, 1, midiFile);
                    
                    // Check for all-notes-off messages
                    if (controller == 123 && value == 0) {
                        // Turn off all notes on this channel
                        for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                            if (activeNotes[i].isActive && activeNotes[i].channel == channel) {
                                activeNotes[i].isActive = false;
                                totalActiveNotes--;
                            }
                        }
                    }
                    break;
                }
                
                case META_EVENT: {
                    // Meta event
                    uint8_t metaType;
                    unsigned long length;
                    
                    fread(&metaType, 1, 1, midiFile);
                    length = readVarLen(midiFile);
                    
                    // Skip meta event data
                    fseek(midiFile, length, SEEK_CUR);
                    break;
                }
                
                case SYSTEM_MESSAGE: {
                    if (eventType == 0xF0 || eventType == 0xF7) {
                        // System Exclusive
                        unsigned long length = readVarLen(midiFile);
                        // Skip SysEx data
                        fseek(midiFile, length, SEEK_CUR);
                    }
                    break;
                }
                
                default:
                    // Skip other event types
                    if ((eventType & 0xF0) == PROGRAM_CHANGE || 
                        (eventType & 0xF0) == CHAN_PRESSURE) {
                        // 1-byte data
                        fseek(midiFile, 1, SEEK_CUR);
                    } else {
                        // 2-byte data
                        fseek(midiFile, 2, SEEK_CUR);
                    }
                    break;
            }
        }
        
        // Make sure we're at the end of the track as expected
        if (ftell(midiFile) != trackEndPos) {
            fseek(midiFile, trackEndPos, SEEK_SET);
        }
    }
    
    fclose(midiFile);
    
    // Print polyphony statistics
    printf("\n===== POLYPHONY ANALYSIS =====\n");
    printf("Maximum simultaneous notes: %d\n", maxActiveNotes);
    printf("Maximum melodic notes: %d/%d (OPL limit is %d)\n", 
           maxActiveMelodicNotes, maxActiveNotes, MAX_OPL_NOTES_MELODIC);
    printf("Maximum percussion notes: %d/%d (OPL limit is %d)\n", 
           maxActivePercussionNotes, maxActiveNotes, MAX_OPL_NOTES_PERCUSSION);
    printf("Maximum active MIDI channels: %d/16\n", maxActiveChannels);
    
    // Print channel activity
    printf("\nChannel Usage (note counts):\n");
    for (int i = 0; i < 16; i++) {
        if (channelActivity[i] > 0) {
            if (i == 9) {
                printf("Channel 10 (Percussion): %d notes\n", channelActivity[i]);
            } else {
                printf("Channel %d: %d notes\n", i + 1, channelActivity[i]);
            }
        }
    }
    
    // Check compatibility
    printf("\n===== COMPATIBILITY ASSESSMENT =====\n");
    
    bool hasPolyphonyIssues = false;
    
    if (maxActiveMelodicNotes > MAX_OPL_NOTES_MELODIC) {
        printf("⚠️ ISSUE: Maximum melodic polyphony (%d) exceeds OPL3 limit (%d)\n", 
               maxActiveMelodicNotes, MAX_OPL_NOTES_MELODIC);
        printf("   This may result in missing notes during playback.\n");
        hasPolyphonyIssues = true;
    } else {
        printf("✓ Melodic polyphony is within OPL3 limits\n");
    }
    
    if (maxActivePercussionNotes > MAX_OPL_NOTES_PERCUSSION && channelActivity[9] > 0) {
        printf("⚠️ ISSUE: Maximum percussion polyphony (%d) exceeds OPL3 limit (%d)\n", 
               maxActivePercussionNotes, MAX_OPL_NOTES_PERCUSSION);
        printf("   This may result in missing percussion during playback.\n");
        hasPolyphonyIssues = true;
    } else if (channelActivity[9] > 0) {
        printf("✓ Percussion polyphony is within OPL3 limits\n");
    }
    
    if (hasPolyphonyIssues) {
        printf("\nRecommendations:\n");
        printf("1. Reduce polyphony in the MIDI file if possible\n");
        printf("2. Consider editing the MIDI file to prioritize important notes\n");
        printf("3. Try different OPL instruments which may be more audible\n");
    } else {
        printf("\n✓ This MIDI file should play correctly on OPL3 synthesis\n");
    }
    
    return true;
}
