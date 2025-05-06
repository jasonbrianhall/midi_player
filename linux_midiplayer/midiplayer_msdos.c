#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <pc.h>        /* For inportb, outportb */
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <conio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <dos.h>        /* For _disable() and _enable() */
#include <go32.h>       /* For _go32_conventional_memory_start */
#include "midiplayer_msdos.h"
#include "dbopl_wrapper_msdos.h"

// Global variables
struct FMInstrument adl[181];
int globalVolume = 100;
bool enableNormalization = true;
bool isPlaying = false;
bool paused = false;

// MIDI file state
FILE* midiFile = NULL;
int TrackCount = 0;
int DeltaTicks = 0;
double Tempo = 500000;  // Default 120 BPM
double playTime = 0;

// Track state
int tkPtr[MAX_TRACKS] = {0};
double tkDelay[MAX_TRACKS] = {0};
int tkStatus[MAX_TRACKS] = {0};
bool loopStart = false;
bool loopEnd = false;
int loPtr[MAX_TRACKS] = {0};
double loDelay[MAX_TRACKS] = {0};
int loStatus[MAX_TRACKS] = {0};
double loopwait = 0;
int rbPtr[MAX_TRACKS] = {0};
double rbDelay[MAX_TRACKS] = {0};
int rbStatus[MAX_TRACKS] = {0};
double playwait = 0;

// MIDI channel state
int ChPatch[16] = {0};
double ChBend[16] = {0};
int ChVolume[16] = {127};
int ChPanning[16] = {0};
int ChVibrato[16] = {0};

// Sound Blaster variables
unsigned short sb_base_addr = 0;
unsigned char sb_irq = 7;  // Default IRQ
unsigned char sb_dma = 1;  // Default DMA channel
unsigned char sb_version_major = 0;
unsigned char sb_version_minor = 0;

// DMA buffer variables
#define DMA_BUFFER_SIZE 8192
unsigned char *dma_buffer = NULL;
unsigned char *dma_page_aligned_buffer = NULL;
volatile int buffer_position = 0;
volatile int dma_buffer_half = 0; // 0 = first half, 1 = second half

// DPMI variables for protected mode
_go32_dpmi_seginfo old_irq_handler;
_go32_dpmi_seginfo new_irq_handler;
_go32_dpmi_seginfo old_timer_handler;
_go32_dpmi_seginfo new_timer_handler;
volatile unsigned long timer_ticks = 0;

// Sound Blaster DSP commands
#define DSP_RESET       0x01
#define DSP_SPEAKER_ON  0xD1
#define DSP_SPEAKER_OFF 0xD3
#define DSP_8BIT_AUTO   0x1C
#define DSP_GET_VERSION 0xE1

// DMA controller ports
#define DMA_MASK_REG    0x0A
#define DMA_MODE_REG    0x0B
#define DMA_CLEAR_FF    0x0C

unsigned long _go32_conventional_memory_start(void) {
    return 0;  // In DJGPP, conventional memory starts at physical address 0
}

// DMA channel registers (for 8-bit channels 0-3)
unsigned short dma_address_regs[4] = {0x00, 0x02, 0x04, 0x06};
unsigned short dma_count_regs[4] = {0x01, 0x03, 0x05, 0x07};
unsigned short dma_page_regs[4] = {0x87, 0x83, 0x81, 0x82};

// Simple delay function (DJGPP doesn't have delay)
void my_delay(int ms) {
    usleep(ms * 1000);  // usleep takes microseconds
}

// Write a command to the DSP
void sb_dsp_write(unsigned char value) {
    // Wait until DSP is ready to receive a command
    while ((inportb(sb_base_addr + 0xC) & 0x80) != 0);
    outportb(sb_base_addr + 0xC, value);
}

// Read a byte from the DSP
unsigned char sb_dsp_read() {
    // Wait until DSP has data available
    while ((inportb(sb_base_addr + 0xE) & 0x80) == 0);
    return inportb(sb_base_addr + 0xA);
}

// Forward declarations
void fill_audio_buffer(void);
void processEvents(void);

