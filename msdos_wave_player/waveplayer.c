/**
 * Sound Blaster WAV Player with Double-Buffering
 * Plays WAV files using Sound Blaster DMA
 * Fixed stereo playback and buffer transition issues
 * Added support for BLASTER environment variable
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <pc.h>
#include <ctype.h>

// Default Sound Blaster settings (a220 i7 d1 t5)
#define DEFAULT_SB_BASE 0x220
#define DEFAULT_SB_IRQ 7
#define DEFAULT_SB_DMA 1
#define DEFAULT_SB_DMA_16BIT 5
#define DEFAULT_SB_TYPE 5  // SB16

// Sound Blaster settings (will be initialized from BLASTER or defaults)
unsigned int SB_BASE;
unsigned int SB_IRQ;
unsigned int SB_DMA_CHANNEL;
unsigned int SB_DMA_16BIT_CHANNEL;
unsigned int SB_TYPE;

// Sound Blaster register offsets
#define SB_RESET_OFFSET 0x6
#define SB_READ_OFFSET 0xA
#define SB_WRITE_OFFSET 0xC
#define SB_STATUS_OFFSET 0xE
#define SB_MIXER_ADDR_OFFSET 0x4
#define SB_MIXER_DATA_OFFSET 0x5

// Sound Blaster registers (computed from base address)
unsigned int SB_RESET;
unsigned int SB_READ;
unsigned int SB_WRITE;
unsigned int SB_STATUS;
unsigned int SB_MIXER_ADDR;
unsigned int SB_MIXER_DATA;

int dma_buffer_size = 4096;  // Size per buffer - will be adjusted based on format

// Exact WAV file header structure from wav_converter.h
typedef struct {
    // RIFF chunk descriptor
    char riff_header[4];        // "RIFF"
    unsigned long wav_size;     // File size - 8
    char wave_header[4];        // "WAVE"
    
    // fmt subchunk
    char fmt_header[4];         // "fmt "
    unsigned long fmt_chunk_size;  // 16 for PCM
    unsigned short audio_format;    // 1 for PCM
    unsigned short num_channels;    // 1 or 2
    unsigned long sample_rate;      // e.g., 44100
    unsigned long byte_rate;        // sample_rate * num_channels * bytes_per_sample
    unsigned short block_align;     // num_channels * bytes_per_sample
    unsigned short bits_per_sample; // 16 for 16-bit audio
    
    // data subchunk
    char data_header[4];        // "data"
    unsigned long data_size;    // Number of bytes in data
} WAV_HEADER;

// Sound Blaster DSP version
typedef struct {
    unsigned char major;
    unsigned char minor;
} DSP_VERSION;

// Double buffer structure
typedef struct {
    _go32_dpmi_seginfo segment;
    void *address;
    int size;
    int active;  // 0 or 1
} DMA_BUFFER;

// Global variables
DMA_BUFFER dma_buffers[2];
FILE *wav_file;
unsigned long data_size = 0;
unsigned long bytes_read = 0;
volatile int playing = 0;
unsigned short sample_rate = 22050;
unsigned short channels = 1;
unsigned short bits_per_sample = 8;
DSP_VERSION dsp_version = {0, 0};
int using_16bit = 0;
volatile int current_buffer = 0;
_go32_dpmi_seginfo old_irq_handler;
_go32_dpmi_seginfo new_irq_handler;

// IRQ handler variables
volatile int buffer_finished = 0;

// Parse BLASTER environment variable
void parse_blaster_env(void) {
    char *blaster = getenv("BLASTER");
    
    // Use defaults if BLASTER is not defined
    if (blaster == NULL) {
        printf("BLASTER environment variable not found, using defaults:\n");
        printf("A220 I7 D1 T5\n");
        
        SB_BASE = DEFAULT_SB_BASE;
        SB_IRQ = DEFAULT_SB_IRQ;
        SB_DMA_CHANNEL = DEFAULT_SB_DMA;
        SB_DMA_16BIT_CHANNEL = DEFAULT_SB_DMA_16BIT;
        SB_TYPE = DEFAULT_SB_TYPE;
    } else {
        printf("Found BLASTER=%s\n", blaster);
        
        // Initialize with defaults (in case some parameters are missing)
        SB_BASE = DEFAULT_SB_BASE;
        SB_IRQ = DEFAULT_SB_IRQ;
        SB_DMA_CHANNEL = DEFAULT_SB_DMA;
        SB_DMA_16BIT_CHANNEL = DEFAULT_SB_DMA_16BIT;
        SB_TYPE = DEFAULT_SB_TYPE;
        
        // Make a copy we can modify
        char env_copy[256];
        strncpy(env_copy, blaster, 255);
        env_copy[255] = '\0';
        
        // Parse the environment string
        char *token = strtok(env_copy, " ");
        while (token != NULL) {
            // Handle each parameter type
            switch (toupper(token[0])) {
                case 'A':  // Address
                    sscanf(token + 1, "%x", &SB_BASE);
                    break;
                case 'I':  // IRQ
                    sscanf(token + 1, "%d", &SB_IRQ);
                    break;
                case 'D':  // 8-bit DMA
                    sscanf(token + 1, "%d", &SB_DMA_CHANNEL);
                    break;
                case 'H':  // 16-bit DMA
                    sscanf(token + 1, "%d", &SB_DMA_16BIT_CHANNEL);
                    break;
                case 'T':  // Type
                    sscanf(token + 1, "%d", &SB_TYPE);
                    break;
            }
            
            token = strtok(NULL, " ");
        }
    }
    
    // Compute register addresses from base address
    SB_RESET = SB_BASE + SB_RESET_OFFSET;
    SB_READ = SB_BASE + SB_READ_OFFSET;
    SB_WRITE = SB_BASE + SB_WRITE_OFFSET;
    SB_STATUS = SB_BASE + SB_STATUS_OFFSET;
    SB_MIXER_ADDR = SB_BASE + SB_MIXER_ADDR_OFFSET;
    SB_MIXER_DATA = SB_BASE + SB_MIXER_DATA_OFFSET;
    
    // Use high DMA channel from BLASTER env if set, otherwise use default
    if (SB_DMA_16BIT_CHANNEL == 0) {
        SB_DMA_16BIT_CHANNEL = DEFAULT_SB_DMA_16BIT;
    }
    
    // Print the settings we're using
    printf("Using Sound Blaster settings:\n");
    printf("  Base I/O: 0x%03X\n", SB_BASE);
    printf("  IRQ: %d\n", SB_IRQ);
    printf("  8-bit DMA: %d\n", SB_DMA_CHANNEL);
    printf("  16-bit DMA: %d\n", SB_DMA_16BIT_CHANNEL);
    printf("  Type: %d\n", SB_TYPE);
}

int calculate_optimal_buffer_size(unsigned short channels, unsigned short bits_per_sample, 
                                 unsigned long sample_rate) {
    // Calculate bytes per second
    unsigned long bytes_per_sec = sample_rate * channels * (bits_per_sample / 8);
    
    // Base buffer sizes for different bit rates
    int buffer_size;
    
    if (bytes_per_sec <= 22050) {
        // Low quality audio (e.g., 11025 Hz, 8-bit, mono)
        buffer_size = 4096;  // 8KB is enough
    } else if (bytes_per_sec <= 88200) {
        // Medium quality (e.g., 22050 Hz, 16-bit, stereo)
        buffer_size = 8192;  // 16KB
    } else if (bytes_per_sec <= 176400) {
        // CD quality (44100 Hz, 16-bit, stereo)
        buffer_size = 16384;  // 32KB
    } else {
        // High quality (48000+ Hz, 16-bit, stereo)
        buffer_size = 32768;  // 64KB (maximum safe size)
    }
    
    // Ensure buffer size doesn't exceed maximum safe DMA boundary
    if (buffer_size > 65536) {
        buffer_size = 65536;
    }
    
    // For 16-bit samples, ensure buffer size is even
    if (bits_per_sample == 16) {
        buffer_size &= ~1;
    }
    
    // For stereo, ensure buffer size is multiple of 2*bytes_per_sample
    if (channels == 2) {
        int bytes_per_frame = channels * (bits_per_sample / 8);
        buffer_size = (buffer_size / bytes_per_frame) * bytes_per_frame;
    }
    
    return buffer_size;
}

// Reset the Sound Blaster DSP
int reset_dsp(void) {
    outportb(SB_RESET, 1);
    delay(10);
    outportb(SB_RESET, 0);
    delay(10);
    
    int timeout = 100;
    while (timeout-- && !(inportb(SB_STATUS) & 0x80))
        delay(1);
    
    if (timeout <= 0) {
        printf("DSP reset timeout\n");
        return 0;
    }
    
    int val = inportb(SB_READ);
    if (val != 0xAA) {
        printf("DSP reset failed: got 0x%02X instead of 0xAA\n", val);
        return 0;
    }
    
    return 1;
}

// Write a command to the DSP
void write_dsp(unsigned char value) {
    int timeout = 1000;
    while (timeout-- && (inportb(SB_STATUS) & 0x80))
        delay(1);
    
    if (timeout <= 0) {
        printf("DSP write timeout\n");
        return;
    }
    
    outportb(SB_WRITE, value);
}

// Get DSP version
int get_dsp_version(DSP_VERSION *version) {
    write_dsp(0xE1);
    
    int timeout = 100;
    while (timeout-- && !(inportb(SB_STATUS) & 0x80))
        delay(1);
    
    if (timeout <= 0)
        return 0;
    
    version->major = inportb(SB_READ);
    
    timeout = 100;
    while (timeout-- && !(inportb(SB_STATUS) & 0x80))
        delay(1);
    
    if (timeout <= 0)
        return 0;
    
    version->minor = inportb(SB_READ);
    
    return 1;
}

// Set the DSP sample rate for stereo playback
void set_sample_rate(unsigned short rate) {
    // No need to divide by 2 for stereo - the SB16 handles stereo properly
    if (using_16bit) {
        // Set 16-bit sample rate
        write_dsp(0x41);  // Set output sample rate
        write_dsp(rate >> 8);
        write_dsp(rate & 0xFF);
    } else {
        // Set 8-bit sample rate
        write_dsp(0x40);
        unsigned char time_constant = 256 - (1000000 / rate);
        write_dsp(time_constant);
    }
    
    // Introduce a small delay after setting sample rate
    delay(10);
}

// Allocate DMA buffers for double-buffering
int allocate_dma_buffers(void) {
    int i;
    for (i = 0; i < 2; i++) {
        // Allocate DOS memory block based on dynamic buffer size
        dma_buffers[i].segment.size = (dma_buffer_size + 15) / 16;  // Size in paragraphs
        
        if (_go32_dpmi_allocate_dos_memory(&dma_buffers[i].segment) != 0) {
            // Fail - free any allocated buffers
            while (--i >= 0) {
                _go32_dpmi_free_dos_memory(&dma_buffers[i].segment);
            }
            printf("Failed to allocate DMA buffer %d\n", i);
            return 0;
        }
        
        // Calculate the linear address
        dma_buffers[i].address = (void *)((dma_buffers[i].segment.rm_segment << 4) + __djgpp_conventional_base);
        dma_buffers[i].size = dma_buffer_size;
        dma_buffers[i].active = 0;
    }
    
    return 1;
}

// Free DMA buffers
void free_dma_buffers(void) {
    for (int i = 0; i < 2; i++) {
        if (dma_buffers[i].segment.size > 0) {
            _go32_dpmi_free_dos_memory(&dma_buffers[i].segment);
            dma_buffers[i].segment.size = 0;
        }
    }
}

// Program the DMA controller for the current buffer
void program_dma(int buffer_index) {
    unsigned long phys_addr = dma_buffers[buffer_index].segment.rm_segment << 4;
    unsigned char page;
    unsigned short offset;
    int count = dma_buffers[buffer_index].size;
    
    if (using_16bit) {
        // For 16-bit DMA
        page = phys_addr >> 16;
        offset = (phys_addr >> 1) & 0xFFFF;  // 16-bit DMA uses word count
        
        // Disable DMA channel
        outportb(0xD4, (SB_DMA_16BIT_CHANNEL & 3) | 4);
        
        // Clear flip-flop
        outportb(0xD8, 0);
        
        // Set mode (single cycle, write, channel)
        outportb(0xD6, 0x48 | (SB_DMA_16BIT_CHANNEL & 3));
        
        // Set DMA address
        outportb(0xC0 + ((SB_DMA_16BIT_CHANNEL & 3) << 2), offset & 0xFF);
        outportb(0xC0 + ((SB_DMA_16BIT_CHANNEL & 3) << 2), offset >> 8);
        
        // Set DMA page
        switch (SB_DMA_16BIT_CHANNEL & 3) {
            case 0: outportb(0x87, page); break;  // DMA 4
            case 1: outportb(0x83, page); break;  // DMA 5
            case 2: outportb(0x81, page); break;  // DMA 6
            case 3: outportb(0x82, page); break;  // DMA 7
        }
        
        // Set DMA count (bytes - 1) for 16-bit DMA
        count = count / 2;  // Convert to 16-bit word count
        outportb(0xC2 + ((SB_DMA_16BIT_CHANNEL & 3) << 2), (count - 1) & 0xFF);
        outportb(0xC2 + ((SB_DMA_16BIT_CHANNEL & 3) << 2), (count - 1) >> 8);
        
        // Enable DMA channel
        outportb(0xD4, SB_DMA_16BIT_CHANNEL & 3);
    } else {
        // For 8-bit DMA
        page = phys_addr >> 16;
        offset = phys_addr & 0xFFFF;
        
        // Disable DMA channel
        outportb(0x0A, SB_DMA_CHANNEL | 4);
        
        // Clear flip-flop
        outportb(0x0C, 0);
        
        // Set mode (single cycle, write, channel)
        outportb(0x0B, 0x48 | SB_DMA_CHANNEL);
        
        // Set DMA address
        outportb(SB_DMA_CHANNEL << 1, offset & 0xFF);
        outportb(SB_DMA_CHANNEL << 1, offset >> 8);
        
        // Set DMA page register based on channel
        switch (SB_DMA_CHANNEL) {
            case 0: outportb(0x87, page); break;
            case 1: outportb(0x83, page); break;
            case 2: outportb(0x81, page); break;
            case 3: outportb(0x82, page); break;
        }
        
        // Set DMA count (bytes - 1)
        outportb((SB_DMA_CHANNEL << 1) + 1, (count - 1) & 0xFF);
        outportb((SB_DMA_CHANNEL << 1) + 1, (count - 1) >> 8);
        
        // Enable DMA channel
        outportb(0x0A, SB_DMA_CHANNEL);
    }
}

// Fill a DMA buffer with audio data
int fill_buffer(int buffer_index) {
    int bytes_to_read = dma_buffers[buffer_index].size;
    void *buffer_addr = dma_buffers[buffer_index].address;
    
    // Adjust bytes to read if near end of file
    if (bytes_read + bytes_to_read > data_size) {
        bytes_to_read = data_size - bytes_read;
        if (bytes_to_read <= 0) {
            return 0;  // End of file
        }
    }
    
    // Read data into buffer
    if (fread(buffer_addr, 1, bytes_to_read, wav_file) != bytes_to_read) {
        printf("Error reading WAV data\n");
        return 0;
    }
    
    // Pad with silence if not full
    if (bytes_to_read < dma_buffers[buffer_index].size) {
        if (bits_per_sample == 8) {
            // 8-bit silence is 128 (unsigned)
            memset((char *)buffer_addr + bytes_to_read, 128, 
                   dma_buffers[buffer_index].size - bytes_to_read);
        } else {
            // 16-bit silence is 0 (signed)
            memset((char *)buffer_addr + bytes_to_read, 0, 
                   dma_buffers[buffer_index].size - bytes_to_read);
        }
    }
    
    // Update bytes read counter
    bytes_read += bytes_to_read;
    
    return 1;  // Success
}

// Start playback of a buffer
void start_buffer_playback(int buffer_index) {
    // Program DMA controller
    program_dma(buffer_index);
    
    // Start playback
    if (using_16bit) {
        if (channels == 2) {
            // 16-bit stereo
            write_dsp(0xB6);  // DSP command for 16-bit playback
            write_dsp(0x30);  // Mode: signed stereo
            
            // For stereo, count is number of frames (stereo pairs)
            int count = dma_buffers[buffer_index].size / 4;  // 4 bytes per frame
            write_dsp((count - 1) & 0xFF);
            write_dsp((count - 1) >> 8);
        } else {
            // 16-bit mono
            write_dsp(0xB6);  // DSP command for 16-bit playback
            write_dsp(0x10);  // Mode: signed mono
            
            int count = dma_buffers[buffer_index].size / 2;  // 2 bytes per sample
            write_dsp((count - 1) & 0xFF);
            write_dsp((count - 1) >> 8);
        }
    } else {
        if (channels == 2) {
            // 8-bit stereo
            write_dsp(0xC6);  // DSP command for 8-bit playback
            write_dsp(0x20);  // Mode: unsigned stereo
            
            int count = dma_buffers[buffer_index].size / 2;  // 2 bytes per frame
            write_dsp((count - 1) & 0xFF);
            write_dsp((count - 1) >> 8);
        } else {
            // 8-bit mono
            write_dsp(0xC6);  // DSP command for 8-bit playback
            write_dsp(0x00);  // Mode: unsigned mono
            
            write_dsp((dma_buffers[buffer_index].size - 1) & 0xFF);
            write_dsp((dma_buffers[buffer_index].size - 1) >> 8);
        }
    }
    
    // Mark buffer as active
    dma_buffers[buffer_index].active = 1;
}

void calculate_total_playback_time(unsigned long data_size, unsigned short channels, 
                                  unsigned short bits_per_sample, unsigned long sample_rate,
                                  int *minutes, int *seconds) {
    unsigned long bytes_per_sample = bits_per_sample / 8;
    unsigned long bytes_per_frame = bytes_per_sample * channels;
    unsigned long total_frames = data_size / bytes_per_frame;
    
    // Calculate total seconds
    double total_seconds = (double)total_frames / sample_rate;
    
    // Convert to minutes and seconds
    *minutes = (int)(total_seconds / 60.0);
    *seconds = (int)(total_seconds - (*minutes * 60));
}

// Calculate milliseconds needed to play a buffer
int calculate_buffer_time(int buffer_size) {
    int bytes_per_sample = bits_per_sample / 8;
    int bytes_per_frame = bytes_per_sample * channels;
    return (buffer_size * 1000) / (sample_rate * bytes_per_frame);
}

// Play a WAV file with double-buffering for smooth playback
void play_wav(const char *filename) {
    WAV_HEADER header;
    int buffer_time_ms;
    int end_of_file = 0;
    
    printf("Playing %s...\n", filename);
    
    // Check SB version
    if (!reset_dsp() || !get_dsp_version(&dsp_version)) {
        printf("Failed to detect Sound Blaster\n");
        return;
    }
    
    // Sound Blaster 16 and above supports 16-bit audio
    printf("Sound Blaster DSP version %d.%d detected\n", dsp_version.major, dsp_version.minor);
    
    if (dsp_version.major >= 4) {
        printf("16-bit audio support: Yes\n");
    } else {
        printf("16-bit audio support: No (will use 8-bit)\n");
    }
    
    // Open the WAV file
    wav_file = fopen(filename, "rb");
    if (!wav_file) {
        printf("Failed to open file\n");
        return;
    }
    
    // Read the complete WAV header
    if (fread(&header, sizeof(WAV_HEADER), 1, wav_file) != 1) {
        printf("Failed to read WAV header\n");
        fclose(wav_file);
        return;
    }
    
    // Dump header info for debugging
    printf("--- WAV Header Info ---\n");
    printf("RIFF: %.4s\n", header.riff_header);
    printf("WAVE: %.4s\n", header.wave_header);
    printf("fmt : %.4s\n", header.fmt_header);
    printf("data: %.4s\n", header.data_header);
    printf("Format: %d\n", header.audio_format);
    printf("Channels: %d\n", header.num_channels);
    printf("Sample rate: %lu Hz\n", header.sample_rate);
    printf("Bits per sample: %d\n", header.bits_per_sample);
    printf("Data size: %lu bytes\n", header.data_size);
    int total_minutes, total_seconds;
    calculate_total_playback_time(header.data_size, header.num_channels, header.bits_per_sample, header.sample_rate,
                             &total_minutes, &total_seconds);
    printf("Total playback time: %d:%02d\n", total_minutes, total_seconds);
    printf("----------------------\n");
    
    // Check if this is a valid WAV file
    if (memcmp(header.riff_header, "RIFF", 4) != 0 || 
        memcmp(header.wave_header, "WAVE", 4) != 0 ||
        memcmp(header.fmt_header, "fmt ", 4) != 0 ||
        memcmp(header.data_header, "data", 4) != 0) {
        printf("Not a valid WAV file structure\n");
        fclose(wav_file);
        return;
    }
    
    // Check format - must be PCM
    if (header.audio_format != 1) {
        printf("Only PCM format (1) is supported, file has format %d\n", header.audio_format);
        fclose(wav_file);
        return;
    }
    
    // Store format information
    channels = header.num_channels;
    sample_rate = header.sample_rate;
    bits_per_sample = header.bits_per_sample;
    data_size = header.data_size;
    
    // Determine playback mode
    using_16bit = (bits_per_sample == 16) && (dsp_version.major >= 4);
    
    // Calculate optimal buffer size based on audio format
    dma_buffer_size = calculate_optimal_buffer_size(channels, bits_per_sample, sample_rate);

    // Calculate buffer playback time
    buffer_time_ms = calculate_buffer_time(dma_buffer_size);
        
    printf("Optimal buffer size: %d bytes, playback time: %d ms\n", 
           dma_buffer_size, buffer_time_ms);

    // Allocate DMA buffers
    if (!allocate_dma_buffers()) {
        printf("Failed to allocate DMA buffers\n");
        fclose(wav_file);
        return;
    }

    // Reset the Sound Blaster
    if (!reset_dsp()) {
        printf("Failed to reset Sound Blaster\n");
        fclose(wav_file);
        return;
    }
    
    // Set the mixer for stereo or mono output
    outportb(SB_MIXER_ADDR, 0x0E);  // Select output control register
    if (channels == 2) {
        outportb(SB_MIXER_DATA, 0x03);  // Stereo (both left and right enabled)
    } else {
        outportb(SB_MIXER_DATA, 0x01);  // Mono (left only)
    }
    
    // Turn on the speaker
    write_dsp(0xD1);
    
    // Set the sample rate - important for correct playback speed
    set_sample_rate(sample_rate);
    
    // Initialize bytes read counter
    bytes_read = 0;
    current_buffer = 0;
    
    // Fill both buffers to start
    if (!fill_buffer(0) || !fill_buffer(1)) {
        printf("Failed to fill initial buffers\n");
        fclose(wav_file);
        return;
    }
    
    printf("Playing in %s, %d-bit - press any key to stop...\n", 
           channels == 2 ? "stereo" : "mono",
           bits_per_sample);
    
    // Start the first buffer
    start_buffer_playback(0);
    
    // Main playback loop using double-buffering
    playing = 1;
    
    while (playing && !kbhit() && !end_of_file) {
        // Wait for the current buffer to finish
        // This is a simplified approach without interrupts
        int elapsed = 0;
        int next_buffer = (current_buffer + 1) % 2;
        
        while (elapsed < buffer_time_ms * 0.95 && !kbhit()) {  // 95% of buffer time
            delay(10);
            elapsed += 10;
        }
        
        // Switch buffers
        current_buffer = next_buffer;
        
        // Start playing the next buffer
        start_buffer_playback(current_buffer);
        
        // Fill the buffer that just finished playing
        next_buffer = (current_buffer + 1) % 2;
        if (!fill_buffer(next_buffer)) {
            end_of_file = 1;
        }
    }
    
    // Stop playback
    write_dsp(0xD5);  // Stop Sound Blaster output
    delay(50);
    
    // Clear any key press
    if (kbhit()) getch();
    
    // Turn off the speaker
    write_dsp(0xD3);
    
    // Reset the DSP
    reset_dsp();
    
    // Close the file
    fclose(wav_file);
    
    printf("Playback complete\n");
}

// Main function
int main(int argc, char *argv[]) {
    printf("Sound Blaster WAV Player with Double-Buffering\n");
    printf("==============================\n\n");
    
    // Check command line
    if (argc < 2) {
        printf("Usage: %s <wav_file>\n", argv[0]);
        return 1;
    }
    
    // Enable near pointers
    if (__djgpp_nearptr_enable() == 0) {
        printf("Failed to enable near pointers\n");
        return 1;
    }
    
    // Parse BLASTER environment variable to set Sound Blaster parameters
    parse_blaster_env();
        
    // Play the WAV file
    play_wav(argv[1]);
    
    // Free DMA buffers
    free_dma_buffers();
    
    // Disable near pointers
    __djgpp_nearptr_disable();
    
    return 0;
}
