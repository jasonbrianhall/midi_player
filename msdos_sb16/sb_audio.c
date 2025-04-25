#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <time.h>
#include "sb_audio.h"

// Global variables
static int sb_base_address = SB_BASE_ADDRESS;
static int sb_irq = SB_IRQ;
static int sb_dma = SB_DMA;
static int sb_sample_rate = 44100;
static bool sb_stereo = true;
static bool sb_16bit = true;
static bool sb_initialized = false;
static bool sb_playing = false;
static int sb_dsp_version = 0;

// DMA buffer management
static uint8_t *sb_dma_buffer[SB_DMA_BUFFER_COUNT];
static int sb_current_buffer = 0;
static int sb_buffer_size = SB_BUFFER_SIZE;

// Audio callback function
static void (*sb_callback)(void*, uint8_t*, int) = NULL;
static void *sb_userdata = NULL;

// Interrupt handler variables
static void (*old_interrupt_handler)() = NULL;
static volatile bool interrupt_received = false;

// DOS-specific port I/O functions
void outportb(unsigned short port, unsigned char value) {
    outp(port, value);
}

unsigned char inportb(unsigned short port) {
    return inp(port);
}

// Custom delay implementation
void sb_delay(int milliseconds) {
    clock_t start_time = clock();
    clock_t end_time = start_time + (milliseconds * CLOCKS_PER_SEC / 1000);
    while (clock() < end_time) {
        // Wait
    }
}

// IRQ handler for SoundBlaster (non-interrupt version for standard compilers)
void SB_IRQHandler(void) {
    // Acknowledge interrupt
    inportb(sb_base_address + 0xE);
    
    // Signal that interrupt was received
    interrupt_received = true;
    
    // Send EOI to PIC
    outportb(0x20, 0x20);
    if (sb_irq >= 8) {
        outportb(0xA0, 0x20);
    }
}

// SoundBlaster DSP reset
static bool SB_ResetDSP(void) {
    // Reset DSP by writing 1 to the reset port
    outportb(sb_base_address + 6, 1);
    sb_delay(10);  // Wait 10ms
    outportb(sb_base_address + 6, 0);
    sb_delay(10);  // Wait 10ms
    
    // Wait for DSP to return 0xAA
    int timeout = 100;  // 100ms timeout
    while (timeout > 0) {
        if ((inportb(sb_base_address + 0xE) & 0x80) != 0) {
            if (inportb(sb_base_address + 0xA) == 0xAA) {
                return true;
            }
        }
        sb_delay(1);
        timeout--;
    }
    
    return false;
}

// Write to DSP
static void SB_WriteDSP(uint8_t value) {
    // Wait until DSP is ready for data
    while ((inportb(sb_base_address + 0xC) & 0x80) != 0) {
        // Wait
    }
    outportb(sb_base_address + 0xC, value);
}

// Read from DSP
static uint8_t SB_ReadDSP(void) {
    // Wait until DSP has data available
    while ((inportb(sb_base_address + 0xE) & 0x80) == 0) {
        // Wait
    }
    return inportb(sb_base_address + 0xA);
}

