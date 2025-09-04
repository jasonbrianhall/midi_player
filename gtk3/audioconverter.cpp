#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cstring>
#include "midiplayer.h"
#include "dbopl_wrapper.h"

#ifdef WIN32
#include <windows.h>
#endif

// External variables and functions needed from midiplayer.cpp
extern double playTime;
extern bool isPlaying;
extern double playwait;
extern int globalVolume;
extern void processEvents();

bool convertMidiToWav(const char* midi_filename, const char* wav_filename, int volume);

// In-memory MIDI to WAV conversion function
#ifndef WIN32
bool convertMidiToWavInMemory(const std::vector<uint8_t>& midiData, std::vector<uint8_t>& wavData) {
    // Create a temporary file to write the MIDI data
    char tempMidiPath[L_tmpnam] = {0};
    
    // Use tmpnam for non-Windows platforms
    if (std::tmpnam(tempMidiPath) == NULL) {
        std::cerr << "Failed to create temporary MIDI file path" << std::endl;
        return false;
    }

    // Write MIDI data to temp file
    FILE* tempMidi = fopen(tempMidiPath, "wb");
    if (!tempMidi) {
        std::cerr << "Failed to open temporary MIDI file for writing" << std::endl;
        return false;
    }
    
    fwrite(midiData.data(), 1, midiData.size(), tempMidi);
    fclose(tempMidi);
    
    // Create a temporary file for the WAV output
    char tempWavPath[L_tmpnam] = {0};
    
    // Use tmpnam for non-Windows platforms
    if (std::tmpnam(tempWavPath) == NULL) {
        std::cerr << "Failed to create temporary WAV file path" << std::endl;
        remove(tempMidiPath);
        return false;
    }
    
    // Use the existing convertMidiToWav function
    bool conversionSuccess = convertMidiToWav(tempMidiPath, tempWavPath, 1000);
    
    if (!conversionSuccess) {
        std::cerr << "MIDI to WAV conversion failed" << std::endl;
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Read the resulting WAV file into memory
    FILE* wavFile = fopen(tempWavPath, "rb");
    if (!wavFile) {
        std::cerr << "Failed to open temporary WAV file for reading" << std::endl;
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Get the WAV file size
    fseek(wavFile, 0, SEEK_END);
    long wavSize = ftell(wavFile);
    fseek(wavFile, 0, SEEK_SET);
    
    if (wavSize <= 0) {
        std::cerr << "WAV file is empty or invalid" << std::endl;
        fclose(wavFile);
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Instead of just reading the file, let's reconstruct the WAV with our standard format
    // Read the WAV header first (44 bytes)
    uint8_t header[44];
    size_t headerBytesRead = fread(header, 1, 44, wavFile);
    
    if (headerBytesRead != 44) {
        std::cerr << "Failed to read WAV header" << std::endl;
        fclose(wavFile);
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Validate it's a WAV file
    if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F' ||
        header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E') {
        std::cerr << "Invalid WAV file format" << std::endl;
        fclose(wavFile);
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Extract the data size from the WAV file
    uint32_t dataSize = wavSize - 44; // Total file size minus header size
    
    // Read the audio data
    std::vector<uint8_t> audioData(dataSize);
    size_t dataBytesRead = fread(audioData.data(), 1, dataSize, wavFile);
    fclose(wavFile);
    
    if (dataBytesRead != dataSize) {
        std::cerr << "Failed to read WAV data" << std::endl;
        remove(tempMidiPath);
        remove(tempWavPath);
        return false;
    }
    
    // Create a new WAV header that matches the MP3 conversion format
    struct WavHeader {
        // RIFF header
        char riff_header[4] = {'R', 'I', 'F', 'F'};
        uint32_t wav_size;        // File size - 8
        char wave_header[4] = {'W', 'A', 'V', 'E'};
        
        // Format chunk
        char fmt_header[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo
        uint32_t sample_rate = 44100; // 44.1kHz
        uint32_t byte_rate = sample_rate * num_channels * 2; // bytes per second
        uint16_t block_align = num_channels * 2; // bytes per sample
        uint16_t bits_per_sample = 16; // 16-bit
        
        // Data chunk
        char data_header[4] = {'d', 'a', 't', 'a'};
        uint32_t data_bytes;      // Size of data
    };
    
    // Fill in the header with the correct values
    WavHeader newHeader;
    newHeader.data_bytes = dataSize;
    newHeader.wav_size = 36 + dataSize; // 36 is the size of the header excluding RIFF header
    
    // Create the final WAV data with our new header + the audio data
    wavData.resize(sizeof(newHeader) + dataSize);
    
    // Copy the header to the beginning of wavData
    std::memcpy(wavData.data(), &newHeader, sizeof(newHeader));
    
    // Copy the PCM data after the header
    std::memcpy(wavData.data() + sizeof(newHeader), audioData.data(), dataSize);
    
    // Clean up temporary files
    //remove(tempMidiPath);
    //remove(tempWavPath);
    
    std::cerr << "MIDI to WAV conversion successful, created WAV with " << dataSize 
              << " bytes of audio data" << std::endl;
    
    return true;
}
#else // For Windows platforms
bool convertMidiToWavInMemory(const std::vector<uint8_t>& midiData, std::vector<uint8_t>& wavData) {
    // Create a temporary file to write the MIDI data
    char tempDir[MAX_PATH];
    char tempMidiPath[MAX_PATH];
    char tempWavPath[MAX_PATH];
    
    // Get the temporary directory path
    if (GetTempPathA(MAX_PATH, tempDir) == 0) {
        std::cerr << "Failed to get temporary directory path" << std::endl;
        return false;
    }
    
    // Create temporary file names
    if (GetTempFileNameA(tempDir, "mid", 0, tempMidiPath) == 0) {
        std::cerr << "Failed to create temporary MIDI file name" << std::endl;
        return false;
    }
    
    if (GetTempFileNameA(tempDir, "wav", 0, tempWavPath) == 0) {
        std::cerr << "Failed to create temporary WAV file name" << std::endl;
        DeleteFileA(tempMidiPath);
        return false;
    }
    
    // Write MIDI data to temp file
    FILE* tempMidi = fopen(tempMidiPath, "wb");
    if (!tempMidi) {
        std::cerr << "Failed to open temporary MIDI file for writing" << std::endl;
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    fwrite(midiData.data(), 1, midiData.size(), tempMidi);
    fclose(tempMidi);
    
    // Use the existing convertMidiToWav function
    bool conversionSuccess = convertMidiToWav(tempMidiPath, tempWavPath, 1000);
    
    if (!conversionSuccess) {
        std::cerr << "MIDI to WAV conversion failed" << std::endl;
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    // Read the resulting WAV file into memory
    FILE* wavFile = fopen(tempWavPath, "rb");
    if (!wavFile) {
        std::cerr << "Failed to open temporary WAV file for reading" << std::endl;
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    // Get the WAV file size
    fseek(wavFile, 0, SEEK_END);
    long wavSize = ftell(wavFile);
    fseek(wavFile, 0, SEEK_SET);
    
    if (wavSize <= 0) {
        std::cerr << "WAV file is empty or invalid" << std::endl;
        fclose(wavFile);
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    // Read the WAV header first (44 bytes)
    uint8_t header[44];
    size_t headerBytesRead = fread(header, 1, 44, wavFile);
    
    if (headerBytesRead != 44) {
        std::cerr << "Failed to read WAV header" << std::endl;
        fclose(wavFile);
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    // Validate it's a WAV file
    if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F' ||
        header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E') {
        std::cerr << "Invalid WAV file format" << std::endl;
        fclose(wavFile);
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    // Extract the data size from the WAV file
    uint32_t dataSize = wavSize - 44; // Total file size minus header size
    
    // Read the audio data
    std::vector<uint8_t> audioData(dataSize);
    size_t dataBytesRead = fread(audioData.data(), 1, dataSize, wavFile);
    fclose(wavFile);
    
    if (dataBytesRead != dataSize) {
        std::cerr << "Failed to read WAV data" << std::endl;
        DeleteFileA(tempMidiPath);
        DeleteFileA(tempWavPath);
        return false;
    }
    
    // Create a new WAV header that matches the MP3 conversion format
    struct WavHeader {
        // RIFF header
        char riff_header[4] = {'R', 'I', 'F', 'F'};
        uint32_t wav_size;        // File size - 8
        char wave_header[4] = {'W', 'A', 'V', 'E'};
        
        // Format chunk
        char fmt_header[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo
        uint32_t sample_rate = 44100; // 44.1kHz
        uint32_t byte_rate = sample_rate * num_channels * 2; // bytes per second
        uint16_t block_align = num_channels * 2; // bytes per sample
        uint16_t bits_per_sample = 16; // 16-bit
        
        // Data chunk
        char data_header[4] = {'d', 'a', 't', 'a'};
        uint32_t data_bytes;      // Size of data
    };
    
    // Fill in the header with the correct values
    WavHeader newHeader;
    newHeader.data_bytes = dataSize;
    newHeader.wav_size = 36 + dataSize; // 36 is the size of the header excluding RIFF header
    
    // Create the final WAV data with our new header + the audio data
    wavData.resize(sizeof(newHeader) + dataSize);
    
    // Copy the header to the beginning of wavData
    std::memcpy(wavData.data(), &newHeader, sizeof(newHeader));
    
    // Copy the PCM data after the header
    std::memcpy(wavData.data() + sizeof(newHeader), audioData.data(), dataSize);
    
    // Clean up temporary files
    DeleteFileA(tempMidiPath);
    DeleteFileA(tempWavPath);
    
    std::cerr << "MIDI to WAV conversion successful, created WAV with " << dataSize 
              << " bytes of audio data" << std::endl;
    
    return true;
}
#endif


// Convert MP3 to WAV using SDL2_mixer
bool convertMp3ToWavInMemory(const std::vector<uint8_t>& mp3Data, std::vector<uint8_t>& wavData) {
    // Initialize SDL and SDL_mixer if not already initialized
    static bool sdl_initialized = false;
    if (!sdl_initialized) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_mixer with default frequency, format, channels, and chunksize
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
            return false;
        }
        
        sdl_initialized = true;
    }
    
    // Create an SDL_RWops from the MP3 data
    SDL_RWops* rw = SDL_RWFromMem((void*)mp3Data.data(), mp3Data.size());
    if (!rw) {
        std::cerr << "Failed to create RWops from memory! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Load the MP3 as a chunk (using SDL_mixer)
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 0); // 0 means don't free the RWops when done
    
    // Make sure to close the RWops
    SDL_RWclose(rw);
    
    if (!chunk) {
        std::cerr << "Failed to load MP3! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    
    // At this point, SDL_mixer has decoded the MP3 to raw PCM data
    // chunk->abuf contains the audio data and chunk->alen is the length
    
    // Create a minimal WAV header
    struct WavHeader {
        // RIFF header
        char riff_header[4] = {'R', 'I', 'F', 'F'};
        uint32_t wav_size;        // File size - 8
        char wave_header[4] = {'W', 'A', 'V', 'E'};
        
        // Format chunk
        char fmt_header[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo (SDL_mixer default)
        uint32_t sample_rate = 44100; // SDL_mixer default
        uint32_t byte_rate = sample_rate * num_channels * 2; // bytes per second
        uint16_t block_align = num_channels * 2; // bytes per sample
        uint16_t bits_per_sample = 16; // SDL_mixer default
        
        // Data chunk
        char data_header[4] = {'d', 'a', 't', 'a'};
        uint32_t data_bytes;      // chunk->alen
    };
    
    // Fill in the header with the correct values
    WavHeader header;
    header.data_bytes = chunk->alen;
    header.wav_size = 36 + header.data_bytes; // 36 is the size of the header excluding RIFF header
    
    // Create the WAV data with header + audio data
    wavData.resize(sizeof(header) + chunk->alen);
    
    // Copy the header to the beginning of wavData
    std::memcpy(wavData.data(), &header, sizeof(header));
    
    // Copy the PCM data after the header
    std::memcpy(wavData.data() + sizeof(header), chunk->abuf, chunk->alen);
    
    // Free the chunk
    Mix_FreeChunk(chunk);
    
    return true;
}
