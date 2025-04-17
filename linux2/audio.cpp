// audio.cpp
// SDL-based audio output for MIDI player with DBOPL integration

#include "audio.h"
#include "dbopl.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>

// Global variables for audio system
static SDL_AudioDeviceID audioDevice;
static std::mutex audioMutex;
static std::vector<int16_t> audioBuffer;
static std::queue<int16_t> audioQueue;
static DBOPL::Handler oplHandler;
static int sampleRate = 44100;
static int bufferSize = 512;

// Callback function for SDL audio
void audioCallback(void* userdata, Uint8* stream, int len) {
    // Calculate how many samples we need to fill
    int sampleCount = len / sizeof(int16_t);
    int16_t* output = reinterpret_cast<int16_t*>(stream);
    
    std::lock_guard<std::mutex> lock(audioMutex);
    
    // Fill the buffer with data from the queue, or silence if queue is empty
    for (int i = 0; i < sampleCount; i++) {
        if (!audioQueue.empty()) {
            output[i] = audioQueue.front();
            audioQueue.pop();
        } else {
            output[i] = 0;
        }
    }
}

bool Audio::init() {
    // Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set up SDL audio
    SDL_AudioSpec wantedSpec, actualSpec;
    SDL_zero(wantedSpec);
    wantedSpec.freq = sampleRate;
    wantedSpec.format = AUDIO_S16;
    wantedSpec.channels = 1; // Mono output (we'll handle stereo ourselves)
    wantedSpec.samples = bufferSize;
    wantedSpec.callback = audioCallback;
    
    // Open audio device
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantedSpec, &actualSpec, 0);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize OPL emulator
    oplHandler.Init(sampleRate);
    
    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
    
    std::cout << "Audio initialized - Sample rate: " << actualSpec.freq << std::endl;
    return true;
}

void Audio::shutdown() {
    // Stop and clean up SDL audio
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    
    SDL_Quit();
}

void Audio::writeOPL(uint32_t reg, uint8_t val) {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    // Write to the OPL emulator
    uint32_t port = 0;
    if (reg & 0x100) {
        port = 2; // High register set in OPL3
    }
    
    // Pass the register write to the OPL emulator
    oplHandler.WriteReg(oplHandler.WriteAddr(port, reg & 0xff), val);
}

void Audio::updateOPL() {
    // Generate a buffer of audio data from the OPL emulator
    audioBuffer.resize(bufferSize);
    int32_t* tempBuffer = new int32_t[bufferSize];
    
    // Generate OPL output samples
    oplHandler.Generate(tempBuffer, bufferSize);
    
    // Lock the audio mutex before modifying the queue
    std::lock_guard<std::mutex> lock(audioMutex);
    
    // Convert 32-bit samples to 16-bit and add to the queue
    for (int i = 0; i < bufferSize; i++) {
        // Scale the volume and clip to 16-bit
        int32_t sample = tempBuffer[i] / 32;
        
        // Apply soft clipping to avoid harsh distortion
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        
        // Add the sample to the queue
        audioQueue.push(static_cast<int16_t>(sample));
    }
    
    delete[] tempBuffer;
}

int Audio::queueRemaining() {
    std::lock_guard<std::mutex> lock(audioMutex);
    return audioQueue.size();
}

// Audio namespace implementation
namespace Audio {
    int getSampleRate() {
        return sampleRate;
    }
    
    void setSampleRate(int rate) {
        sampleRate = rate;
    }
    
    int getBufferSize() {
        return bufferSize;
    }
    
    void setBufferSize(int size) {
        bufferSize = size;
    }
}
