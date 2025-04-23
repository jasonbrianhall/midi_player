#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dbopl_wrapper.h"
#include "dbopl.h"

// C linkage to work with the C code
extern "C" {
#include "midiplayer.h"
}

// The DBOPL emulator handler
static DBOPL::Handler opl_handler;

// Stereo audio buffer for OPL output
static int32_t opl_buffer[1024 * 2];

// OPL channel allocation and state tracking
static struct {
    bool active;
    int midi_channel;
    int midi_note;
    int instrument;
    int velocity;
    int pan;
} opl_channels[18];

// Track MIDI channel state
static int midi_channel_program[16] = {0};
static int midi_channel_volume[16] = {127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127};
static int midi_channel_pan[16] = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};

// Initialize the OPL emulator
extern "C" void OPL_Init(int sample_rate) {
    // Initialize the OPL emulator
    opl_handler.Init(sample_rate);
    
    // Reset all channels
    memset(opl_channels, 0, sizeof(opl_channels));
    
    // Set OPL3 mode
    opl_handler.WriteReg(0x105, 0x01);
    
    // Initialize the instruments (from the original MIDI player)
    for (int i = 0; i < 18; i++) {
        opl_channels[i].active = false;
    }
}

// Reset the OPL emulator
extern "C" void OPL_Reset(void) {
    // Turn off all notes
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].active) {
            // Key off
            uint32_t reg_offset = (i % 9);
            uint32_t bank = (i / 9);
            uint32_t reg_b0 = 0xB0 + reg_offset + (bank * 0x100);
            
            // Get current value and clear key-on bit
            uint8_t current_val = opl_handler.chip.regBD;  // Get current value
            opl_handler.WriteReg(reg_b0, current_val & 0xDF); // Clear key-on bit
            
            opl_channels[i].active = false;
        }
    }
}

// Write to OPL register
extern "C" void OPL_WriteReg(uint32_t reg, uint8_t value) {
    opl_handler.WriteReg(reg, value);
}

// Generate audio samples
extern "C" void OPL_Generate(int16_t *buffer, int num_samples) {
    // Clear the buffer
    memset(opl_buffer, 0, num_samples * 2 * sizeof(int32_t));
    
    // Generate OPL audio
    opl_handler.Generate(opl_buffer, num_samples);
    
    // Convert to 16-bit and apply volume scaling
    for (int i = 0; i < num_samples * 2; i++) {
        // Scale down to 16-bit range and apply master volume
        int32_t sample = opl_buffer[i] / 4;
        
        // Clip to 16-bit range
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;
        
        buffer[i] = (int16_t)sample;
    }
}

// Clean up OPL resources
extern "C" void OPL_Shutdown(void) {
    // Nothing specific to clean up with DBOPL
}

// Find a free OPL channel for a new note
static int allocate_opl_channel(int midi_channel, int note) {
    // First try to find an inactive channel
    for (int i = 0; i < 18; i++) {
        if (!opl_channels[i].active) {
            return i;
        }
    }
    
    // If no free channels, find the oldest channel of the same MIDI channel
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].midi_channel == midi_channel) {
            return i;
        }
    }
    
    // If still no channel, just take the first one
    return 0;
}