// IRQ handler for Sound Blaster (DPMI protected mode handler)
void sb_irq_handler(_go32_dpmi_registers *regs) {
    // Acknowledge interrupt from Sound Blaster
    inportb(sb_base_addr + 0xE);
    
    // Toggle which half of the buffer we're filling
    dma_buffer_half = 1 - dma_buffer_half;
    
    // Set buffer position for the next half to fill
    buffer_position = dma_buffer_half * (DMA_BUFFER_SIZE / 2);
    
    // Fill the inactive half of the buffer with new audio data
    if (isPlaying && !paused) {
        fill_audio_buffer();
    }
    
    // Acknowledge interrupt to PIC
    outportb(0x20, 0x20);
}

// Timer interrupt handler (for MIDI event processing)
void timer_handler(_go32_dpmi_registers *regs) {
    // Increment our counter
    timer_ticks++;
    
    // Update MIDI timing (at approximately 100Hz)
    if (timer_ticks % 2 == 0 && isPlaying && !paused) {
        // Process MIDI events
        playTime += 0.01;  // 10ms
        
        // Process any due MIDI events
        playwait -= 0.01;
        while (playwait <= 0.0 && isPlaying) {
            processEvents();
        }
    }
}

// Detect Sound Blaster card
int detect_sound_blaster() {
    // Sound Blaster base addresses to check
    const unsigned short SB_ADDRESSES[] = {0x220, 0x240, 0x260, 0x280};
    
    // Try each possible address
    for (int i = 0; i < 4; i++) {
        // Reset the DSP
        outportb(SB_ADDRESSES[i] + 6, 1);
        my_delay(10);  // Wait 10ms
        outportb(SB_ADDRESSES[i] + 6, 0);
        my_delay(10);  // Wait 10ms
        
        // Check if reset successful (should get 0xAA back)
        if (inportb(SB_ADDRESSES[i] + 10) == 0xAA) {
            // Found a Sound Blaster!
            sb_base_addr = SB_ADDRESSES[i];
            
            // Get DSP version
            sb_dsp_write(DSP_GET_VERSION);
            sb_version_major = sb_dsp_read();
            sb_version_minor = sb_dsp_read();
            
            printf("Sound Blaster detected at 0x%x, version %d.%d\n", 
                   sb_base_addr, sb_version_major, sb_version_minor);
            return 1;
        }
    }
    
    printf("No Sound Blaster card detected!\n");
    return 0;
}

// Allocate DMA buffer (page-aligned)
int allocate_dma_buffer() {
    // First, allocate the main buffer with extra space for alignment
    dma_buffer = (unsigned char *)malloc(DMA_BUFFER_SIZE + 16);
    if (dma_buffer == NULL) {
        printf("Failed to allocate DMA buffer\n");
        return 0;
    }
    
    // Align to 16-byte boundary for DMA
    unsigned long addr = (unsigned long)dma_buffer;
    unsigned long aligned_addr = (addr + 15) & ~15;
    dma_page_aligned_buffer = (unsigned char *)aligned_addr;
    
    // Fill buffer with silence (unsigned 8-bit, 128 = silence)
    memset(dma_page_aligned_buffer, 128, DMA_BUFFER_SIZE);
    
    return 1;
}

// Program the DMA controller for Sound Blaster playback
void setup_dma(unsigned char channel, unsigned char *buffer, unsigned int length) {
    unsigned long phys_addr;
    unsigned int page, offset;
    
    // Get the physical address of the buffer
    phys_addr = _go32_conventional_memory_start() + (unsigned long)buffer;
    
    // Calculate page and offset
    page = phys_addr >> 16;
    offset = phys_addr & 0xFFFF;
    
    // Disable interrupts during DMA programming
    _disable();
    
    // Disable the DMA channel
    outportb(DMA_MASK_REG, channel | 0x04);
    
    // Clear the byte pointer flip-flop
    outportb(DMA_CLEAR_FF, 0);
    
    // Set DMA mode: single transfer, auto-init, read mode (from memory)
    outportb(DMA_MODE_REG, 0x58 | channel);
    
    // Set DMA buffer address
    outportb(dma_address_regs[channel], offset & 0xFF);
    outportb(dma_address_regs[channel], (offset >> 8) & 0xFF);
    
    // Set DMA page
    outportb(dma_page_regs[channel], page & 0xFF);
    
    // Set DMA count (length - 1)
    outportb(dma_count_regs[channel], (length - 1) & 0xFF);
    outportb(dma_count_regs[channel], ((length - 1) >> 8) & 0xFF);
    
    // Enable the DMA channel
    outportb(DMA_MASK_REG, channel & 0x03);
    
    // Re-enable interrupts
    _enable();
}