// Program DMA controller for SoundBlaster
static bool SB_ProgramDMA(uint8_t *buffer, int length) {
    uint16_t dma_page_reg;
    uint16_t dma_addr_reg;
    uint16_t dma_count_reg;
    uint16_t dma_mode_reg;
    uint8_t dma_mode;
    
    if (sb_dma < 4) {
        // 8-bit DMA
        dma_page_reg = 0x87 + sb_dma;
        dma_addr_reg = 0x00 + (sb_dma * 2);
        dma_count_reg = 0x01 + (sb_dma * 2);
        dma_mode_reg = 0x0B;
        dma_mode = 0x48 + sb_dma;  // Single transfer, increment, auto-init, write, channel
    } else {
        // 16-bit DMA
        dma_page_reg = 0x89 + (sb_dma - 4);
        dma_addr_reg = 0xC0 + ((sb_dma - 4) * 4);
        dma_count_reg = 0xC2 + ((sb_dma - 4) * 4);
        dma_mode_reg = 0xD6;
        dma_mode = 0x48 + (sb_dma - 4);  // Single transfer, increment, auto-init, write, channel
    }
    
    // Calculate physical address and page
    uint32_t addr = (uint32_t)buffer;
    uint16_t dma_addr;
    uint8_t dma_page;
    
    if (sb_dma < 4) {
        // 8-bit DMA
        dma_addr = addr & 0xFFFF;
        dma_page = (addr >> 16) & 0xFF;
    } else {
        // 16-bit DMA
        dma_addr = (addr >> 1) & 0xFFFF;
        dma_page = (addr >> 16) & 0xFF;
    }
    
    // Adjust count for DMA controller (count is in bytes - 1)
    uint16_t dma_count = (length - 1) & 0xFFFF;
    
    // Disable DMA channel
    outportb(0x0A, 0x04 + (sb_dma & 0x03));
    if (sb_dma >= 4) {
        outportb(0xD4, 0x04 + ((sb_dma - 4) & 0x03));
    }
    
    // Clear flip-flop
    outportb(0x0C, 0);
    if (sb_dma >= 4) {
        outportb(0xD8, 0);
    }
    
    // Set DMA mode
    outportb(dma_mode_reg, dma_mode);
    
    // Set DMA address
    outportb(dma_addr_reg, dma_addr & 0xFF);
    outportb(dma_addr_reg, (dma_addr >> 8) & 0xFF);
    
    // Set DMA count
    outportb(dma_count_reg, dma_count & 0xFF);
    outportb(dma_count_reg, (dma_count >> 8) & 0xFF);
    
    // Set DMA page
    outportb(dma_page_reg, dma_page);
    
    // Enable DMA channel
    outportb(0x0A, sb_dma & 0x03);
    if (sb_dma >= 4) {
        outportb(0xD4, (sb_dma - 4) & 0x03);
    }
    
    return true;
}

// Function to install IRQ handler (simplified for non-DJGPP)
static void install_handler(int irq) {
    // For now, just set the flag to indicate interrupt simulation
    // In a real DOS environment, this would use _dos_setvect or similar
    printf("Note: Interrupt handlers skipped in non-DOS environment\n");
    printf("Sound may not work correctly - falling back to polling mode\n");
    
    // We'll simulate interrupts through polling (non-optimal but should work)
    old_interrupt_handler = NULL;
    
    // Unmask IRQ in PIC (this should still work in most cases)
    uint8_t imr = inportb(irq >= 8 ? 0xA1 : 0x21);
    outportb(irq >= 8 ? 0xA1 : 0x21, imr & ~(1 << (irq & 7)));
}

// Function to remove IRQ handler (simplified for non-DJGPP)
static void remove_handler(int irq) {
    // For now, just cleanup
    // In a real DOS environment, this would use _dos_setvect or similar
    
    // Mask IRQ in PIC (this should still work in most cases)
    uint8_t imr = inportb(irq >= 8 ? 0xA1 : 0x21);
    outportb(irq >= 8 ? 0xA1 : 0x21, imr | (1 << (irq & 7)));
    
    old_interrupt_handler = NULL;
}

