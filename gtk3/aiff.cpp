#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "aiff.h"
#include "audio_player.h"
#include "vfs.h"


// Helper function to read big-endian 32-bit integer
uint32_t read_be32(const void* data) {
    const unsigned char* bytes = (const unsigned char*)data;
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

// Helper function to read big-endian 16-bit integer
uint16_t read_be16(const void* data) {
    const unsigned char* bytes = (const unsigned char*)data;
    return (bytes[0] << 8) | bytes[1];
}

// Read IEEE 754 80-bit extended precision float (sample rate)
double read_ieee754_extended(const void* data) {
    const unsigned char* bytes = (const unsigned char*)data;
    
    // Extract sign, exponent, and mantissa
    bool sign = (bytes[0] & 0x80) != 0;
    uint16_t exponent = ((bytes[0] & 0x7F) << 8) | bytes[1];
    
    // Handle special cases
    if (exponent == 0) return 0.0;
    if (exponent == 0x7FFF) return sign ? -INFINITY : INFINITY;
    
    // Extract mantissa (64-bit)
    uint64_t mantissa = 0;
    for (int i = 2; i < 10; i++) {
        mantissa = (mantissa << 8) | bytes[i];
    }
    
    // Quick check for common sample rates to avoid floating point errors
    if (exponent == 0x400E) {
        if ((mantissa >> 32) == 0xAC440000UL) return 44100.0;
        if ((mantissa >> 32) == 0xBB800000UL) return 48000.0;
        if ((mantissa >> 32) == 0x98968000UL) return 39062.5;
    }
    if (exponent == 0x400D) {
        if ((mantissa >> 32) == 0xAC440000UL) return 22050.0;
        if ((mantissa >> 32) == 0xBB800000UL) return 24000.0;
    }
    if (exponent == 0x400C) {
        if ((mantissa >> 32) == 0xAC440000UL) return 11025.0;
    }
    if (exponent == 0x400F) {
        if ((mantissa >> 32) == 0x80000000UL) return 65536.0;
    }
    
    // General calculation for non-standard rates
    double normalized_mantissa = 1.0 + (double)mantissa / (1ULL << 63);
    double result = normalized_mantissa * pow(2.0, exponent - 16383);
    
    return sign ? -result : result;
}

// Convert big-endian 16-bit samples to host byte order
void convert_be16_samples(int16_t* samples, size_t count) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    for (size_t i = 0; i < count; i++) {
        uint16_t val = (uint16_t)samples[i];
        samples[i] = (int16_t)(((val & 0xFF) << 8) | ((val >> 8) & 0xFF));
    }
#endif
    // On big-endian systems, no conversion needed
}

// Parse AIFF file header and return format information
bool parse_aiff_header(FILE* file, AIFFInfo* info) {
    if (!file || !info) return false;
    
    // Initialize info structure
    memset(info, 0, sizeof(AIFFInfo));
    
    // Read FORM header (12 bytes)
    char form_header[12];
    if (fread(form_header, 1, 12, file) != 12) {
        return false;
    }
    
    // Verify AIFF format
    if (strncmp(form_header, "FORM", 4) != 0 || strncmp(form_header + 8, "AIFF", 4) != 0) {
        return false;
    }
    
    uint32_t form_size = read_be32(form_header + 4);
    
    // Read chunks
    long current_pos = 12; // After FORM header
    bool found_comm = false, found_ssnd = false;
    
    while (current_pos < (long)(form_size + 8) && (!found_comm || !found_ssnd)) {
        fseek(file, current_pos, SEEK_SET);
        
        char chunk_header[8];
        if (fread(chunk_header, 1, 8, file) != 8) {
            break;
        }
        
        uint32_t chunk_size = read_be32(chunk_header + 4);
        
        if (strncmp(chunk_header, "COMM", 4) == 0) {
            // Common chunk - contains audio format info
            char comm_data[18];
            if (fread(comm_data, 1, 18, file) != 18) {
                return false;
            }
            
            info->channels = read_be16(comm_data);
            info->sample_frames = read_be32(comm_data + 2);
            info->bits_per_sample = read_be16(comm_data + 6);
            info->sample_rate = read_ieee754_extended(comm_data + 8);
            
            if (info->sample_rate > 0) {
                info->duration = (double)info->sample_frames / info->sample_rate;
            }
            
            found_comm = true;
            
        } else if (strncmp(chunk_header, "SSND", 4) == 0) {
            // Sound data chunk
            char ssnd_header[8];
            if (fread(ssnd_header, 1, 8, file) != 8) {
                return false;
            }
            
            uint32_t offset = read_be32(ssnd_header);
            
            // Calculate actual audio data position
            info->data_offset = current_pos + 16 + offset; // chunk header + ssnd header + offset
            info->data_size = chunk_size - 8 - offset; // chunk size - ssnd header - offset
            
            found_ssnd = true;
        }
        
        // Move to next chunk (chunks are word-aligned)
        current_pos += 8 + chunk_size;
        if (chunk_size % 2) current_pos++; // Pad to word boundary
    }
    
    // Validate that we found required chunks
    return found_comm && found_ssnd && info->channels > 0 && info->sample_rate > 0;
}

