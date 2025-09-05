#ifndef VIRTUAL_FILESYSTEM_H
#define VIRTUAL_FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Virtual file structure
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
    size_t position;
} VirtualFile;

// Virtual WAV converter structure
typedef struct {
    VirtualFile* vf;
    int sample_rate;
    int channels;
    size_t samples_written;
} VirtualWAVConverter;

// Core virtual filesystem functions
void init_virtual_filesystem();
void cleanup_virtual_filesystem();
VirtualFile* create_virtual_file(const char* filename);
VirtualFile* get_virtual_file(const char* filename);
bool delete_virtual_file(const char* filename);

// Virtual file operations
bool virtual_file_write(VirtualFile* vf, const void* data, size_t size);
size_t virtual_file_read(VirtualFile* vf, void* buffer, size_t size);
bool virtual_file_seek(VirtualFile* vf, long offset, int whence);
long virtual_file_tell(VirtualFile* vf);
size_t virtual_file_size(VirtualFile* vf);

// Virtual WAV converter functions
VirtualWAVConverter* virtual_wav_converter_init(const char* filename, int sample_rate, int channels);
bool virtual_wav_converter_write(VirtualWAVConverter* converter, int16_t* samples, size_t count);
void virtual_wav_converter_finish(VirtualWAVConverter* converter);
void virtual_wav_converter_free(VirtualWAVConverter* converter);
bool load_virtual_wav_file(AudioPlayer *player, const char* virtual_filename);

// Helper function for OGG conversion (you'll need to implement this)
bool convertOggToVirtualWav(const char* ogg_filename, const char* virtual_wav_filename);

#endif // VIRTUAL_FILESYSTEM_H
