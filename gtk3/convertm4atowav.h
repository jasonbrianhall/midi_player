#ifndef CONVERTM4ATOWAV_H
#define CONVERTM4ATOWAV_H

#include <stdint.h>
#include <vector>

// Convert M4A data in memory to WAV data in memory
bool convertM4aToWavInMemory(const std::vector<uint8_t>& m4a_data, std::vector<uint8_t>& wav_data);

// Convert M4A file to WAV file (alternative implementation if needed)
bool convertM4aToWav(const char* input_filename, const char* output_filename);

#endif // CONVERTM4ATOWAV_H