// Start Sound Blaster playback
void start_sb_playback(int sample_rate) {
    // Calculate time constant
    int time_constant = 256 - 1000000 / sample_rate;
    
    // Set the sample rate
    sb_dsp_write(0x40);
    sb_dsp_write(time_constant);
    
    // Setup DMA transfer
    setup_dma(sb_dma, dma_page_aligned_buffer, DMA_BUFFER_SIZE);
    
    // Start 8-bit auto-init DMA playback
    sb_dsp_write(DSP_8BIT_AUTO);
    sb_dsp_write((DMA_BUFFER_SIZE - 1) & 0xFF);
    sb_dsp_write(((DMA_BUFFER_SIZE - 1) >> 8) & 0xFF);
    
    // Turn on speaker
    sb_dsp_write(DSP_SPEAKER_ON);
}

// Setup IRQ handler using DJGPP DPMI
void setup_irq() {
    int vector = sb_irq + 8;
    
    // Get current interrupt handler
    _go32_dpmi_get_protected_mode_interrupt_vector(vector, &old_irq_handler);
    
    // Setup our handler
    new_irq_handler.pm_offset = (unsigned long)sb_irq_handler;
    new_irq_handler.pm_selector = _go32_my_cs();
    _go32_dpmi_allocate_iret_wrapper(&new_irq_handler);
    _go32_dpmi_set_protected_mode_interrupt_vector(vector, &new_irq_handler);
    
    // Enable IRQ in the PIC
    outportb(0x21, inportb(0x21) & ~(1 << sb_irq));
}

// Setup timer interrupt using DJGPP DPMI
void setup_timer() {
    // Get current interrupt handler
    _go32_dpmi_get_protected_mode_interrupt_vector(0x1C, &old_timer_handler);
    
    // Setup our handler
    new_timer_handler.pm_offset = (unsigned long)timer_handler;
    new_timer_handler.pm_selector = _go32_my_cs();
    _go32_dpmi_allocate_iret_wrapper(&new_timer_handler);
    _go32_dpmi_set_protected_mode_interrupt_vector(0x1C, &new_timer_handler);
}

// Initialize Sound Blaster for playback
int init_sound_blaster() {
    if (!detect_sound_blaster()) {
        return 0;
    }
    
    // Allocate DMA buffer
    if (!allocate_dma_buffer()) {
        return 0;
    }
    
    // Reset DSP
    outportb(sb_base_addr + 6, 1);
    my_delay(10);
    outportb(sb_base_addr + 6, 0);
    my_delay(10);
    
    // Wait for reset acknowledgement
    if (sb_dsp_read() != 0xAA) {
        printf("Failed to reset Sound Blaster DSP\n");
        return 0;
    }
    
    // Setup IRQ handler
    setup_irq();
    
    // Setup timer interrupt
    setup_timer();
    
    return 1;
}

// Fill buffer with DBOPL-generated audio
void fill_audio_buffer() {
    int samples_per_half = DMA_BUFFER_SIZE / 2 / 2; // Divide by 2 for stereo
    int16_t temp_buffer[512 * 2]; // Temporary buffer for DBOPL output
    
    // Fill the inactive half of the buffer
    int fill_position = (1 - dma_buffer_half) * (DMA_BUFFER_SIZE / 2);
    
    // Generate audio in chunks to fit our temp buffer
    for (int offset = 0; offset < samples_per_half; offset += 512) {
        int chunk_size = (offset + 512 <= samples_per_half) ? 512 : (samples_per_half - offset);
        
        // Generate audio with DBOPL
        OPL_Generate(temp_buffer, chunk_size);
        
        // Convert to 8-bit unsigned and copy to DMA buffer
        for (int i = 0; i < chunk_size * 2; i++) {
            // Apply global volume scaling
            int32_t sample = (int32_t)(temp_buffer[i] * (globalVolume / 100.0));
            
            // Convert from 16-bit signed to 8-bit unsigned
            sample = (sample / 256) + 128;
            
            // Clip to 0-255 range
            if (sample > 255) sample = 255;
            if (sample < 0) sample = 0;
            
            // Copy to DMA buffer
            dma_page_aligned_buffer[fill_position++] = (unsigned char)sample;
        }
    }
}

