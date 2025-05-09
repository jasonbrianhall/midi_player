/*
 * Integrated DOS MIDI Player
 * Combines MIDI-to-WAV conversion and WAV playback in a single application
 * Supports both direct playback and conversion modes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <conio.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include "midiplayer_dos.h"
#include "dbopl_wrapper.h"
#include "sb_player.h"
#include "dos_utils.h"
#include "midi_to_wav.h"

/* Global variables */
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern int globalVolume;
extern bool enableNormalization;
extern bool paused;

/* Function prototypes */
bool convert_and_play(const char* midi_filename, int volume);
bool direct_play_midi(const char* midi_filename, int volume);
void display_help(void);

int main(int argc, char* argv[]) {
    /* Set default volume to 1000% (lower than converter-only default) */
    globalVolume = 1000;
    
    /* Setup terminal settings */
    init_terminal();
    
    /* Parse command line arguments */
    if (argc < 2) {
        display_help();
        restore_terminal();
        return 1;
    }
    
    /* Parse options */
    bool direct_mode = false;
    bool convert_only = false;
    char* midi_filename = NULL;
    char* wav_filename = NULL;
    int volume = globalVolume;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--convert") == 0) {
            convert_only = true;
            if (i + 1 < argc) {
                wav_filename = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing WAV filename after -c/--convert option\n");
                display_help();
                restore_terminal();
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--volume") == 0) {
            if (i + 1 < argc) {
                volume = atoi(argv[++i]);
                if (volume <= 0) {
                    fprintf(stderr, "Warning: Invalid volume. Using default (1000%%)\n");
                    volume = 1000;
                }
            } else {
                fprintf(stderr, "Warning: Missing volume after -v/--volume option\n");
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            display_help();
            restore_terminal();
            return 0;
        } else {
            /* Assume it's the MIDI filename */
            if (!midi_filename) {
                midi_filename = argv[i];
            } else {
                fprintf(stderr, "Warning: Multiple filenames specified. Using '%s'\n", midi_filename);
            }
        }
    }
    
    /* Check if we have a MIDI filename */
    if (!midi_filename) {
        fprintf(stderr, "Error: No MIDI file specified\n");
        display_help();
        restore_terminal();
        return 1;
    }
    
    /* Set global volume */
    globalVolume = volume;
    
    /* Generate temporary WAV filename if needed */
    char temp_wav_filename[256];
    if (!wav_filename && convert_only) {
        /* Create WAV filename by replacing .mid with .wav */
        strcpy(temp_wav_filename, midi_filename);
        char* ext = strrchr(temp_wav_filename, '.');
        if (ext) {
            strcpy(ext, ".wav");
        } else {
            strcat(temp_wav_filename, ".wav");
        }
        wav_filename = temp_wav_filename;
    }
    
    printf("DOS MIDI Player\n");
    printf("==============\n");
    
    /* Select operation mode */
    if (direct_mode) {
        /* Direct MIDI playback mode */
        printf("Playing %s directly (volume: %d%%)\n", midi_filename, volume);
        if (!direct_play_midi(midi_filename, volume)) {
            fprintf(stderr, "Error: Failed to play MIDI file\n");
            restore_terminal();
            return 1;
        }
    } else if (convert_only) {
        /* Convert to WAV only */
        printf("Converting %s to %s (volume: %d%%)\n", midi_filename, wav_filename, volume);
        if (!convert_midi_to_wav(midi_filename, wav_filename, volume)) {
            fprintf(stderr, "Error: MIDI to WAV conversion failed\n");
            restore_terminal();
            return 1;
        }
        printf("Conversion completed successfully\n");
    } else {
        /* Convert and play mode */
        printf("Converting and playing %s (volume: %d%%)\n", midi_filename, volume);
        if (!convert_and_play(midi_filename, volume)) {
            fprintf(stderr, "Error: Failed to convert and play MIDI file\n");
            restore_terminal();
            return 1;
        }
    }
    
    /* Restore terminal settings */
    restore_terminal();
    return 0;
}

/* Display help information */
void display_help(void) {
    printf("DOS MIDI Player - Plays MIDI files using OPL3 emulation\n");
    printf("Usage: MIDPLAY [options] <midi_file>\n\n");
    printf("Options:\n");
    printf("  -c, --convert WAV  Convert to WAV file and exit\n");
    printf("  -v, --volume N     Set volume (default: 1000%%)\n");
    printf("  -h, --help         Display this help and exit\n\n");
    printf("Controls during playback:\n");
    printf("  Space              Pause/Resume\n");
    printf("  +/-                Increase/Decrease volume\n");
    printf("  N                  Toggle volume normalization\n");
    printf("  Q/ESC              Quit playback\n");
    printf("  Ctrl+C             Stop playback\n");
}

/* Convert MIDI to WAV and immediately play it */
bool convert_and_play(const char* midi_filename, int volume) {
    /* Create temporary WAV filename */
    char temp_wav_filename[256];
    strcpy(temp_wav_filename, "midplay.tmp");
    
    /* Convert MIDI to WAV */
    printf("Converting MIDI to temporary WAV file...\n");
    if (!convert_midi_to_wav(midi_filename, temp_wav_filename, volume)) {
        fprintf(stderr, "Error: MIDI to WAV conversion failed\n");
        return false;
    }
    
    /* Play the WAV file */
    printf("Playing converted WAV file...\n");
    if (!play_wav_file(temp_wav_filename)) {
        fprintf(stderr, "Error: WAV playback failed\n");
        remove(temp_wav_filename);  /* Delete temporary file */
        return false;
    }
    
    /* Clean up temporary file */
    remove(temp_wav_filename);
    
    return true;
}

/* Direct MIDI playback without WAV conversion */
bool direct_play_midi(const char* midi_filename, int volume) {
    /* Initialize SDL audio and mixer */
    if (!initSDL()) {
        fprintf(stderr, "Failed to initialize audio system\n");
        return false;
    }
    
    /* Load the MIDI file */
    if (!loadMidiFile(midi_filename)) {
        fprintf(stderr, "Failed to load MIDI file: %s\n", midi_filename);
        cleanup();
        return false;
    }
    
    /* Set global volume */
    globalVolume = volume;
    
    /* Play the MIDI file */
    playMidiFile();
    
    /* Cleanup */
    cleanup();
    
    return true;
}
