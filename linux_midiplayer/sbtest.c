/**
 * soundblaster.c - Simple SoundBlaster DSP programming example using DJGPP
 *
 * This program demonstrates how to initialize the SoundBlaster card,
 * program the DSP (Digital Signal Processor), and play a simple sound.
 *
 * Compile with: gcc -o soundblaster.exe soundblaster.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <unistd.h>
#include <math.h>

/* SoundBlaster base addresses and settings */
#define SB_DEFAULT_BASE    0x220
#define SB_RESET           (SB_DEFAULT_BASE + 0x6)
#define SB_READ_DATA       (SB_DEFAULT_BASE + 0xA)
#define SB_WRITE_DATA      (SB_DEFAULT_BASE + 0xC)
#define SB_WRITE_STATUS    (SB_DEFAULT_BASE + 0xC)
#define SB_DATA_AVAILABLE  (SB_DEFAULT_BASE + 0xE)

/* SoundBlaster DSP commands */
#define DSP_SPEAKER_ON     0xD1
#define DSP_SPEAKER_OFF    0xD3
#define DSP_8BIT_OUT       0x10
#define DSP_GET_VERSION    0xE1

/* Sound sample definitions */
#define SAMPLE_RATE        8000  /* 8 KHz */
#define SAMPLE_SIZE        8000  /* 1 second of audio */

/* Function prototypes */
int sb_reset(void);
int sb_write_dsp(unsigned char value);
unsigned char sb_read_dsp(void);
void sb_speaker_on(void);
void sb_speaker_off(void);
void sb_play_sound(unsigned char *buffer, int length);
unsigned char *generate_beep(int length, int frequency);

/* Main function */
int main(void) {
    unsigned char *sound_buffer;
    
    printf("SoundBlaster programming example with DJGPP\n");
    printf("Resetting SoundBlaster card...\n");
    
    /* Initialize the SoundBlaster card */
    if (!sb_reset()) {
        printf("Failed to reset SoundBlaster card!\n");
        return 1;
    }
    
    /* Check DSP version */
    sb_write_dsp(DSP_GET_VERSION);
    printf("SoundBlaster DSP version: %d.%d\n", 
           sb_read_dsp(), sb_read_dsp());
    
    /* Generate a simple beep sound (440 Hz = A4 note) */
    printf("Generating sound buffer...\n");
    sound_buffer = generate_beep(SAMPLE_SIZE, 440);
    if (sound_buffer == NULL) {
        printf("Failed to generate sound buffer!\n");
        return 1;
    }
    
    /* Play the sound */
    printf("Playing sound...\n");
    sb_speaker_on();
    sb_play_sound(sound_buffer, SAMPLE_SIZE);
    sb_speaker_off();
    
    /* Clean up */
    free(sound_buffer);
    printf("Sound playback complete!\n");
    
    return 0;
}

/* Reset the SoundBlaster DSP */
int sb_reset(void) {
    outportb(SB_RESET, 1);
    delay(10);  /* Wait 10ms */
    outportb(SB_RESET, 0);
    delay(10);  /* Wait 10ms */
    
    /* Check if reset was successful */
    if (inportb(SB_DATA_AVAILABLE) & 0x80) {
        unsigned char value = inportb(SB_READ_DATA);
        return (value == 0xAA);
    }
    
    return 0;
}

/* Write a value to the DSP */
int sb_write_dsp(unsigned char value) {
    int timeout = 1000;
    
    /* Wait until the DSP is ready to receive data */
    while (timeout-- && (inportb(SB_WRITE_STATUS) & 0x80)) {
        delay(1);
    }
    
    if (timeout <= 0) {
        return 0;
    }
    
    /* Send the value */
    outportb(SB_WRITE_DATA, value);
    return 1;
}

/* Read a value from the DSP */
unsigned char sb_read_dsp(void) {
    int timeout = 1000;
    
    /* Wait until data is available */
    while (timeout-- && !(inportb(SB_DATA_AVAILABLE) & 0x80)) {
        delay(1);
    }
    
    if (timeout <= 0) {
        return 0;
    }
    
    /* Read the value */
    return inportb(SB_READ_DATA);
}

/* Turn on the SoundBlaster speaker */
void sb_speaker_on(void) {
    sb_write_dsp(DSP_SPEAKER_ON);
}

/* Turn off the SoundBlaster speaker */
void sb_speaker_off(void) {
    sb_write_dsp(DSP_SPEAKER_OFF);
}

/* Play a sound buffer through the SoundBlaster */
void sb_play_sound(unsigned char *buffer, int length) {
    int i;
    
    /* Send 8-bit direct output command */
    sb_write_dsp(DSP_8BIT_OUT);
    sb_write_dsp(length - 1);  /* Length - 1 */
    
    /* Send samples to the DSP */
    for (i = 0; i < length; i++) {
        outportb(SB_WRITE_DATA, buffer[i]);
        delay(1);  /* Small delay to ensure proper timing */
    }
}

/* Generate a simple sine wave beep sound */
unsigned char *generate_beep(int length, int frequency) {
    int i;
    double time, increment;
    unsigned char *buffer;
    
    /* Allocate memory for the sound buffer */
    buffer = (unsigned char *)malloc(length);
    if (buffer == NULL) {
        return NULL;
    }
    
    /* Generate a sine wave */
    increment = 2.0 * 3.14159265 * frequency / SAMPLE_RATE;
    for (i = 0, time = 0.0; i < length; i++, time += increment) {
        /* Scale sine wave to range 0-255 with center at 128 */
        buffer[i] = (unsigned char)(128.0 + 127.0 * sin(time));
    }
    
    return buffer;
}
