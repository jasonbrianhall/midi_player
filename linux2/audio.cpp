#include "audio.h"
#include "dbopl.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>
#include <vector>

// Static variables
static SDL_AudioDeviceID audioDevice;
static DBOPL::Handler oplHandler;
static int sampleRate = 44100;
static int bufferSize = 512;

// Simple buffer for audio
static std::vector<int16_t> audioBuffer;

// Very basic audio callback
void audioCallback(void* userdata, Uint8* stream, int len) {
    // Clear the stream to silence
    SDL_memset(stream, 0, len);
    
    // Directly copy our buffer to the stream if available
    if (audioBuffer.size() * sizeof(int16_t) >= (size_t)len) {
        SDL_memcpy(stream, audioBuffer.data(), len);
    }
}

bool Audio::init() {
    // Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set up a very simple SDL audio spec
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = sampleRate;
    want.format = AUDIO_S16;
    want.channels = 1;
    want.samples = bufferSize;
    want.callback = audioCallback;
    
    // Open audio device with minimal requirements
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize OPL emulator
    oplHandler.Init(sampleRate);
    
    // Pre-allocate audio buffer
    audioBuffer.resize(bufferSize);
    
    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
    
    std::cout << "Audio initialized - Sample rate: " << have.freq << std::endl;
    return true;
}

void Audio::shutdown() {
    // Stop and clean up SDL audio
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void Audio::writeOPL(uint32_t reg, uint8_t val) {
    // Write to the OPL emulator
    uint32_t port = 0;
    if (reg & 0x100) {
        port = 2; // High register set in OPL3
    }
    
    // Pass the register write to the OPL emulator
    oplHandler.WriteReg(oplHandler.WriteAddr(port, reg & 0xff), val);
}

void Audio::updateOPL() {
    // Use fixed buffer size to avoid any potential overflow
    const int size = 256; // Smaller buffer for safety
    
    // Generate audio with bounds checking
    try {
        int32_t tempBuffer[size];
        
        // Clear buffer first
        memset(tempBuffer, 0, size * sizeof(int32_t));
        
        // Generate OPL output samples with careful size management
        oplHandler.Generate(tempBuffer, size);
        
        // Lock audio while we update
        SDL_LockAudioDevice(audioDevice);
        
        // Resize the buffer if needed
        if (audioBuffer.size() < size) {
            audioBuffer.resize(size);
        }
        
        // Convert to 16-bit with simple scaling and bounds checking
        for (int i = 0; i < size; i++) {
            int32_t sample = tempBuffer[i] / 32;
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            audioBuffer[i] = static_cast<int16_t>(sample);
        }
        
        SDL_UnlockAudioDevice(audioDevice);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateOPL: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in updateOPL" << std::endl;
    }
}

int Audio::queueRemaining() {
    return bufferSize; // Simplified
}

int Audio::getSampleRate() {
    return sampleRate;
}

void Audio::setSampleRate(int rate) {
    sampleRate = rate;
}

int Audio::getBufferSize() {
    return bufferSize;
}

void Audio::setBufferSize(int size) {
    bufferSize = size;
}