// Cleanup when shutting down
void cleanup() {
    // Stop playback
    sb_dsp_write(DSP_SPEAKER_OFF);
    
    // Restore IRQ handler
    _go32_dpmi_set_protected_mode_interrupt_vector(sb_irq + 8, &old_irq_handler);
    _go32_dpmi_free_iret_wrapper(&new_irq_handler);
    
    // Restore timer handler
    _go32_dpmi_set_protected_mode_interrupt_vector(0x1C, &old_timer_handler);
    _go32_dpmi_free_iret_wrapper(&new_timer_handler);
    
    // Free DMA buffer
    if (dma_buffer != NULL) {
        free(dma_buffer);
        dma_buffer = NULL;
        dma_page_aligned_buffer = NULL;
    }
    
    // Cleanup OPL
    OPL_Shutdown();
    
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
}

// Helper: Read variable length value from MIDI file
unsigned long readVarLen(FILE* f) {
    unsigned char c;
    unsigned long value = 0;
    
    if (fread(&c, 1, 1, f) != 1) return 0;
    
    value = c;
    if (c & 0x80) {
        value &= 0x7F;
        do {
            if (fread(&c, 1, 1, f) != 1) return value;
            value = (value << 7) + (c & 0x7F);
        } while (c & 0x80);
    }
    
    return value;
}

// Helper: Read bytes from file
int readString(FILE* f, int len, char* str) {
    return fread(str, 1, len, f);
}

// Helper: Parse big-endian integer
unsigned long convertInteger(char* str, int len) {
    unsigned long value = 0;
    for (int i = 0; i < len; i++) {
        value = value * 256 + (unsigned char)str[i];
    }
    return value;
}

// Load and parse MIDI file
bool loadMidiFile(const char* filename) {
    char buffer[256];
    char id[5] = {0};
    unsigned long headerLength;
    int format;
    
    // Open file
    midiFile = fopen(filename, "rb");
    if (!midiFile) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return false;
    }
    
    // Read MIDI header
    if (readString(midiFile, 4, id) != 4 || strncmp(id, "MThd", 4) != 0) {
        fprintf(stderr, "Error: Not a valid MIDI file\n");
        fclose(midiFile);
        midiFile = NULL;
        return false;
    }
    
    // Read header length
    readString(midiFile, 4, buffer);
    headerLength = convertInteger(buffer, 4);
    if (headerLength != 6) {
        fprintf(stderr, "Error: Invalid MIDI header length\n");
        fclose(midiFile);
        midiFile = NULL;
        return false;
    }
    
    // Read format type
    readString(midiFile, 2, buffer);
    format = (int)convertInteger(buffer, 2);
    
    // Read number of tracks
    readString(midiFile, 2, buffer);
    TrackCount = (int)convertInteger(buffer, 2);
    if (TrackCount > MAX_TRACKS) {
        fprintf(stderr, "Error: Too many tracks in MIDI file\n");
        fclose(midiFile);
        midiFile = NULL;
        return false;
    }
    
    // Read time division
    readString(midiFile, 2, buffer);
    DeltaTicks = (int)convertInteger(buffer, 2);
    
    // Initialize track data
    for (int tk = 0; tk < TrackCount; tk++) {
        // Read track header
        if (readString(midiFile, 4, id) != 4 || strncmp(id, "MTrk", 4) != 0) {
            fprintf(stderr, "Error: Invalid track header\n");
            fclose(midiFile);
            midiFile = NULL;
            return false;
        }
        
        // Read track length
        readString(midiFile, 4, buffer);
        unsigned long trackLength = convertInteger(buffer, 4);
        long pos = ftell(midiFile);
        
        // Read first event delay
        tkDelay[tk] = readVarLen(midiFile);
        tkPtr[tk] = ftell(midiFile);
        
        // Skip to next track
        fseek(midiFile, pos + (long)trackLength, SEEK_SET);
    }
    
    // Reset file position for playback
    fseek(midiFile, 0, SEEK_SET);
    
    printf("MIDI file loaded: %s\n", filename);
    printf("Format: %d, Tracks: %d, Time Division: %d\n", format, TrackCount, DeltaTicks);
    
    return true;
}