// Load audio samples from AIFF file
bool load_aiff_samples(FILE* file, const AIFFInfo* info, int16_t** samples, size_t* sample_count) {
    if (!file || !info || !samples || !sample_count) return false;
    
    // Currently only support 16-bit samples
    if (info->bits_per_sample != 16) {
        printf("Unsupported AIFF sample size: %d bits (only 16-bit supported)\n", info->bits_per_sample);
        return false;
    }
    
    // Seek to audio data
    if (fseek(file, info->data_offset, SEEK_SET) != 0) {
        return false;
    }
    
    // Calculate number of samples
    size_t total_samples = info->data_size / 2; // 16-bit = 2 bytes per sample
    
    // Allocate memory for samples
    int16_t* audio_data = (int16_t*)malloc(info->data_size);
    if (!audio_data) {
        return false;
    }
    
    // Read audio data
    if (fread(audio_data, 1, info->data_size, file) != info->data_size) {
        free(audio_data);
        return false;
    }
    
    // Convert big-endian samples to host byte order
    convert_be16_samples(audio_data, total_samples);
    
    *samples = audio_data;
    *sample_count = total_samples;
    
    return true;
}

bool convert_aiff_to_wav(AudioPlayer *player, const char* filename) {
    // Check cache first
    const char* cached_file = get_cached_conversion(&player->conversion_cache, filename);
    if (cached_file) {
        strncpy(player->temp_wav_file, cached_file, sizeof(player->temp_wav_file) - 1);
        player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
        return true;
    }
    
    // Generate a unique virtual filename
    static int virtual_counter = 0;
    char virtual_filename[256];
    snprintf(virtual_filename, sizeof(virtual_filename), "virtual_aiff_%d.wav", virtual_counter++);
    
    strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting AIFF to virtual WAV: %s -> %s\n", filename, virtual_filename);
    
    // Open AIFF file
    FILE* aiff_file = fopen(filename, "rb");
    if (!aiff_file) {
        printf("Cannot open AIFF file: %s\n", filename);
        return false;
    }
    
    // Parse AIFF header
    AIFFInfo aiff_info;
    if (!parse_aiff_header(aiff_file, &aiff_info)) {
        printf("Failed to parse AIFF header\n");
        fclose(aiff_file);
        return false;
    }
    
    printf("AIFF: %d Hz, %d channels, %d bits, %.2f seconds\n", 
           (int)aiff_info.sample_rate, aiff_info.channels, 
           aiff_info.bits_per_sample, aiff_info.duration);
    
    // Load AIFF samples
    int16_t* samples = NULL;
    size_t sample_count = 0;
    if (!load_aiff_samples(aiff_file, &aiff_info, &samples, &sample_count)) {
        printf("Failed to load AIFF samples\n");
        fclose(aiff_file);
        return false;
    }
    fclose(aiff_file);
    
    // Calculate WAV data size
    uint32_t wav_data_size = sample_count * sizeof(int16_t);
    uint32_t wav_file_size = 44 + wav_data_size; // WAV header + data
    
    // Create WAV header
    AIFFWAVHeader wav_header;
    memcpy(wav_header.riff, "RIFF", 4);
    wav_header.file_length = wav_file_size - 8;
    memcpy(wav_header.wave, "WAVE", 4);
    memcpy(wav_header.fmt, "fmt ", 4);
    wav_header.fmt_length = 16;
    wav_header.audio_format = 1; // PCM
    wav_header.num_channels = aiff_info.channels;
    wav_header.sample_rate = (uint32_t)aiff_info.sample_rate;
    wav_header.byte_rate = wav_header.sample_rate * wav_header.num_channels * 2; // 16-bit
    wav_header.block_align = wav_header.num_channels * 2; // 16-bit
    wav_header.bits_per_sample = 16;
    memcpy(wav_header.data, "data", 4);
    wav_header.data_length = wav_data_size;
    
    // Create virtual file and write WAV data
    VirtualFile* vf = create_virtual_file(virtual_filename);
    if (!vf) {
        printf("Cannot create virtual WAV file: %s\n", virtual_filename);
        free(samples);
        return false;
    }
    
    // Write WAV header
    if (!virtual_file_write(vf, &wav_header, sizeof(wav_header))) {
        printf("Failed to write WAV header to virtual file\n");
        free(samples);
        return false;
    }
    
    // Write audio data
    if (!virtual_file_write(vf, samples, wav_data_size)) {
        printf("Failed to write audio data to virtual file\n");
        free(samples);
        return false;
    }
    
    free(samples);
    
    // Add to cache after successful conversion
    add_to_conversion_cache(&player->conversion_cache, filename, virtual_filename);
    
    printf("AIFF conversion to virtual file complete\n");
    return true;
}
