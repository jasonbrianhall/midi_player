/*
 * Sound Blaster WAV Player Header
 * Provides functions for playing WAV files through the Sound Blaster
 */
#ifndef SB_PLAYER_H
#define SB_PLAYER_H

#include <stdbool.h>
#include <stdio.h>

// WAV file header structure
typedef struct {
    // RIFF chunk descriptor
    char riff_header[4];        // "RIFF"
    unsigned long wav_size;     // File size - 8
    char wave_header[4];        // "WAVE"
    
    // fmt subchunk
    char fmt_header[4];         // "fmt "
    unsigned long fmt_chunk_size;  // 16 for PCM
    unsigned short audio_format;    // 1 for PCM
    unsigned short num_channels;    // 1 or 2
    unsigned long sample_rate;      // e.g., 44100
    unsigned long byte_rate;        // sample_rate * num_channels * bytes_per_sample
    unsigned short block_align;     // num_channels * bytes_per_sample
    unsigned short bits_per_sample; // 16 for 16-bit audio
    
    // data subchunk
    char data_header[4];        // "data"
    unsigned long data_size;    // Number of bytes in data
} WAV_HEADER;

// Sound Blaster DSP version
typedef struct {
    unsigned char major;
    unsigned char minor;
} DSP_VERSION;

// Function to play a WAV file
bool play_wav_file(const char *filename);

// Function to detect Sound Blaster and get version
bool detect_sound_blaster(DSP_VERSION *version);

#endif /* SB_PLAYER_H */