// Load an FM instrument into an OPL channel
static void load_instrument(int opl_channel, int instrument) {
    uint32_t reg_offset = (opl_channel % 9);
    uint32_t bank = (opl_channel / 9);
    
    // Modulator
    OPL_WriteReg(0x20 + reg_offset + (bank * 0x100), adl[instrument].modChar1);
    OPL_WriteReg(0x40 + reg_offset + (bank * 0x100), adl[instrument].modChar2);
    OPL_WriteReg(0x60 + reg_offset + (bank * 0x100), adl[instrument].modChar3);
    OPL_WriteReg(0x80 + reg_offset + (bank * 0x100), adl[instrument].modChar4);
    OPL_WriteReg(0xE0 + reg_offset + (bank * 0x100), adl[instrument].modChar5);
    
    // Carrier
    OPL_WriteReg(0x23 + reg_offset + (bank * 0x100), adl[instrument].carChar1);
    OPL_WriteReg(0x43 + reg_offset + (bank * 0x100), adl[instrument].carChar2);
    OPL_WriteReg(0x63 + reg_offset + (bank * 0x100), adl[instrument].carChar3);
    OPL_WriteReg(0x83 + reg_offset + (bank * 0x100), adl[instrument].carChar4);
    OPL_WriteReg(0xE3 + reg_offset + (bank * 0x100), adl[instrument].carChar5);
    
    // Feedback/Connection
    OPL_WriteReg(0xC0 + reg_offset + (bank * 0x100), adl[instrument].fbConn);
}

// Set the frequency for a note
static void set_note_frequency(int opl_channel, int note, bool keyon) {
    uint32_t reg_offset = (opl_channel % 9);
    uint32_t bank = (opl_channel / 9);
    
    // Calculate frequency number and block
    double freq = 440.0 * pow(2.0, (note - 69) / 12.0);
    int block = (note / 12) - 1;
    if (block < 0) block = 0;
    if (block > 7) block = 7;
    
    // Calculate F-Number
    double fnumVal = freq * (1 << (20 - block)) / 49716.0;
    int fnum = (int)fnumVal;
    
    // Frequency low byte
    OPL_WriteReg(0xA0 + reg_offset + (bank * 0x100), fnum & 0xFF);
    
    // Frequency high bits and keyon
    uint8_t regval = ((block & 7) << 2) | ((fnum >> 8) & 3);
    if (keyon) {
        regval |= 0x20; // Set key-on bit
    }
    OPL_WriteReg(0xB0 + reg_offset + (bank * 0x100), regval);
}

// Set volume for an OPL channel
static void set_channel_volume(int opl_channel, int velocity, int volume) {
    uint32_t reg_offset = (opl_channel % 9);
    uint32_t bank = (opl_channel / 9);
    int instrument = opl_channels[opl_channel].instrument;
    
    // Scale volume based on velocity and channel volume
    int scaled_vol = (velocity * volume) / 127;
    
    // Calculate operator attenuation
    int mod_level = adl[instrument].modChar2 & 0x3F;
    int car_level = adl[instrument].carChar2 & 0x3F;
    
    // Apply velocity scaling to carrier level (main volume)
    // Higher value = more attenuation = less volume
    int scaled_car_level = car_level + (63 - scaled_vol) / 2;
    if (scaled_car_level > 63) scaled_car_level = 63;
    
    // Apply velocity scaling to modulator (softer effect)
    int scaled_mod_level = mod_level + (63 - scaled_vol) / 4;
    if (scaled_mod_level > 63) scaled_mod_level = 63;
    
    // Update registers
    uint8_t car_reg_val = (adl[instrument].carChar2 & 0xC0) | scaled_car_level;
    uint8_t mod_reg_val = (adl[instrument].modChar2 & 0xC0) | scaled_mod_level;
    
    OPL_WriteReg(0x40 + reg_offset + (bank * 0x100), mod_reg_val);
    OPL_WriteReg(0x43 + reg_offset + (bank * 0x100), car_reg_val);
}

// Set panning for an OPL channel
static void set_channel_pan(int opl_channel, int pan) {
    uint32_t reg_offset = (opl_channel % 9);
    uint32_t bank = (opl_channel / 9);
    int instrument = opl_channels[opl_channel].instrument;
    
    // Get the base feedback/connection value
    uint8_t fb_conn = adl[instrument].fbConn;
    
    // OPL3 panning bits:
    // D5 (0x20) = Right speaker
    // D4 (0x10) = Left speaker
    uint8_t panning = 0x30; // Default: both speakers
    
    if (pan < 48) {
        panning = 0x10; // Left only
    } else if (pan > 80) {
        panning = 0x20; // Right only
    }
    
    // Preserve feedback bits and add panning
    uint8_t new_fb_conn = (fb_conn & 0x0F) | panning;
    
    OPL_WriteReg(0xC0 + reg_offset + (bank * 0x100), new_fb_conn);
}

