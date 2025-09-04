#include "convertoggtowav.h"
#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool convertOggToWav(const char* ogg_filename, const char* wav_filename) {
    FILE* ogg_file = fopen(ogg_filename, "rb");
    if (!ogg_file) {
        printf("Cannot open OGG file: %s\n", ogg_filename);
        return false;
    }
    
    OggVorbis_File vf;
    if (ov_open_callbacks(ogg_file, &vf, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) {
        printf("Invalid OGG Vorbis file\n");
        fclose(ogg_file);
        return false;
    }
    
    vorbis_info* vi = ov_info(&vf, -1);
    long total_samples = ov_pcm_total(&vf, -1);
    
    printf("OGG: %ld Hz, %d channels, %ld samples\n", vi->rate, vi->channels, total_samples);
    
    // Create WAV file
    FILE* wav_file = fopen(wav_filename, "wb");
    if (!wav_file) {
        printf("Cannot create WAV file: %s\n", wav_filename);
        ov_clear(&vf);
        return false;
    }
    
    // Write WAV header
    int sample_rate = vi->rate;
    int channels = vi->channels;
    int bits_per_sample = 16;
    long data_size = total_samples * channels * (bits_per_sample / 8);
    long file_size = data_size + 36;
    
    // RIFF header
    fwrite("RIFF", 1, 4, wav_file);
    fwrite(&file_size, 4, 1, wav_file);
    fwrite("WAVE", 1, 4, wav_file);
    
    // fmt chunk
    fwrite("fmt ", 1, 4, wav_file);
    int fmt_chunk_size = 16;
    short audio_format = 1; // PCM
    int byte_rate = sample_rate * channels * bits_per_sample / 8;
    short block_align = channels * bits_per_sample / 8;
    
    fwrite(&fmt_chunk_size, 4, 1, wav_file);
    fwrite(&audio_format, 2, 1, wav_file);
    fwrite(&channels, 2, 1, wav_file);
    fwrite(&sample_rate, 4, 1, wav_file);
    fwrite(&byte_rate, 4, 1, wav_file);
    fwrite(&block_align, 2, 1, wav_file);
    fwrite(&bits_per_sample, 2, 1, wav_file);
    
    // data chunk
    fwrite("data", 1, 4, wav_file);
    fwrite(&data_size, 4, 1, wav_file);
    
    // Convert audio data
    char buffer[4096];
    int current_section;
    long bytes_read;
    
    while ((bytes_read = ov_read(&vf, buffer, sizeof(buffer), 0, 2, 1, &current_section)) > 0) {
        fwrite(buffer, 1, bytes_read, wav_file);
    }
    
    fclose(wav_file);
    ov_clear(&vf);
    
    printf("OGG conversion complete\n");
    return true;
}