// Update volume
void updateVolume(int change) {
    globalVolume += change;
    if (globalVolume < 0) globalVolume = 0;
    if (globalVolume > 200) globalVolume = 200;
    
    printf("Volume: %d%%\n", globalVolume);
}

// Toggle volume normalization
void toggleNormalization() {
    enableNormalization = !enableNormalization;
    printf("Normalization: %s\n", enableNormalization ? "ON" : "OFF");
}

// Handle a single MIDI event
void handleMidiEvent(int tk) {
    unsigned char status, data1, data2;
    unsigned char evtype;
    unsigned long len;
    
    // Get file position
    fseek(midiFile, tkPtr[tk], SEEK_SET);
    
    // Read status byte or use running status
    if (fread(&status, 1, 1, midiFile) != 1) return;
    
    // Check for running status
    if (status < 0x80) {
        fseek(midiFile, tkPtr[tk], SEEK_SET); // Go back one byte
        status = tkStatus[tk]; // Use running status
    } else {
        tkStatus[tk] = status;
    }
    
    int midCh = status & 0x0F;
    
    // Handle different event types
    switch (status & 0xF0) {
        case NOTE_OFF: {
            // Note Off event
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            ChBend[midCh] = 0;
            OPL_NoteOff(midCh, data1);
            break;
        }
        
        case NOTE_ON: {
            // Note On event
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            // Note on with velocity 0 is treated as note off
            if (data2 == 0) {
                ChBend[midCh] = 0;
                OPL_NoteOff(midCh, data1);
                break;
            }
            
            OPL_NoteOn(midCh, data1, data2);
            break;
        }
        
        case CONTROL_CHANGE: {
            // Control Change
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            switch (data1) {
                case 1:  // Modulation Wheel
                    ChVibrato[midCh] = data2;
                    break;
                    
                case 7:  // Channel Volume
                    ChVolume[midCh] = data2;
                    OPL_SetVolume(midCh, data2);
                    break;
                    
                case 10: // Pan
                    ChPanning[midCh] = data2;
                    OPL_SetPan(midCh, data2);
                    break;
                    
                case 11: // Expression
                    // Expression is like a secondary volume control
                    for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                        if (opl_channels[i].active && opl_channels[i].midi_channel == midCh) {
                            set_channel_volume(i, opl_channels[i].velocity, 
                                             (ChVolume[midCh] * data2) / 127);
                        }
                    }
                    break;
                    
                case 120: // All Sound Off
                    // Immediately silence all sound (emergency)
                    OPL_Reset();
                    break;
                    
                case 121: // Reset All Controllers
                    // Reset controllers to default
                    for (int i = 0; i < 16; i++) {
                        ChBend[i] = 0;
                        ChVibrato[i] = 0;
                        // Don't reset volume and panning
                    }
                    break;
                    
                case 123: // All Notes Off
                    // Turn off all notes on this channel
                    for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                        if (opl_channels[i].active && opl_channels[i].midi_channel == midCh) {
                            OPL_NoteOff(midCh, opl_channels[i].midi_note);
                        }
                    }
                    break;
            }
            break;
        }
        
        case PROGRAM_CHANGE: {
            // Program Change
            fread(&data1, 1, 1, midiFile);
            ChPatch[midCh] = data1;
            OPL_ProgramChange(midCh, data1);
            break;
        }
        
        case CHAN_PRESSURE: {
            // Channel Aftertouch
            fread(&data1, 1, 1, midiFile);
            // Could apply pressure to all active notes on this channel
            for (int i = 0; i < MAX_OPL_CHANNELS; i++) {
                if (opl_channels[i].active && opl_channels[i].midi_channel == midCh) {
                    // Apply aftertouch as a volume scaling
                    set_channel_volume(i, opl_channels[i].velocity, 
                                     (ChVolume[midCh] * data1) / 127);
                }
            }
            break;
        }
        
        case PITCH_BEND: {
            // Pitch Bend
            fread(&data1, 1, 1, midiFile);
            fread(&data2, 1, 1, midiFile);
            
            // Combine LSB and MSB into a 14-bit value
            int bend = (data2 << 7) | data1;
            ChBend[midCh] = bend;
            
            OPL_SetPitchBend(midCh, bend);
            break;
        }
        
        case META_EVENT: case SYSTEM_MESSAGE: {
            // Meta events and system exclusive
            if (status == META_EVENT) {
                // Meta event
                fread(&evtype, 1, 1, midiFile);
                len = readVarLen(midiFile);
                
                if (evtype == META_END_OF_TRACK) {
                    tkStatus[tk] = -1;  // Mark track as ended
                    fseek(midiFile, len, SEEK_CUR);  // Skip event data
                } else if (evtype == META_TEMPO) {
                    // Tempo change
                    char tempo[4] = {0};
                    readString(midiFile, (int)len, tempo);
                    unsigned long tempoVal = convertInteger(tempo, (int)len);
                    Tempo = tempoVal;
                } else if (evtype == META_TEXT) {
                    // Text event - check for loop markers or custom instructions
                    char text[256] = {0};
                    readString(midiFile, (int)len < 255 ? (int)len : 255, text);
                    
                    if (strcmp(text, "loopStart") == 0) {
                        loopStart = true;
                    } else if (strcmp(text, "loopEnd") == 0) {
                        loopEnd = true;
                    } else if (strstr(text, "volume=") == text) {
                        // Custom volume instruction, format: "volume=XX"
                        int volume = atoi(text + 7);
                        if (volume >= 0 && volume <= 127) {
                            ChVolume[midCh] = volume;
                            OPL_SetVolume(midCh, volume);
                        }
                    } else if (strstr(text, "instrument=") == text) {
                        // Custom instrument instruction, format: "instrument=XX"
                        int instrument = atoi(text + 11);
                        if (instrument >= 0 && instrument < 181) {
                            ChPatch[midCh] = instrument;
                            OPL_ProgramChange(midCh, instrument);
                        }
                    }
                } else {
                    // Skip other meta events
                    fseek(midiFile, len, SEEK_CUR);
                }
            } else {
                // System exclusive - skip
                len = readVarLen(midiFile);
                fseek(midiFile, (long)len, SEEK_CUR);
            }
            break;
        }
        
        default: {
            // Unknown or unsupported message type
            // Try to skip based on expected message length
            switch (status & 0xF0) {
                case 0xC0: // Program Change
                case 0xD0: // Channel Pressure
                    fseek(midiFile, 1, SEEK_CUR); // One data byte
                    break;
                default:
                    fseek(midiFile, 2, SEEK_CUR); // Assume two data bytes
                    break;
            }
            break;
        }
    }
    
    // Read next event delay
    unsigned long nextDelay = readVarLen(midiFile);
    tkDelay[tk] += nextDelay;
    
    // Save new file position
    tkPtr[tk] = ftell(midiFile);
}

