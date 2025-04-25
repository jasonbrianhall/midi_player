#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "midiplayer.h"
#include "sb_audio.h"

int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME] = {0}; // Empty filename by default
    int base_address = 0x220;  // Default SoundBlaster base address
    int irq = 7;               // Default SoundBlaster IRQ
    int dma = 1;               // Default SoundBlaster DMA channel
    
    printf("SoundBlaster MIDI Player for DOS\n");
    printf("-------------------------------\n");
    
    // Check for command line parameter
    if (argc > 1) {
        strncpy(filename, argv[1], MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        printf("Using MIDI file from command line: %s\n", filename);
    } else {
        printf("Error: First parameter should be a midi file.\n");
        return 1;
    }
    
    // Optional: Allow override of SoundBlaster settings via command line
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "base=", 5) == 0) {
            base_address = strtol(argv[i] + 5, NULL, 16);
        } else if (strncmp(argv[i], "irq=", 4) == 0) {
            irq = atoi(argv[i] + 4);
        } else if (strncmp(argv[i], "dma=", 4) == 0) {
            dma = atoi(argv[i] + 4);
        }
    }
    
    // Initialize FM instruments
    initFMInstruments();
    
    // Initialize SoundBlaster
    if (!SB_Init(base_address, irq, dma, 44100, true, true)) {
        fprintf(stderr, "Failed to initialize SoundBlaster\n");
        return 1;
    }
    
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
