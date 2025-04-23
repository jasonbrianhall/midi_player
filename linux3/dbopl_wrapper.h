#ifndef DBOPL_WRAPPER_H
#define DBOPL_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the OPL emulator
void OPL_Init(int sample_rate);

// Reset the OPL emulator
void OPL_Reset(void);

// Write to OPL register
void OPL_WriteReg(uint32_t reg, uint8_t value);

// Generate audio samples
void OPL_Generate(int16_t *buffer, int num_samples);

// Clean up OPL resources
void OPL_Shutdown(void);

// Helper functions for MIDI player
void OPL_NoteOn(int channel, int note, int velocity);
void OPL_NoteOff(int channel, int note);
void OPL_ProgramChange(int channel, int program);
void OPL_SetPan(int channel, int pan);
void OPL_SetVolume(int channel, int volume);
void OPL_SetPitchBend(int channel, int bend);

// Load instrument data from your existing instruments.c
extern void OPL_LoadInstruments(void);

#ifdef __cplusplus
}
#endif

#endif // DBOPL_WRAPPER_H
