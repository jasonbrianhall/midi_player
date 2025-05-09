/*
 * Minimal SDL2 implementation for MS-DOS using DJGPP
 * Only implements the functions needed for MIDI-to-WAV conversion
 */
#ifndef SDL_MINIMAL_H
#define SDL_MINIMAL_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pc.h>      /* DJGPP-specific header for port I/O */
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/segments.h>
#include <conio.h>
#include <string.h>

/* Basic SDL types */
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;

/* SDL Audio formats */
#define AUDIO_U8     0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8     0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16    0x8010  /**< Signed 16-bit samples */
#define AUDIO_S16LSB 0x8010  /**< Signed 16-bit samples (little-endian) */
#define AUDIO_S16MSB 0x9010  /**< Signed 16-bit samples (big-endian) */

/* SDL Initialization flags */
#define SDL_INIT_AUDIO    0x00000010
#define SDL_INIT_TIMER    0x00000001

/* SDL Audio device ID type */
typedef int SDL_AudioDeviceID;

/* SDL Audio specification structure */
typedef struct {
    int freq;               /**< DSP frequency -- samples per second */
    Uint16 format;          /**< Audio data format */
    Uint8 channels;         /**< Number of channels: 1 mono, 2 stereo */
    Uint8 silence;          /**< Audio buffer silence value (calculated) */
    Uint16 samples;         /**< Audio buffer size in samples */
    Uint16 padding;         /**< Necessary for some compile environments */
    Uint32 size;            /**< Audio buffer size in bytes (calculated) */
    void (*callback)(void *userdata, Uint8 *stream, int len); /**< Callback function */
    void *userdata;         /**< Userdata passed to callback function */
} SDL_AudioSpec;

/* SDL Error handling */
const char* SDL_GetError(void);

/* SDL Initialization and shutdown */
int SDL_Init(Uint32 flags);
void SDL_Quit(void);

/* SDL Audio functions */
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device,
                                     int iscapture,
                                     const SDL_AudioSpec *desired,
                                     SDL_AudioSpec *obtained,
                                     int allowed_changes);
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev);

/* SDL Timing functions */
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);

/* SDL Memory */
#define SDL_zero(x) memset(&(x), 0, sizeof(x))

/* Implementation helpers */
void _sdl_init_timer(void);
void _sdl_exit_timer(void);

/* DJGPP has its own delay function that we'll use */
#ifndef delay
#include <unistd.h>
#define delay(ms) usleep((ms) * 1000)
#endif

#endif /* SDL_MINIMAL_H */