// Process events for all tracks
void processEvents() {
    // Save rollback info for each track
    for (int tk = 0; tk < TrackCount; tk++) {
        rbPtr[tk] = tkPtr[tk];
        rbDelay[tk] = tkDelay[tk];
        rbStatus[tk] = tkStatus[tk];
        
        // Handle events for tracks that are due
        if (tkStatus[tk] >= 0 && tkDelay[tk] <= 0) {
            handleMidiEvent(tk);
        }
    }
    
    // Handle loop points
    if (loopStart) {
        // Save loop beginning point
        for (int tk = 0; tk < TrackCount; tk++) {
            loPtr[tk] = rbPtr[tk];
            loDelay[tk] = rbDelay[tk];
            loStatus[tk] = rbStatus[tk];
        }
        loopwait = playwait;
        loopStart = false;
    } else if (loopEnd) {
        // Return to loop beginning
        for (int tk = 0; tk < TrackCount; tk++) {
            tkPtr[tk] = loPtr[tk];
            tkDelay[tk] = loDelay[tk];
            tkStatus[tk] = loStatus[tk];
        }
        loopEnd = false;
        playwait = loopwait;
    }
    
    // Find the shortest delay from all tracks
    double nextDelay = -1;
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] < 0) continue;
        if (nextDelay == -1 || tkDelay[tk] < nextDelay) {
            nextDelay = tkDelay[tk];
        }
    }
    
    // Check if all tracks are ended
    bool allEnded = true;
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] >= 0) {
            allEnded = false;
            break;
        }
    }
    
    if (allEnded) {
        // Either loop or stop playback
        if (loopwait > 0) {
            // Return to loop beginning
            for (int tk = 0; tk < TrackCount; tk++) {
                tkPtr[tk] = loPtr[tk];
                tkDelay[tk] = loDelay[tk];
                tkStatus[tk] = loStatus[tk];
            }
            playwait = loopwait;
        } else {
            // Stop playback
            isPlaying = false;
            return;
        }
    }
    
    // Update all track delays
    for (int tk = 0; tk < TrackCount; tk++) {
        tkDelay[tk] -= nextDelay;
    }
    
    // Schedule next event
    double t = nextDelay * Tempo / (DeltaTicks * 1000000.0);
    playwait += t;
}