// Function to initialize the SoundBlaster card
bool SB_Init(int base_address, int irq, int dma, int sample_rate, bool stereo, bool use_16bit) {
    if (sb_initialized) {
        return true; // Already initialized
    }
    
    // Store parameters
    sb_base_address = base_address;
    sb_irq = irq;
    sb_dma = dma;
    sb_sample_rate = sample_rate;
    sb_stereo = stereo;
    sb_16bit = use_16bit;
    
    printf("Initializing SoundBlaster at address 0x%x, IRQ %d, DMA %d\n", 
           sb_base_address, sb_irq, sb_dma);
    
    // Reset DSP
    if (!SB_ResetDSP()) {
        fprintf(stderr, "Error: DSP reset failed\n");
        return false;
    }
    
    // Get DSP version
    SB_WriteDSP(SB_DSP_VERSION);
    uint8_t major = SB_ReadDSP();
    uint8_t minor = SB_ReadDSP();
    sb_dsp_version = (major << 8) | minor;
    printf("SoundBlaster DSP version %d.%d detected\n", major, minor);
    
    // Set up interrupt handler (simplified for non-DJGPP)
    install_handler(sb_irq);
    
    // Turn on speaker
    SB_WriteDSP(SB_SPEAKER_ON);
    
    // Calculate buffer size based on settings
    sb_buffer_size = SB_BUFFER_SIZE;
    if (sb_stereo) sb_buffer_size *= 2;
    if (sb_16bit) sb_buffer_size *= 2;
    
    // Allocate DMA buffers
    for (int i = 0; i < SB_DMA_BUFFER_COUNT; i++) {
        // Allocate aligned memory for DMA
        sb_dma_buffer[i] = (uint8_t *)malloc(sb_buffer_size + 16);
        if (sb_dma_buffer[i] == NULL) {
            fprintf(stderr, "Error allocating DMA buffer %d\n", i);
            // Free already allocated buffers
            for (int j = 0; j < i; j++) {
                free(sb_dma_buffer[j]);
                sb_dma_buffer[j] = NULL;
            }
            return false;
        }
        
        // Align buffer to 16-byte boundary
        uint32_t addr = (uint32_t)sb_dma_buffer[i];
        addr = (addr + 15) & ~15;
        sb_dma_buffer[i] = (uint8_t *)addr;
        
        // Clear buffer
        memset(sb_dma_buffer[i], 0, sb_buffer_size);
    }
    
    sb_initialized = true;
    printf("SoundBlaster initialized successfully\n");
    
    return true;
}

// Function to start audio playback
bool SB_StartPlayback(void) {
    if (!sb_initialized) {
        fprintf(stderr, "SoundBlaster not initialized\n");
        return false;
    }
    
    if (sb_playing) {
        return true; // Already playing
    }
    
    // Fill initial buffer
    if (sb_callback) {
        sb_callback(sb_userdata, sb_dma_buffer[0], sb_buffer_size);
    } else {
        // No callback, fill with silence
        memset(sb_dma_buffer[0], 0, sb_buffer_size);
    }
    
    // Program DMA controller
    SB_ProgramDMA(sb_dma_buffer[0], sb_buffer_size);
    
    // Set the sample rate
    if (sb_16bit) {
        SB_WriteDSP(SB_SET_SAMPLE_RATE);
        SB_WriteDSP((sb_sample_rate >> 8) & 0xFF);
        SB_WriteDSP(sb_sample_rate & 0xFF);
    }
    
    // Start playback
    if (sb_16bit) {
        // 16-bit output
        SB_WriteDSP(SB_OUTPUT_16BIT);
        SB_WriteDSP(sb_stereo ? 0x20 : 0x00);  // Stereo/mono flag
        SB_WriteDSP((sb_buffer_size - 1) & 0xFF);  // Low byte of length - 1
        SB_WriteDSP(((sb_buffer_size - 1) >> 8) & 0xFF);  // High byte of length - 1
    } else {
        // 8-bit output
        SB_WriteDSP(SB_OUTPUT_8BIT);
        SB_WriteDSP(sb_stereo ? 0x20 : 0x00);  // Stereo/mono flag
        SB_WriteDSP((sb_buffer_size - 1) & 0xFF);  // Low byte of length - 1
        SB_WriteDSP(((sb_buffer_size - 1) >> 8) & 0xFF);  // High byte of length - 1
    }
    
    sb_playing = true;
    sb_current_buffer = 0;
    interrupt_received = false;
    
    printf("SoundBlaster playback started\n");
    
    return true;
}

