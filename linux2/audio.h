// audio.h
// Header file for SDL-based audio system with DBOPL integration

#ifndef AUDIO_H
#define AUDIO_H

#include <cstdint>

namespace Audio {
    // Initialize the audio system
    bool init();
    
    // Shut down the audio system
    void shutdown();
    
    // Write to OPL register
    void writeOPL(uint32_t reg, uint8_t val);
    
    // Update OPL emulation and generate audio
    void updateOPL();
    
    // Get the number of samples remaining in the queue
    int queueRemaining();
    
    // Get/set the sample rate
    int getSampleRate();
    void setSampleRate(int rate);
    
    // Get/set the buffer size
    int getBufferSize();
    void setBufferSize(int size);
}

#endif // AUDIO_H
