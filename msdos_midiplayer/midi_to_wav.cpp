/*
 * MIDI to WAV Converter Function
 * Extracted from main_dos.cpp for integration
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "midiplayer_dos.h"
#include "wav_converter.h"
#include "dbopl_wrapper.h"
#include "dos_utils.h"

/* External variables needed for conversion */
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern int globalVolume;
extern void processEvents(void);

/* MIDI to WAV conversion function */
bool convert_midi_to_wav(const char* midi_filename, const char* wav_filename, int volume) {
    /* Reset global state variables */
    playTime = 0;
    isPlaying = true;
    
    /* Set global volume */
    globalVolume = volume;
    
    /* Initialize SDL and audio systems */
    if (!initSDL()) {
        fprintf(stderr, "Failed to initialize SDL\n");
        return false;
    }
    
    /* Load MIDI file */
    printf("Loading %s...\n", midi_filename);
    if (!loadMidiFile(midi_filename)) {
        fprintf(stderr, "Failed to load MIDI file\n");
        cleanup();
        return false;
    }
    
    /* Prepare WAV converter */
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
    
    /* Temporary buffer for audio generation */
    int16_t* audio_buffer = (int16_t*)malloc(AUDIO_BUFFER * AUDIO_CHANNELS * sizeof(int16_t));
    if (!audio_buffer) {
        fprintf(stderr, "Failed to allocate audio buffer\n");
        wav_converter_free(wav_converter);
        cleanup();
        return false;
    }
    
    printf("Converting %s to WAV (Volume: %d%%)...\n", midi_filename, globalVolume);
    
    /* Duration of an audio buffer in seconds */
    double buffer_duration = (double)AUDIO_BUFFER / SAMPLE_RATE;
    
    /* Begin conversion */
    int previous_seconds = -1;
    
    /* Initialize playwait for the first events */
    processEvents();
    
    /* Continue processing as long as the MIDI is still playing */
    keep_running = 1;  /* Reset the ctrl-c flag */
    
    while (isPlaying && keep_running) {
        /* Generate audio block */
        memset(audio_buffer, 0, AUDIO_BUFFER * AUDIO_CHANNELS * sizeof(int16_t));
        OPL_Generate(audio_buffer, AUDIO_BUFFER);
        
        /* Write to WAV file */
        if (!wav_converter_write(wav_converter, audio_buffer, AUDIO_BUFFER * AUDIO_CHANNELS)) {
            fprintf(stderr, "Failed to write audio data\n");
            break;
        }
        
        /* Update playTime */
        playTime += buffer_duration;
        
        /* The critical fix: decrease playwait by exactly the buffer duration */
        playwait -= buffer_duration;
        
        /* Process events when timer reaches zero or below */
        while (playwait <= 0 && isPlaying) {
            processEvents();
        }
        
        /* Display progress */
        int current_seconds = (int)playTime;
        if (current_seconds > previous_seconds) {
            printf("\rConverting... %d seconds", current_seconds);
            fflush(stdout);
            previous_seconds = current_seconds;
        }
        
        /* Check for keyboard input - DOS specific */
        if (kbhit()) {
            int ch = getch();
            if (ch == 'q' || ch == 'Q' || ch == 27) {  /* Q or ESC */
                keep_running = 0;
                break;
            }
        }
    }
    
    printf("\nFinishing conversion...\n");
    
    /* Free audio buffer */
    free(audio_buffer);
    
    /* Finalize WAV file */
    wav_converter_finish(wav_converter);
    wav_converter_free(wav_converter);
    
    /* Cleanup */
    cleanup();
    
    return keep_running; /* If interrupted, return false */
}
