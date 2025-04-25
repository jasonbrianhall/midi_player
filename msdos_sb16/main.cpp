#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "midiplayer.h"

int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME] = {0}; // Empty filename by default
    
    printf("SoundBlaster MIDI Player for DOS\n");
    printf("-------------------------------\n");
    
    // Check for command line parameter
    if (argc > 1) {
        strncpy(filename, argv[1], MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        printf("Using MIDI file from command line: %s\n", filename);
    } else {
        printf("Error: First parameter should be a midi file.");
        return 1;
    }
        
    // Initialize FM instruments
    initFMInstruments();
    
    // Load MIDI file
    printf("Loading %s...\n", filename);
    if (!loadMidiFile(filename)) {
        fprintf(stderr, "Failed to load MIDI file\n");
        cleanup();
        return 1;
    }
    
    // Play the MIDI file
    playMidiFile();
    
    // Clean up
    cleanup();
    
    return 0;
}
