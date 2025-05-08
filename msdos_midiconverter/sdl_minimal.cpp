/*
 * Minimal SDL2 implementation for MS-DOS with DJGPP
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pc.h>
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/segments.h>
#include <conio.h>
#include <stdarg.h>
#include "sdl_minimal.h"

/* Global state variables */
static bool sdl_initialized = false;
static bool timer_initialized = false;
static Uint32 start_ticks = 0;
static char error_message[256] = "No error";
static SDL_AudioSpec current_audio_spec;
static void (*current_audio_callback)(void *userdata, Uint8 *stream, int len) = NULL;
static void *current_userdata = NULL;
static bool audio_paused = true;

/* DOS timer variables */
static _go32_dpmi_seginfo old_timer_handler;
static _go32_dpmi_seginfo new_timer_handler;
static volatile Uint32 ticks_counter = 0;

/* Timer interrupt handler - 18.2 ticks per second in DOS */
static void timer_handler(_go32_dpmi_registers *r) {
    ticks_counter++;
    /* Chain to the old interrupt handler */
    _go32_dpmi_chain_protected_mode_interrupt_vector(0x08, &old_timer_handler);
}

/* Initialize the timer */
void _sdl_init_timer() {
    if (timer_initialized) return;
    
    /* Get the current timer interrupt handler */
    _go32_dpmi_get_protected_mode_interrupt_vector(0x08, &old_timer_handler);
    
    /* Set up our handler */
    new_timer_handler.pm_offset = (unsigned long)timer_handler;
    new_timer_handler.pm_selector = _go32_my_cs();
    _go32_dpmi_allocate_iret_wrapper(&new_timer_handler);
    _go32_dpmi_set_protected_mode_interrupt_vector(0x08, &new_timer_handler);
    
    timer_initialized = true;
}

/* Restore the original timer */
void _sdl_exit_timer() {
    if (!timer_initialized) return;
    
    /* Restore the original timer handler */
    _go32_dpmi_set_protected_mode_interrupt_vector(0x08, &old_timer_handler);
    _go32_dpmi_free_iret_wrapper(&new_timer_handler);
    
    timer_initialized = false;
}

/* Error handling function */
const char* SDL_GetError() {
    return error_message;
}

/* Set the error message */
static void SDL_SetError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(error_message, format, args);
    va_end(args);
}

/* Initialize SDL */
int SDL_Init(Uint32 flags) {
    if (sdl_initialized) {
        SDL_SetError("SDL already initialized");
        return -1;
    }
    
    /* Initialize timer if requested */
    if (flags & SDL_INIT_TIMER) {
        _sdl_init_timer();
    }
    
    /* Initialize start ticks for SDL_GetTicks() */
    start_ticks = time(NULL) * 1000;  /* Milliseconds since epoch */
    
    sdl_initialized = true;
    return 0;
}

/* Quit SDL */
void SDL_Quit() {
    if (!sdl_initialized) return;
    
    /* Cleanup timer */
    if (timer_initialized) {
        _sdl_exit_timer();
    }
    
    sdl_initialized = false;
}

/* Get current timestamp in milliseconds */
Uint32 SDL_GetTicks() {
    if (timer_initialized) {
        /* Convert 18.2Hz ticks to milliseconds */
        return (Uint32)((ticks_counter * 1000) / 18.2);
    } else {
        /* Use time() as fallback */
        return (Uint32)(time(NULL) * 1000 - start_ticks);
    }
}

/* Delay execution for given milliseconds */
void SDL_Delay(Uint32 ms) {
    /* Simple busy-wait delay */
    Uint32 start = SDL_GetTicks();
    while ((SDL_GetTicks() - start) < ms) {
        /* Yield CPU to prevent high usage in DJGPP */
        __dpmi_yield();
    }
}

/* Open audio device (simulated) */
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device,
                                     int iscapture,
                                     const SDL_AudioSpec *desired,
                                     SDL_AudioSpec *obtained,
                                     int allowed_changes) {
    /* We don't actually initialize hardware, just save the spec */
    if (!desired) {
        SDL_SetError("No audio specification provided");
        return 0;
    }
    
    /* Copy the desired spec to current and obtained */
    memcpy(&current_audio_spec, desired, sizeof(SDL_AudioSpec));
    
    /* Calculate buffer size based on format */
    int bytes_per_sample = 2;  /* Default for 16-bit */
    if (desired->format == AUDIO_U8 || desired->format == AUDIO_S8) {
        bytes_per_sample = 1;
    }
    
    current_audio_spec.size = desired->samples * bytes_per_sample * desired->channels;
    current_audio_spec.silence = (desired->format == AUDIO_U8) ? 128 : 0;
    
    /* Save the callback */
    current_audio_callback = desired->callback;
    current_userdata = desired->userdata;
    
    /* If requested, return the obtained spec */
    if (obtained) {
        memcpy(obtained, &current_audio_spec, sizeof(SDL_AudioSpec));
    }
    
    /* We don't actually use multiple devices, just return 1 as the device ID */
    return 1;
}

/* Pause/unpause audio device */
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on) {
    if (dev != 1) return;  /* We only support one device */
    
    audio_paused = (pause_on != 0);
    
    /* For conversion to WAV, we don't actually play audio, 
       so we don't need to do anything here */
}

/* Close audio device */
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev) {
    if (dev != 1) return;  /* We only support one device */
    
    /* Reset state */
    current_audio_callback = NULL;
    current_userdata = NULL;
    audio_paused = true;
}
