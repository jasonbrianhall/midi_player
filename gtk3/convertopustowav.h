#ifndef CONVERTOPUSTOWAV_H
#define CONVERTOPUSTOWAV_H

#ifdef __cplusplus
extern "C" {
#include <vector>
#endif

bool convertOpusToWav(const char* opus_filename, const char* wav_filename);

#ifdef __cplusplus
}
bool convertOpusToWavInMemory(const std::vector<uint8_t>& opus_data, std::vector<uint8_t>& wav_data);
#endif

#endif // CONVERTOPUSTOWAV_H
