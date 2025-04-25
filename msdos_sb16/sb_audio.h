#ifndef SB_AUDIO_H
#define SB_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// SoundBlaster I/O ports (standard addresses)
#define SB_BASE_ADDRESS      0x220       // Default base address
#define SB_MIXER_PORT        0x224       // Mixer port
#define SB_MIXER_DATA        0x225       // Mixer data port
#define SB_RESET_PORT        0x226       // DSP reset port
#define SB_READ_PORT         0x22A       // DSP read port
#define SB_WRITE_PORT        0x22C       // DSP write port
#define SB_READ_STATUS_PORT  0x22E       // Read status port
#define SB_IRQ               5           // Default IRQ
#define SB_DMA               1           // Default DMA channel

// SoundBlaster commands
#define SB_DSP_VERSION       0xE1        // Get DSP version
#define SB_SPEAKER_ON        0xD1        // Turn speaker on
#define SB_SPEAKER_OFF       0xD3        // Turn speaker off
#define SB_STOP_DMA          0xD0        // Stop DMA transfer
#define SB_PAUSE_DMA         0xD0        // Pause DMA transfer
#define SB_CONTINUE_DMA      0xD4        // Continue DMA transfer
#define SB_GET_SPEAKER       0xD8        // Get speaker status

// SB16-specific commands
#define SB_SET_SAMPLE_RATE   0x41        // Set sample rate (output)
#define SB_SET_INPUT_RATE    0x42        // Set sample rate (input)
#define SB_OUTPUT_16BIT      0xB6        // 16-bit output
#define SB_OUTPUT_8BIT       0xC6        // 8-bit output
#define SB_INPUT_16BIT       0xBE        // 16-bit input
#define SB_INPUT_8BIT        0xCE        // 8-bit input

// SB buffer parameters
#define SB_BUFFER_SIZE       4096        // Default DMA buffer size
#define SB_DMA_BUFFER_COUNT  2           // Number of DMA buffers (double buffering)

// Function prototypes
bool SB_Init(int base_address, int irq, int dma, int sample_rate, bool stereo, bool use_16bit);
void SB_Shutdown(void);
bool SB_Reset(void);
bool SB_SetSampleRate(int rate);
bool SB_StartPlayback(void);
bool SB_StopPlayback(void);
void SB_FillBuffer(int16_t *buffer, int num_samples);
bool SB_IsPlaying(void);
void SB_SetCallbackFunction(void (*callback)(void*, uint8_t*, int), void* userdata);
void SB_WaitForIRQ(void);
int SB_GetDSPVersion(void);
void SB_UpdateBuffer(void);

// Use DJGPP's built-in functions instead of these
// void outportb(uint16_t port, uint8_t value);
// uint8_t inportb(uint16_t port);

// Renamed to avoid conflicts with DOS.H
void ms_delay(int milliseconds);

#ifdef __cplusplus
}
#endif

#endif // SB_AUD
