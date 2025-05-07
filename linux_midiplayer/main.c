#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "midiplayer.h"

// This declaration ensures we can access the global variable from midiplayer.c
extern struct termios old_tio;

// This function will be called when the program exits
void restore_terminal(void) {
    // Reset terminal to original settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

int main(int argc, char* argv[]) {
    // Register the cleanup function to ensure terminal settings are restored on exit
    atexit(restore_terminal);
    
    char filename[MAX_FILENAME] = {0}; // Empty filename by default
    
    printf("Linux MIDI Player with SDL2\n");
    printf("-------------------------\n");
    
    // Check for command line parameter
    if (argc > 1) {
        strncpy(filename, argv[1], MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        printf("Using MIDI file from command line: %s\n", filename);
    } else {
        printf("First parameter should be a midi file.");
        return 1;
    }
    
    // Initialize SDL and audio
    if (!initSDL()) {
        fprintf(stderr, "Failed to initialize SDL\n");
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