// Function to handle buffer updates during playback
void SB_UpdateBuffer(void) {
    if (!sb_initialized || !sb_playing) {
        return;
    }
    
    // In non-DOS environment, we need to poll the sound card
    // to simulate interrupts
    static int counter = 0;
    counter++;
    
    // Every few calls, simulate an interrupt
    if (counter >= 10) {
        counter = 0;
        interrupt_received = true;
    }
    
    // Check if interrupt was received (buffer completed)
    if (interrupt_received) {
        interrupt_received = false;
        
        // Switch to next buffer
        sb_current_buffer = (sb_current_buffer + 1) % SB_DMA_BUFFER_COUNT;
        
        // Fill the buffer using the callback function
        if (sb_callback) {
            sb_callback(sb_userdata, sb_dma_buffer[sb_current_buffer], sb_buffer_size);
        } else {
            // No callback, fill with silence
            memset(sb_dma_buffer[sb_current_buffer], 0, sb_buffer_size);
        }
    }
}

// Function to stop audio playback
bool SB_StopPlayback(void) {
    if (!sb_initialized || !sb_playing) {
        return true; // Not playing
    }
    
    // Stop playback
    SB_WriteDSP(SB_STOP_DMA);
    
    sb_playing = false;
    printf("SoundBlaster playback stopped\n");
    
    return true;
}

// Function to shut down SoundBlaster
void SB_Shutdown(void) {
    if (!sb_initialized) {
        return;
    }
    
    // Stop playback if still active
    if (sb_playing) {
        SB_StopPlayback();
    }
    
    // Turn off speaker
    SB_WriteDSP(SB_SPEAKER_OFF);
    
    // Remove interrupt handler (simplified for non-DJGPP)
    remove_handler(sb_irq);
    
    // Free DMA buffers
    for (int i = 0; i < SB_DMA_BUFFER_COUNT; i++) {
        if (sb_dma_buffer[i]) {
            free(sb_dma_buffer[i]);
            sb_dma_buffer[i] = NULL;
        }
    }
    
    sb_initialized = false;
    printf("SoundBlaster shutdown complete\n");
}

// Function to reset the DSP
bool SB_Reset(void) {
    if (!sb_initialized) {
        return false;
    }
    
    return SB_ResetDSP();
}

// Function to set sample rate
bool SB_SetSampleRate(int rate) {
    if (!sb_initialized) {
        return false;
    }
    
    // Update sample rate
    sb_sample_rate = rate;
    
    // Apply to DSP
    if (sb_16bit) {
        SB_WriteDSP(SB_SET_SAMPLE_RATE);
        SB_WriteDSP((sb_sample_rate >> 8) & 0xFF);
        SB_WriteDSP(sb_sample_rate & 0xFF);
    }
    
    return true;
}

// Function to set callback function
void SB_SetCallbackFunction(void (*callback)(void*, uint8_t*, int), void* userdata) {
    sb_callback = callback;
    sb_userdata = userdata;
}

// Function to fill buffer manually (used by external code)
void SB_FillBuffer(int16_t *buffer, int num_samples) {
    if (!sb_initialized || !sb_playing) {
        return;
    }
    
    // Calculate size in bytes
    int size = num_samples * (sb_16bit ? 2 : 1) * (sb_stereo ? 2 : 1);
    if (size > sb_buffer_size) {
        size = sb_buffer_size;
    }
    
    // Copy to current DMA buffer
    memcpy(sb_dma_buffer[sb_current_buffer], buffer, size);
}

// Function to check if audio is playing
bool SB_IsPlaying(void) {
    return sb_playing;
}

// Function to wait for IRQ
void SB_WaitForIRQ(void) {
    // In our polling model, we'll just delay a bit
    sb_delay(10);
    interrupt_received = true;
}

// Function to get DSP version
int SB_GetDSPVersion(void) {
    return sb_dsp_version;
}