// Initialize DOS environment
int init_dos() {
    // Initialize Sound Blaster
    if (!init_sound_blaster()) {
        return 0;
    }
    
    // Initialize OPL
    OPL_Init(SAMPLE_RATE);
    
    // Initialize FM instruments
    OPL_LoadInstruments();
    
    return 1;
}

// Play MIDI file
void playMidiFile() {
    // Initialize variables for all channels
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 64;
        ChVibrato[i] = 0;
    }
    
    // Reset playback state
    playTime = 0;
    isPlaying = true;
    paused = false;
    loopStart = false;
    loopEnd = false;
    playwait = 0;
    loopwait = 0;
    
    // Reset all OPL channels
    OPL_Reset();
    
    // Start playback
    start_sb_playback(SAMPLE_RATE);
    
    printf("Playback started. Controls:\n");
    printf("  ESC - Quit\n");
    printf("  Space - Pause/Resume\n");
    printf("  +/- - Increase/Decrease Volume\n");
    printf("  n - Toggle Normalization\n");
    
    // Main playback loop
    while (isPlaying) {
        // Check for key press
        if (kbhit()) {
            int key = getch();
            switch (key) {
                case 27:  // ESC
                    isPlaying = false;
                    break;
                case ' ':
                    paused = !paused;
                    printf("%s\n", paused ? "Paused" : "Resumed");
                    break;
                case '+':
                    updateVolume(10);
                    break;
                case '-':
                    updateVolume(-10);
                    break;
                case 'n':
                    toggleNormalization();
                    break;
            }
        }
        
        // Small delay to prevent hogging CPU
        my_delay(10);
    }
    
    // Stop playback
    sb_dsp_write(DSP_SPEAKER_OFF);
    
    printf("Playback ended.\n");
}

int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME] = {0}; // Empty filename by default
    
    printf("MS-DOS MIDI Player with Sound Blaster\n");
    printf("-----------------------------------\n");
    
    // Check for command line parameter
    if (argc > 1) {
        strncpy(filename, argv[1], MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        printf("Using MIDI file from command line: %s\n", filename);
    } else {
        printf("First parameter should be a midi file.\n");
        return 1;
    }
    
    // Initialize DOS environment
    if (!init_dos()) {
        fprintf(stderr, "Failed to initialize DOS environment\n");
        return 1;
    }
    
    // Load MIDI file
    printf("Loading %s...\n", filename);
    if (!loadMidiFile(filename)) {
        fprintf(stderr, "Failed to load MIDI file\n");
        cleanup();
        return 1;
    }
    
    // Play the MIDI file
    playMidiFile();
    
    // Clean up
    cleanup();
    
    return 0;
}
