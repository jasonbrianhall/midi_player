#include <stdio.h>
#include <stdlib.h>
#include <dpmi.h>
#include <conio.h>
#include <dos.h>
#include <sys/nearptr.h>
#include <string.h>

// WAV File and Sound Blaster Structures
struct WaveData {
    unsigned int SoundLength, Frequency;
    char *Sample;
};

struct HeaderType {
    long RIFF;
    char NI1[4];
    long WAVE;
    long fmt;
    char NI2[6];
    unsigned int Channels;
    long Frequency;
    char NI3[6];
    unsigned int BitRes;
    long data;
    long datasize;
} Header;

struct WaveData Voice;
unsigned int SB_BASE; // Sound Blaster Base Address
char WaveFile[25];

// Function Prototypes
char ResetDSP(unsigned int Test);
void WriteDSP(unsigned char Value);
void PlayBack(struct WaveData *Wave);
int LoadVoice(struct WaveData *Voice, char *FileName);
void allocate_dma_buffer(size_t size);

// DMA Buffer
unsigned char *dma_buffer = NULL;

// Resets DSP and Detects Sound Blaster
char ResetDSP(unsigned int Test) {
    outportb(Test + 0x6, 1);
    delay(10);
    outportb(Test + 0x6, 0);
    delay(10);
    if ((inportb(Test + 0xE) & 0x80) == 0x80 && inportb(Test + 0xA) == 0xAA) {
        SB_BASE = Test;
        return 1;
    }
    return 0;
}

// Writes a Byte to DSP
void WriteDSP(unsigned char Value) {
    while (inportb(SB_BASE + 0xC) & 0x80); // Wait until ready
    outportb(SB_BASE + 0xC, Value);
}

// Allocates a Memory Buffer Using Malloc
void allocate_dma_buffer(size_t size) {
    dma_buffer = (unsigned char *)malloc(size);
    if (!dma_buffer) {
        printf("Failed to allocate memory for DMA buffer.\n");
        exit(1);
    }
}

// Plays Back WAV Data
void PlayBack(struct WaveData *Wave) {
    unsigned char TimeConstant = (65536 - (256000000 / Wave->Frequency)) >> 8;
    WriteDSP(0x40); // Set sample frequency
    WriteDSP(TimeConstant);

    // Setup DMA
    allocate_dma_buffer(Wave->SoundLength);
    memcpy(dma_buffer, Wave->Sample, Wave->SoundLength);

    // Enable Speaker
    WriteDSP(0xD1);

    // Send Playback Command
    WriteDSP(0x14); // Single-cycle playback
    WriteDSP(Wave->SoundLength & 0xFF);
    WriteDSP(Wave->SoundLength >> 8);

    printf("Playing... Press any key to stop.\n");
    getch();

    // Stop Playback
    WriteDSP(0xD0); // Stop DMA
    WriteDSP(0xD3); // Turn off speaker
}

// Loads a WAV File
int LoadVoice(struct WaveData *Voice, char *FileName) {
    FILE *WAVFile = fopen(FileName, "rb");
    if (!WAVFile) {
        printf("Unable to open file: %s\n", FileName);
        return 0;
    }

    // Read and Validate Header
    fread(&Header, sizeof(Header), 1, WAVFile);
    printf("Header: %x %x %x %x %i %i\n", Header.RIFF, Header.WAVE, Header.fmt, Header.data, Header.Channels,Header.BitRes);
/*    if (Header.RIFF != 0x46464952 || Header.WAVE != 0x45564157 ||
        Header.fmt != 0x20746D66 || Header.data != 0x61746164) { */
    if (Header.RIFF != 0x46464952 || Header.WAVE != 0x45564157 ||
        Header.fmt != 0x20746D66) {
        
        printf("Invalid or unsupported WAV file.\n");
        fclose(WAVFile);
        return 0;
    }
    /*if (Header.Channels != 1 || Header.BitRes != 8) {
        printf("Only 8-bit mono WAV files are supported.\n");
        fclose(WAVFile);
        return 0;
    }*/

    // Load Sample Data
    printf("%i %i\n", Voice->SoundLength, Voice->Frequency);
    Voice->SoundLength = Header.datasize;
    Voice->Frequency = Header.Frequency;
    Voice->Sample = (char *)malloc(Voice->SoundLength);
    if (!Voice->Sample) {
        printf("Memory allocation error.\n");
        fclose(WAVFile);
        return 0;
    }
    fread(Voice->Sample, Voice->SoundLength, 1, WAVFile);
    fclose(WAVFile);
    return 1;
}

int main() {
    if (!__djgpp_nearptr_enable()) {
        printf("Failed to enable near pointer access.\n");
        return 1;
    }

    printf("Detecting Sound Blaster...\n");
    if (ResetDSP(0x220)) {
        printf("Sound Blaster found at 220h.\n");
    } else if (ResetDSP(0x240)) {
        printf("Sound Blaster found at 240h.\n");
    } else {
        printf("No Sound Blaster detected.\n");
        return 1;
    }

    printf("Enter WAV file name: ");
    gets(WaveFile);

    if (LoadVoice(&Voice, WaveFile)) {
        PlayBack(&Voice);
        free(Voice.Sample);
    }

    __djgpp_nearptr_disable();
    return 0;
}

