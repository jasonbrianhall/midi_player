#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "midiplayer.h"
#include "wav_converter.h"
#include "dbopl_wrapper.h"

// Declare external variables
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern void processEvents(void);

// Function to convert MIDI to WAV
bool convertMidiToWav(const char* midi_filename, const char* wav_filename) {
    // Reset global state variables
    playTime = 0;
    isPlaying = true;
    
    // Initialize SDL and audio systems
    if (!initSDL()) {
        fprintf(stderr, "Failed to initialize SDL\n");
        return false;
    }
    
    // Load MIDI file
    printf("Loading %s...\n", midi_filename);
    if (!loadMidiFile(midi_filename)) {
        fprintf(stderr, "Failed to load MIDI file\n");
        cleanup();
        return false;
    }
    
    // Prepare WAV converter
    WAVConverter* wav_converter = wav_converter_init(
        wav_filename, 
        SAMPLE_RATE, 
        AUDIO_CHANNELS
    );
    
    if (!wav_converter) {
        fprintf(stderr, "Failed to create WAV converter\n");
        cleanup();
        return false;
    }
    
    // Temporary buffer for audio generation
    int16_t audio_buffer[AUDIO_BUFFER * AUDIO_CHANNELS];
    
    printf("Converting %s to WAV...\n", midi_filename);
    
    // Duration of an audio buffer in seconds
    double buffer_duration = (double)AUDIO_BUFFER / SAMPLE_RATE;
    
    // Begin conversion
    int previous_seconds = -1;
    
    // Initialize playwait for the first events
    processEvents();
    
    // Continue processing as long as the MIDI is still playing
    while (isPlaying) {
        // Generate audio block
        memset(audio_buffer, 0, sizeof(audio_buffer));
        OPL_Generate(audio_buffer, AUDIO_BUFFER);
        
        // Write to WAV file
        if (!wav_converter_write(wav_converter, audio_buffer, AUDIO_BUFFER * AUDIO_CHANNELS)) {
            fprintf(stderr, "Failed to write audio data\n");
            break;
        }
        
        // Update playTime
        playTime += buffer_duration;
        
        // Process events if needed
        // The critical fix: decrease playwait by exactly the buffer duration
        // This ensures events are processed at the right time
        playwait -= buffer_duration;
        
        // Process events when timer reaches zero or below
        while (playwait <= 0 && isPlaying) {
            processEvents();
        }
        
        // Display progress
        int current_seconds = (int)playTime;
        if (current_seconds > previous_seconds) {
            printf("\rConverting... %d seconds", current_seconds);
            fflush(stdout);
            previous_seconds = current_seconds;
        }
    }
    
    printf("\nFinishing conversion...\n");
    
    // Finalize WAV file
    wav_converter_finish(wav_converter);
    wav_converter_free(wav_converter);
    
    // Cleanup
    cleanup();
    
    return true;
}

int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc < 3) {
        printf("Usage: %s <input_midi> <output_wav>\n", argv[0]);
        printf("Converts a MIDI file to a WAV file\n");
        return 1;
    }
    
    const char* midi_filename = argv[1];
    const char* wav_filename = argv[2];
    
    printf("Converting %s to %s...\n", midi_filename, wav_filename);
    
    if (convertMidiToWav(midi_filename, wav_filename)) {
        printf("Conversion completed successfully.\n");
        return 0;
    } else {
        fprintf(stderr, "MIDI to WAV conversion failed.\n");
        return 1;
    }
}
