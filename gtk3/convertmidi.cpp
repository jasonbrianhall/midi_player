#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mutex>
#include "midiplayer.h"
#include "wav_converter.h"
#include "dbopl_wrapper.h"
#include "audioconverter.h"

// Declare external variables for MIDI state
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern int globalVolume;
extern void processEvents(void);

// Additional external variables (add these)
extern int tkPtr[MAX_TRACKS];
extern double tkDelay[MAX_TRACKS];
extern int tkStatus[MAX_TRACKS];
extern bool loopStart;
extern bool loopEnd;
extern int loPtr[MAX_TRACKS];
extern double loDelay[MAX_TRACKS];
extern int loStatus[MAX_TRACKS];
extern double loopwait;
extern int rbPtr[MAX_TRACKS];
extern double rbDelay[MAX_TRACKS];
extern int rbStatus[MAX_TRACKS];
extern int TrackCount;
extern double Tempo;
extern int DeltaTicks;
extern int ChPatch[16];
extern double ChBend[16];
extern int ChVolume[16];
extern int ChPanning[16];
extern int ChVibrato[16];
extern FILE* midiFile;

// Add a mutex to protect the entire function
static std::mutex conversion_mutex;

// Function to convert MIDI to WAV
bool convertMidiToWav(const char* midi_filename, const char* wav_filename, int volume) {
    // Lock the mutex at the beginning of the function
    conversion_mutex.lock();
    
    // Reset global state variables
    playTime = 0.0;
    isPlaying = false;  // Start with false
    playwait = 0.0;
    
    // Reset all state variables completely
    for (int tk = 0; tk < MAX_TRACKS; tk++) {
        tkPtr[tk] = 0;
        tkDelay[tk] = 0;
        tkStatus[tk] = 0;
        loPtr[tk] = 0;
        loDelay[tk] = 0;
        loStatus[tk] = 0;
        rbPtr[tk] = 0;
        rbDelay[tk] = 0;
        rbStatus[tk] = 0;
    }
    
    TrackCount = 0;
    Tempo = 500000;  // Default 120 BPM
    DeltaTicks = 0;
    
    // Reset channel state
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 0;
        ChVibrato[i] = 0;
    }
    
    // Reset loop state
    loopStart = false;
    loopEnd = false;
    loopwait = 0;
    
    // Reset OPL state completely
    OPL_Shutdown();
    
    // Now it's safe to start playing
    isPlaying = true;
    
    // Set global volume
    globalVolume = volume;
    
    // Initialize SDL and audio systems
    if (!initSDL()) {
        fprintf(stderr, "Failed to initialize SDL\n");
        conversion_mutex.unlock();
        return false;
    }
    
    // Load MIDI file
    printf("Loading %s...\n", midi_filename);
    if (!loadMidiFile(midi_filename)) {
        fprintf(stderr, "Failed to load MIDI file\n");
        cleanup();
        conversion_mutex.unlock();
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
        conversion_mutex.unlock();
        return false;
    }
    
    // Temporary buffer for audio generation
    int16_t audio_buffer[AUDIO_BUFFER * AUDIO_CHANNELS];
    
    printf("Converting %s to WAV (Volume: %d%%)...\n", midi_filename, globalVolume);
    
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
    //cleanup();
    
    // Reset state variables before unlocking
    playTime = 0;
    isPlaying = false;
    playwait = 0.0;
    
    // Make sure midiFile is NULL
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
    
    conversion_mutex.unlock();
    
    return true;
}