// Helper functions for MIDI player

extern "C" void OPL_NoteOn(int channel, int note, int velocity) {
    // Determine which instrument to use
    int instrument = (channel == 9) ? 128 + note - 35 : midi_channel_program[channel];
    
    // Make sure the instrument number is valid
    if (instrument < 0) instrument = 0;
    if (instrument >= 181) instrument = 0;
    
    // Allocate an OPL channel
    int opl_channel = allocate_opl_channel(channel, note);
    
    // If a note is already playing on this OPL channel, turn it off
    if (opl_channels[opl_channel].active) {
        set_note_frequency(opl_channel, opl_channels[opl_channel].midi_note, false);
    }
    
    // Set up the new note
    opl_channels[opl_channel].active = true;
    opl_channels[opl_channel].midi_channel = channel;
    opl_channels[opl_channel].midi_note = note;
    opl_channels[opl_channel].instrument = instrument;
    opl_channels[opl_channel].velocity = velocity;
    
    // Configure the OPL channel
    load_instrument(opl_channel, instrument);
    set_channel_volume(opl_channel, velocity, midi_channel_volume[channel]);
    set_channel_pan(opl_channel, midi_channel_pan[channel]);
    
    // Set the frequency and key it on
    set_note_frequency(opl_channel, note, true);
}

extern "C" void OPL_NoteOff(int channel, int note) {
    // Find the OPL channel playing this note
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].active && 
            opl_channels[i].midi_channel == channel && 
            opl_channels[i].midi_note == note) {
            
            // Turn off the note
            set_note_frequency(i, note, false);
            opl_channels[i].active = false;
            break;
        }
    }
}

extern "C" void OPL_ProgramChange(int channel, int program) {
    // Store the program number for this MIDI channel
    midi_channel_program[channel] = program;
    
    // Update any currently playing notes on this channel
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].active && opl_channels[i].midi_channel == channel) {
            // If it's not a percussion channel, update the instrument
            if (channel != 9) {
                opl_channels[i].instrument = program;
                load_instrument(i, program);
                
                // Reapply the volume and pan settings
                set_channel_volume(i, opl_channels[i].velocity, midi_channel_volume[channel]);
                set_channel_pan(i, midi_channel_pan[channel]);
            }
        }
    }
}

extern "C" void OPL_SetPan(int channel, int pan) {
    // Store the pan setting for this MIDI channel
    midi_channel_pan[channel] = pan;
    
    // Update any currently playing notes on this channel
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].active && opl_channels[i].midi_channel == channel) {
            set_channel_pan(i, pan);
        }
    }
}

extern "C" void OPL_SetVolume(int channel, int volume) {
    // Store the volume setting for this MIDI channel
    midi_channel_volume[channel] = volume;
    
    // Update any currently playing notes on this channel
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].active && opl_channels[i].midi_channel == channel) {
            set_channel_volume(i, opl_channels[i].velocity, volume);
        }
    }
}

extern "C" void OPL_SetPitchBend(int channel, int bend) {
    // Pitch bend is more complex with OPL - we'd need to recalculate frequencies
    // This is a simplified implementation
    for (int i = 0; i < 18; i++) {
        if (opl_channels[i].active && opl_channels[i].midi_channel == channel) {
            // Calculate a note offset based on the bend
           // Bend range: -8192 to 8191, typically ±2 semitones
           double bend_amount = (bend - 8192) / 8192.0;
           double semitones = bend_amount * 2.0; // ±2 semitone range
           
           // Calculate the adjusted frequency
           double note = opl_channels[i].midi_note + semitones;
           
           // Update the frequency but keep note on
           set_note_frequency(i, (int)round(note), true);
       }
   }
}            
