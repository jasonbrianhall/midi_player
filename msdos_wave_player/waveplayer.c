/**
 * Simple Sound Blaster WAV Player
 * Plays 8-bit mono WAV files using Sound Blaster DMA
 * Simplified version with minimal dependencies
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

// Sound Blaster settings
#define SB_BASE 0x220
#define SB_RESET (SB_BASE + 0x6)
#define SB_READ (SB_BASE + 0xA)
#define SB_WRITE (SB_BASE + 0xC)
#define SB_STATUS (SB_BASE + 0xE)

// DMA settings
#define DMA_CHANNEL 1
#define DMA_MASK 0x0A
#define DMA_MODE 0x0B
#define DMA_CLEAR_FF 0x0C
#define DMA_BUFFER_SIZE 8192  // 8K buffer

// WAV file header
typedef struct {
    char riff[4];
    unsigned long size;
    char wave[4];
    char fmt[4];
    unsigned long fmt_size;
    unsigned short format;
    unsigned short channels;
    unsigned long sample_rate;
    unsigned long bytes_per_sec;
    unsigned short block_align;
    unsigned short bits_per_sample;
    // May have additional fields or not, need to search for "data" chunk
} WAV_HEADER;

// Global variables
_go32_dpmi_seginfo dma_buffer;
void *buffer_addr;
FILE *wav_file;
unsigned long data_size = 0;
unsigned long bytes_read = 0;
volatile int playing = 0;
unsigned short sample_rate = 22050;

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

// Read from the DSP status port
unsigned char read_dsp_status(void) {
    return inportb(SB_STATUS);
}

// Allocate a DMA buffer
int allocate_dma_buffer(void) {
    // Allocate DOS memory block
    dma_buffer.size = (DMA_BUFFER_SIZE + 15) / 16;  // Size in paragraphs
    
    if (_go32_dpmi_allocate_dos_memory(&dma_buffer) != 0) {
        printf("Failed to allocate DOS memory\n");
        return 0;
    }
    
    // Calculate the linear address
    buffer_addr = (void *)((dma_buffer.rm_segment << 4) + __djgpp_conventional_base);
    
    return 1;
}

// Free the DMA buffer
void free_dma_buffer(void) {
    if (dma_buffer.size > 0) {
        _go32_dpmi_free_dos_memory(&dma_buffer);
        dma_buffer.size = 0;
    }
}

// Program the DMA controller
void program_dma(int count) {
    unsigned long phys_addr = dma_buffer.rm_segment << 4;
    unsigned char page = phys_addr >> 16;
    unsigned short offset = phys_addr & 0xFFFF;
    
    // Disable DMA channel
    outportb(DMA_MASK, DMA_CHANNEL | 4);
    
    // Clear flip-flop
    outportb(DMA_CLEAR_FF, 0);
    
    // Set mode (single cycle, write, channel)
    outportb(DMA_MODE, 0x48 | DMA_CHANNEL);
    
    // Set DMA address
    outportb(DMA_CHANNEL << 1, offset & 0xFF);
    outportb(DMA_CHANNEL << 1, offset >> 8);
    
    // Set DMA page
    outportb(0x83, page);  // Page register for DMA 1
    
    // Set DMA count (bytes - 1)
    outportb((DMA_CHANNEL << 1) + 1, (count - 1) & 0xFF);
    outportb((DMA_CHANNEL << 1) + 1, (count - 1) >> 8);
    
    // Enable DMA channel
    outportb(DMA_MASK, DMA_CHANNEL);
}

// Find the "data" chunk in a WAV file
int find_data_chunk(FILE *file) {
    char chunk_id[4];
    unsigned long chunk_size;
    
    // Skip to after the RIFF header
    fseek(file, 12, SEEK_SET);
    
    // Search for the "data" chunk
    while (!feof(file)) {
        if (fread(chunk_id, 1, 4, file) != 4)
            return 0;
            
        if (fread(&chunk_size, 4, 1, file) != 1)
            return 0;
            
        if (memcmp(chunk_id, "data", 4) == 0) {
            data_size = chunk_size;
            return 1;
        }
        
        // Skip this chunk
        fseek(file, chunk_size, SEEK_CUR);
    }
    
    return 0;
}

// Wait for playback to complete - a simplified and reliable approach
void wait_for_playback_completion(void) {
    // Calculate time needed to play buffer in milliseconds
    // This is a slightly conservative estimate to ensure playback is complete
    int play_time_ms = (DMA_BUFFER_SIZE * 1000) / sample_rate;
    
    int elapsed = 0;
    while (elapsed < play_time_ms && !kbhit()) {
        delay(9);
        elapsed += 10;
        
        // Check if DSP has an interrupt pending - break early if done
        if (inportb(SB_STATUS) & 0x80) {
            inportb(SB_READ);  // Acknowledge
            break;
        }
    }
}

// Play a WAV file
void play_wav(const char *filename) {
    WAV_HEADER header;
    int bytes_to_read;
    
    printf("Playing %s...\n", filename);
    
    // Open the WAV file
    wav_file = fopen(filename, "rb");
    if (!wav_file) {
        printf("Failed to open file\n");
        return;
    }
    
    // Read the WAV header
    if (fread(&header, sizeof(WAV_HEADER), 1, wav_file) != 1) {
        printf("Failed to read WAV header\n");
        fclose(wav_file);
        return;
    }
    
    // Check if this is a valid WAV file
    if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0) {
        printf("Not a valid WAV file\n");
        fclose(wav_file);
        return;
    }
    
    // Check format
    if (header.format != 1) {
        printf("Only PCM format is supported\n");
        fclose(wav_file);
        return;
    }
    
    if (header.channels != 1) {
        printf("Only mono files are supported\n");
        fclose(wav_file);
        return;
    }
    
    if (header.bits_per_sample != 8) {
        printf("Only 8-bit samples are supported\n");
        fclose(wav_file);
        return;
    }
    
    // Get sample rate
    sample_rate = header.sample_rate;
    
    // Find the data chunk
    if (!find_data_chunk(wav_file)) {
        printf("Data chunk not found\n");
        fclose(wav_file);
        return;
    }
    
    printf("WAV info: Sample rate = %d Hz, Data size = %lu bytes\n", 
           sample_rate, data_size);
    
    // Reset the Sound Blaster
    if (!reset_dsp()) {
        printf("Failed to reset Sound Blaster\n");
        fclose(wav_file);
        return;
    }
    
    // Turn on the speaker
    write_dsp(0xD1);
    
    // Set the sample rate
    write_dsp(0x40);
    write_dsp(256 - (1000000 / sample_rate));
    
    // Reset bytes read counter
    bytes_read = 0;
    
    // Main playback loop
    printf("Press any key to stop...\n");
    playing = 1;
    
    while (playing && bytes_read < data_size && !kbhit()) {
        // Read a block of data into the DMA buffer
        bytes_to_read = DMA_BUFFER_SIZE;
        if (bytes_read + bytes_to_read > data_size) {
            bytes_to_read = data_size - bytes_read;
        }
        
        printf("Reading block: %d bytes at position %lu\n", bytes_to_read, bytes_read);
        if (fread(buffer_addr, 1, bytes_to_read, wav_file) != (size_t)bytes_to_read) {
            printf("Failed to read data\n");
            break;
        }
        
        // If we didn't fill the buffer, pad with silence
        if (bytes_to_read < DMA_BUFFER_SIZE) {
            memset((char *)buffer_addr + bytes_to_read, 128, DMA_BUFFER_SIZE - bytes_to_read);
        }
        
        // Program the DMA controller
        program_dma(DMA_BUFFER_SIZE);
        
        // Start playback
        printf("Starting playback chunk: %lu of %lu bytes\n", bytes_read, data_size);
        write_dsp(0x14);  // 8-bit single cycle playback
        write_dsp((DMA_BUFFER_SIZE - 1) & 0xFF);
        write_dsp((DMA_BUFFER_SIZE - 1) >> 8);
        
        // Wait for playback to complete
        wait_for_playback_completion();
        
        bytes_read += bytes_to_read;
        printf("Completed chunk. Total bytes played: %lu of %lu\n", bytes_read, data_size);
    }
    
    // Clear any key press
    if (kbhit()) getch();
    
    // Turn off the speaker
    write_dsp(0xD3);
    
    // Reset the DSP
    //reset_dsp();
    
    // Close the file
    fclose(wav_file);
    
    printf("Playback complete\n");
}

// Main function
int main(int argc, char *argv[]) {
    printf("Simple Sound Blaster WAV Player\n");
    printf("==============================\n\n");
    printf("Configuration: Address=0x220, IRQ=7, DMA=1\n\n");
    
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
    
    // Allocate DMA buffer
    if (!allocate_dma_buffer()) {
        printf("Failed to allocate DMA buffer\n");
        __djgpp_nearptr_disable();
        return 1;
    }
    
    // Play the WAV file
    play_wav(argv[1]);
    
    // Free DMA buffer
    free_dma_buffer();
    
    // Disable near pointers
    __djgpp_nearptr_disable();
    
    return 0;
}
