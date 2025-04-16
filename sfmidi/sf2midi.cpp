/* SF2MIDI.C - SoundFont 2 MIDI file player for MS-DOS/Sound Blaster
 * 
 * Compile with DJGPP:
 * gcc -O2 -o sf2midi.exe sf2midi.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <pc.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <math.h>
#include <time.h>

/* Sound Blaster I/O ports and configuration */
#define SB_BASE         0x220   /* Default Sound Blaster base address */
#define SB_RESET        0x226
#define SB_READ_DATA    0x22A
#define SB_WRITE_CMD    0x22C
#define SB_WRITE_STATUS 0x22C
#define SB_READ_STATUS  0x22E
#define SB_ACK_8BIT     0x22E
#define SB_IRQ          5
#define SB_DMA          1
#define SB_CMD_SPKR_ON  0xD1
#define SB_CMD_SPKR_OFF 0xD3
#define SB_CMD_GET_VER  0xE1
#define SB_CMD_DMA8_OUT 0x14    /* 8-bit DMA single-cycle output */
#define SB_CMD_DMA8_DAC 0x1C    /* 8-bit DMA auto-init output */
#define SB_CMD_HALT_DMA 0xD0

/* DMA controller ports for 8-bit transfers */
#define DMA_MASK_REG    0x0A
#define DMA_MODE_REG    0x0B
#define DMA_CLEAR_FF    0x0C
#define DMA_ADDR_REG(c) (((c) & 3) << 1)
#define DMA_COUNT_REG(c) (((c) & 3) << 1 | 1)
#define DMA_PAGE_REG(c) ((c) == 0 ? 0x87 : (c) == 1 ? 0x83 : (c) == 2 ? 0x81 : 0x82)

/* Maximum number of instruments and samples to support */
#define MAX_PRESETS     2560
#define MAX_INSTRUMENTS 2560
#define MAX_SAMPLES     5120
#define MAX_MIDI_TRACKS 2400
#define MAX_MIDI_EVENTS 2000
#define BUFFER_SIZE     512

/* Maximum polyphony (simultaneous notes) */
#define MAX_VOICES      8

/* Maximum number of active notes to track */
#define MAX_NOTES       128

/* DMA buffer must not cross a 64K boundary */
#define DMA_BUFFER_SIZE 8192

/* Simplified structures for SF2 parsing */
typedef struct {
    char id[4];
    unsigned long size;
} ChunkHeader;

/* Simplified sample data */
typedef struct {
    char name[20];
    unsigned long start;      /* Start offset in sample data */
    unsigned long end;        /* End offset in sample data */
    unsigned long loopStart;  /* Loop start point */
    unsigned long loopEnd;    /* Loop end point */
    unsigned long sampleRate; /* Sample rate in Hz */
    unsigned char originalPitch; /* Original MIDI key number */
    char pitchCorrection;     /* Pitch correction in cents */
    unsigned short sampleLink;/* Stereo link */
    unsigned short sampleType;/* 1=mono, 2=right, 4=left */
} SF2Sample;

/* Simplified instrument data */
typedef struct {
    char name[20];
    int sampleIndex;        /* Index of first sample for this instrument */
    int sampleCount;        /* Number of samples for this instrument */
    int keyRangeStart[32];  /* First key in range for each sample */
    int keyRangeEnd[32];    /* Last key in range for each sample */
    int sampleIndex2[32];   /* Sample index for each key range */
    int velRangeStart[32];  /* First velocity in range for each sample */
    int velRangeEnd[32];    /* Last velocity in range for each sample */
} SF2Instrument;

/* Simplified preset data */
typedef struct {
    char name[20];
    unsigned short preset;   /* MIDI program number */
    unsigned short bank;     /* MIDI bank number */
    int instrumentIndex;     /* Index to instrument */
} SF2Preset;

/* MIDI event structure */
typedef struct {
    unsigned long deltaTime;  /* Delta time in ticks */
    unsigned long absTime;    /* Absolute time in ticks */
    unsigned char status;     /* Status byte */
    unsigned char channel;    /* MIDI channel (extracted from status) */
    unsigned char data1;      /* First data byte */
    unsigned char data2;      /* Second data byte */
    unsigned char* metaData;  /* Pointer to meta event data */
    unsigned int metaLength;  /* Length of meta event data */
} MIDIEvent;

/* MIDI track structure */
typedef struct {
    unsigned long length;     /* Track length in bytes */
    unsigned long position;   /* Current position in track */
    unsigned long nextEvent;  /* Absolute time of next event in ticks */
    unsigned char endOfTrack; /* Flag to indicate end of track */
    MIDIEvent* events;        /* Array of events in the track */
    int eventCount;           /* Number of events in the track */
    int currentEvent;         /* Current event index */
} MIDITrack;

/* MIDI file structure */
typedef struct {
    unsigned short format;    /* MIDI file format (0, 1, or 2) */
    unsigned short numTracks; /* Number of tracks */
    unsigned short division;  /* Time division (ticks per quarter note) */
    MIDITrack tracks[MAX_MIDI_TRACKS]; /* Track data */
    unsigned long tempo;      /* Current tempo in microseconds per quarter note */
    double currentTime;       /* Current playback time in seconds */
} MIDIFile;

/* Voice allocation structure */
typedef struct {
    int active;              /* 1 if voice is playing, 0 if free */
    int channel;             /* MIDI channel */
    int note;                /* MIDI note number */
    int velocity;            /* MIDI velocity */
    SF2Sample* sample;       /* Pointer to sample being played */
    unsigned long position;  /* Current position in sample */
    unsigned long increment; /* Position increment per frame */
} Voice;

/* Global data storage */
SF2Sample g_samples[MAX_SAMPLES];
SF2Instrument g_instruments[MAX_INSTRUMENTS];
SF2Preset g_presets[MAX_PRESETS];
int g_sampleCount = 0;
int g_instrumentCount = 0;
int g_presetCount = 0;
unsigned long g_sampleDataOffset = 0; /* Offset to sample data in file */
FILE* g_sf2File = NULL;               /* Global SF2 file handle for playback */

/* DMA and Sound Blaster variables */
int sb_port = SB_BASE;
int sb_irq = SB_IRQ;
int sb_dma = SB_DMA;
int sb_detected = 0;
unsigned sb_version = 0;
unsigned char dma_buffer[DMA_BUFFER_SIZE + 32];
unsigned char *dma_buffer_aligned;
unsigned long dma_buffer_physical;
int dma_current_block = 0;
int dma_size = DMA_BUFFER_SIZE / 2;  /* Size of half the buffer */

/* Voice allocation */
Voice voices[MAX_VOICES];
int channelProgram[16] = {0};
int channelBank[16] = {0};
int activeNotes[16][MAX_NOTES] = {0}; /* Track active notes for each channel */

/* Forward declarations */
void stopNote(int channel, int note);

/* Functions to read data with correct endianness */
unsigned short readShort(FILE *file) {
    unsigned char buffer[2];
    fread(buffer, 1, 2, file);
    return (buffer[0]) | (buffer[1] << 8);
}

unsigned long readLong(FILE *file) {
    unsigned char buffer[4];
    fread(buffer, 1, 4, file);
    return (buffer[0]) | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
}

/* Read a variable-length value from MIDI file */
unsigned long readVarLen(FILE *file) {
    unsigned long value = 0;
    unsigned char c;
    
    if ((c = fgetc(file)) & 0x80) {
        value = c & 0x7F;
        do {
            value = (value << 7) + ((c = fgetc(file)) & 0x7F);
        } while (c & 0x80);
    } else {
        value = c;
    }
    
    return value;
}

/* Read a chunk header */
int readChunkHeader(FILE *file, ChunkHeader *header) {
    if (fread(header->id, 1, 4, file) != 4) {
        return 0;
    }
    header->size = readLong(file);
    return 1;
}

/* Compare chunk ID */
int compareID(const char *id1, const char *id2) {
    return strncmp(id1, id2, 4) == 0;
}

/* Find a LIST chunk with specific type */
int findListChunk(FILE *file, const char *listType, long *listStart, long *listEnd) {
    ChunkHeader header;
    char type[4];
    long filePos = ftell(file);
    
    /* Remember file position */
    fseek(file, 0, SEEK_SET);
    
    /* Find RIFF header */
    if (!readChunkHeader(file, &header) || !compareID(header.id, "RIFF")) {
        fseek(file, filePos, SEEK_SET);
        return 0;
    }
    
    long endPos = 8 + header.size; /* End of RIFF chunk */
    
    /* Check sfbk identifier */
    fread(type, 1, 4, file);
    if (!compareID(type, "sfbk")) {
        fseek(file, filePos, SEEK_SET);
        return 0;
    }
    
    /* Start searching at beginning of data */
    while (ftell(file) < endPos) {
        long chunkPos = ftell(file);
        
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "LIST")) {
            /* Read list type */
            fread(type, 1, 4, file);
            
            if (compareID(type, listType)) {
                *listStart = ftell(file);
                *listEnd = chunkPos + 8 + header.size;
                
                /* Restore file position */
                fseek(file, filePos, SEEK_SET);
                return 1;
            }
            
            /* Skip rest of list */
            fseek(file, header.size - 4, SEEK_CUR);
        } else {
            /* Skip non-LIST chunk */
            fseek(file, header.size, SEEK_CUR);
        }
    }
    
    /* Restore file position */
    fseek(file, filePos, SEEK_SET);
    return 0;
}

/* Find the sample data chunk and store its position */
int findSampleData(FILE *file) {
    long sdtaStart, sdtaEnd;
    ChunkHeader header;
    
    /* Find sdta LIST */
    if (!findListChunk(file, "sdta", &sdtaStart, &sdtaEnd)) {
        printf("Error: Cannot find sample data\n");
        return 0;
    }
    
    /* Find smpl chunk within sdta */
    fseek(file, sdtaStart, SEEK_SET);
    
    while (ftell(file) < sdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            printf("Error: Failed to read chunk in sdta\n");
            return 0;
        }
        
        if (compareID(header.id, "smpl")) {
            g_sampleDataOffset = ftell(file);
            printf("Found sample data at offset %lu, size %lu bytes\n", 
                   g_sampleDataOffset, header.size);
            return 1;
        }
        
        /* Skip to next chunk */
        fseek(file, header.size, SEEK_CUR);
    }
    
    printf("Error: Cannot find sample data chunk\n");
    return 0;
}

/* Read instrument data from the inst chunk */
int readInstruments(FILE *file, long pdtaStart, long pdtaEnd) {
    ChunkHeader header;
    long pos = ftell(file);
    
    /* Reset counters */
    g_instrumentCount = 0;
    
    /* First, find the inst chunk */
    fseek(file, pdtaStart, SEEK_SET);
    
    while (ftell(file) < pdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "inst")) {
            /* Calculate instrument count (minus terminal record) */
            g_instrumentCount = header.size / 22 - 1;
            
            if (g_instrumentCount > MAX_INSTRUMENTS) {
                printf("Warning: Too many instruments. Limiting to %d\n", MAX_INSTRUMENTS);
                g_instrumentCount = MAX_INSTRUMENTS;
            }
            
            printf("Reading %d instruments\n", g_instrumentCount);
            
            /* Read instrument names and bag indices */
            for (int i = 0; i < g_instrumentCount; i++) {
                /* Clear instrument data */
                memset(&g_instruments[i], 0, sizeof(SF2Instrument));
                
                /* Read name */
                fread(g_instruments[i].name, 1, 20, file);
                
                /* Read bag index (we don't use this directly) */
                readShort(file);
                
                /* Initialize instrument data */
                g_instruments[i].sampleCount = 0;
            }
            
            /* Skip terminal record */
            fseek(file, 22, SEEK_CUR);
            
            /* Restore position */
            fseek(file, pos, SEEK_SET);
            return 1;
        }
        
        /* Skip chunk */
        fseek(file, header.size, SEEK_CUR);
    }
    
    /* Restore position */
    fseek(file, pos, SEEK_SET);
    printf("Error: Cannot find instrument chunk\n");
    return 0;
}

/* Read preset data from the phdr chunk */
int readPresets(FILE *file, long pdtaStart, long pdtaEnd) {
    ChunkHeader header;
    long pos = ftell(file);
    
    /* Reset counters */
    g_presetCount = 0;
    
    /* First, find the phdr chunk */
    fseek(file, pdtaStart, SEEK_SET);
    
    while (ftell(file) < pdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "phdr")) {
            /* Calculate preset count (minus terminal record) */
            g_presetCount = header.size / 38 - 1;
            
            if (g_presetCount > MAX_PRESETS) {
                printf("Warning: Too many presets. Limiting to %d\n", MAX_PRESETS);
                g_presetCount = MAX_PRESETS;
            }
            
            printf("Reading %d presets\n", g_presetCount);
            
            /* Read preset data */
            for (int i = 0; i < g_presetCount; i++) {
                /* Read name */
                fread(g_presets[i].name, 1, 20, file);
                
                /* Read preset and bank */
                g_presets[i].preset = readShort(file);
                g_presets[i].bank = readShort(file);
                
                /* Skip preset bag index */
                readShort(file);
                
                /* Skip library, genre, morphology */
                readLong(file);
                readLong(file);
                readLong(file);
                
                /* For simplicity, set instrument index to match preset */
                g_presets[i].instrumentIndex = i % g_instrumentCount;
            }
            
            /* Skip terminal record */
            fseek(file, 38, SEEK_CUR);
            
            /* Restore position */
            fseek(file, pos, SEEK_SET);
            return 1;
        }
        
        /* Skip chunk */
        fseek(file, header.size, SEEK_CUR);
    }
    
    /* Restore position */
    fseek(file, pos, SEEK_SET);
    printf("Error: Cannot find preset chunk\n");
    return 0;
}

/* Read sample data from the shdr chunk */
int readSamples(FILE *file, long pdtaStart, long pdtaEnd) {
    ChunkHeader header;
    long pos = ftell(file);
    
    /* Reset counters */
    g_sampleCount = 0;
    
    /* First, find the shdr chunk */
    fseek(file, pdtaStart, SEEK_SET);
    
    while (ftell(file) < pdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "shdr")) {
            /* Calculate sample count (minus terminal record) */
            g_sampleCount = header.size / 46 - 1;
            
            if (g_sampleCount > MAX_SAMPLES) {
                printf("Warning: Too many samples. Limiting to %d\n", MAX_SAMPLES);
                g_sampleCount = MAX_SAMPLES;
            }
            
            printf("Reading %d samples\n", g_sampleCount);
            
            /* Read sample data */
            for (int i = 0; i < g_sampleCount; i++) {
                /* Read name */
                fread(g_samples[i].name, 1, 20, file);
                
                /* Read sample parameters */
                g_samples[i].start = readLong(file);
                g_samples[i].end = readLong(file);
                g_samples[i].loopStart = readLong(file);
                g_samples[i].loopEnd = readLong(file);
                g_samples[i].sampleRate = readLong(file);
                g_samples[i].originalPitch = fgetc(file);
                g_samples[i].pitchCorrection = fgetc(file);
                g_samples[i].sampleLink = readShort(file);
                g_samples[i].sampleType = readShort(file);
                
                /* Basic sample validation */
                if (i < 10) {
                    printf("Sample %d: %s, Start: %lu, End: %lu, Rate: %lu, Pitch: %d\n",
                           i, g_samples[i].name, g_samples[i].start, g_samples[i].end,
                           g_samples[i].sampleRate, g_samples[i].originalPitch);
                }
            }
            
            /* Skip terminal record */
            fseek(file, 46, SEEK_CUR);
            
            /* Restore position */
            fseek(file, pos, SEEK_SET);
            return 1;
        }
        
        /* Skip chunk */
        fseek(file, header.size, SEEK_CUR);
    }
    
    /* Restore position */
    fseek(file, pos, SEEK_SET);
    printf("Error: Cannot find sample headers chunk\n");
    return 0;
}

/* Create a simplified mapping from MIDI programs to samples */
void createMidiToSampleMapping() {
    printf("\nCreating MIDI program to sample mapping...\n");
    
    /* For this simple implementation, we'll just map:
     * - Each preset to its matching instrument index
     * - Each instrument has one sample (the first one available)
     */
    
    /* Assign one sample to each instrument based on name matching */
    for (int i = 0; i < g_instrumentCount; i++) {
        /* Initialize instrument data */
        g_instruments[i].sampleCount = 1;
        g_instruments[i].sampleIndex = i % g_sampleCount;
        
        /* Set up a simple key range (all keys use the same sample) */
        g_instruments[i].keyRangeStart[0] = 0;
        g_instruments[i].keyRangeEnd[0] = 127;
        g_instruments[i].sampleIndex2[0] = g_instruments[i].sampleIndex;
        g_instruments[i].velRangeStart[0] = 0;
        g_instruments[i].velRangeEnd[0] = 127;
    }
    
    /* Print out some examples of the mapping */
    printf("\nMIDI Program to Sample mapping examples:\n");
    for (int i = 0; i < 10 && i < g_presetCount; i++) {
        int instrumentIdx = g_presets[i].instrumentIndex;
        int sampleIdx = g_instruments[instrumentIdx].sampleIndex;
        
        printf("Bank %d, Program %d (%s) -> Instrument %d (%s) -> Sample %d (%s)\n",
               g_presets[i].bank, g_presets[i].preset, g_presets[i].name,
               instrumentIdx, g_instruments[instrumentIdx].name,
               sampleIdx, g_samples[sampleIdx].name);
    }
}

/* Load SF2 file and extract essential data */
int loadSF2(const char *filename) {
    long pdtaStart, pdtaEnd;
    
    /* Open SoundFont file */
    g_sf2File = fopen(filename, "rb");
    if (!g_sf2File) {
        printf("Error: Cannot open file %s\n", filename);
        return 0;
    }
    
    printf("Loading SoundFont file: %s\n", filename);
    
    /* Find sample data */
    if (!findSampleData(g_sf2File)) {
        fclose(g_sf2File);
        g_sf2File = NULL;
        return 0;
    }
    
    /* Find pdta LIST */
    if (!findListChunk(g_sf2File, "pdta", &pdtaStart, &pdtaEnd)) {
        printf("Error: Cannot find preset data\n");
        fclose(g_sf2File);
        g_sf2File = NULL;
        return 0;
    }
    
    printf("Found preset data from offset %ld to %ld\n", pdtaStart, pdtaEnd);
    
    /* Read instruments */
    if (!readInstruments(g_sf2File, pdtaStart, pdtaEnd)) {
        fclose(g_sf2File);
        g_sf2File = NULL;
        return 0;
    }
    
    /* Read presets */
    if (!readPresets(g_sf2File, pdtaStart, pdtaEnd)) {
        fclose(g_sf2File);
        g_sf2File = NULL;
        return 0;
    }
    
    /* Read samples */
    if (!readSamples(g_sf2File, pdtaStart, pdtaEnd)) {
        fclose(g_sf2File);
        g_sf2File = NULL;
        return 0;
    }
    
    /* Create the MIDI program to sample mapping */
    createMidiToSampleMapping();
    
    /* Keep the file open for sample playback */
    return 1;
}

/* Sound Blaster DSP functions */

/* Write a command to the DSP */
void SB_WriteCommand(unsigned char cmd) {
    int timeout;
    
    /* Wait until the DSP is ready to receive a command */
    for (timeout = 0; timeout < 100; timeout++) {
        if ((inportb(sb_port + 0xC) & 0x80) == 0)
            break;
        delay(1);
    }
    
    /* Send the command */
    outportb(sb_port + 0xC, cmd);
}

/* Read data from the DSP */
unsigned char SB_ReadData(void) {
    int timeout;
    
    /* Wait until there is data available */
    for (timeout = 0; timeout < 100; timeout++) {
        if (inportb(sb_port + 0xE) & 0x80)
            break;
        delay(1);
    }
    
    /* Read the data */
    return inportb(sb_port + 0xA);
}

/* Reset the DSP */
int SB_Reset(void) {
    /* Send a 1 to the reset port */
    outportb(sb_port + 0x6, 1);
    
    /* Wait a bit */
    delay(10);
    
    /* Send a 0 to the reset port */
    outportb(sb_port + 0x6, 0);
    
    /* Wait for data to become available */
    if (!(inportb(sb_port + 0xE) & 0x80)) {
        int timeout;
        for (timeout = 0; timeout < 100; timeout++) {
            delay(10);
            if (inportb(sb_port + 0xE) & 0x80)
                break;
        }
        if (timeout >= 100)
            return 0;
    }
    
    /* Read the data and check for 0xAA */
    if (inportb(sb_port + 0xA) != 0xAA)
        return 0;
    
    return 1;
}

/* Get the DSP version */
unsigned SB_GetVersion(void) {
    unsigned char major, minor;
    
    SB_WriteCommand(0xE1);
    major = SB_ReadData();
    minor = SB_ReadData();
    
    return (major << 8) | minor;
}

/* Setup Sound Blaster DSP */
int SB_Init(int basePort, int irq, int dma) {
    sb_port = basePort;
    sb_irq = irq;
    sb_dma = dma;
    
    printf("Initializing Sound Blaster: Port 0x%03X, IRQ %d, DMA %d\n", 
           sb_port, sb_irq, sb_dma);
    
    /* Enable memory access via near pointers */
    if (__djgpp_nearptr_enable() == 0) {
        printf("Error: Cannot enable near pointers\n");
        return 0;
    }
    
    /* Reset the DSP */
    if (!SB_Reset()) {
        printf("Error: Sound Blaster not detected\n");
        return 0;
    }
    
    /* Get version */
    sb_version = SB_GetVersion();
    printf("Sound Blaster version %d.%d detected\n", 
           (sb_version >> 8), (sb_version & 0xFF));
    
    /* Turn on speaker */
    SB_WriteCommand(0xD1);
    
    /* Align DMA buffer */
    dma_buffer_aligned = (unsigned char *)(((unsigned long)dma_buffer + 31) & ~31);
    dma_buffer_physical = __djgpp_conventional_base + (unsigned long)dma_buffer_aligned;
    
    sb_detected = 1;
    return 1;
}

/* Set up DMA for audio transfer */
void SB_SetupDMA(unsigned char *buffer, int count, int auto_init) {
    unsigned long addr = __djgpp_conventional_base + (unsigned long)buffer;
    unsigned int page = addr >> 16;
    unsigned int offset = addr & 0xFFFF;
    
    /* Disable DMA channel */
    outportb(DMA_MASK_REG, 0x04 | (sb_dma & 3));
    
    /* Clear flip-flop */
    outportb(DMA_CLEAR_FF, 0x00);
    
    /* Set mode (single or auto-init) */
    outportb(DMA_MODE_REG, (auto_init ? 0x58 : 0x48) | (sb_dma & 3));
    
    /* Set offset */
    outportb(DMA_ADDR_REG(sb_dma), offset & 0xFF);
    outportb(DMA_ADDR_REG(sb_dma), (offset >> 8) & 0xFF);
    
    /* Set page */
    outportb(DMA_PAGE_REG(sb_dma), page);
    
    /* Set count */
    outportb(DMA_COUNT_REG(sb_dma), (count - 1) & 0xFF);
    outportb(DMA_COUNT_REG(sb_dma), ((count - 1) >> 8) & 0xFF);
    
    /* Enable DMA channel */
    outportb(DMA_MASK_REG, (sb_dma & 3));
}

/* Program DSP to play a sound */
void SB_PlaySound(unsigned char *buffer, int count, int rate, int auto_init) {
    unsigned int time_constant;
    
    /* Set up DMA for transfer */
    SB_SetupDMA(buffer, count, auto_init);
    
    /* Calculate time constant */
    time_constant = 256 - (1000000 / rate);
    
    /* Set sample rate */
    SB_WriteCommand(0x40);
    SB_WriteCommand(time_constant);
    
    /* Play sound */
    if (auto_init) {
        /* 8-bit auto-init DMA digitized sound */
        SB_WriteCommand(0x1C);
    } else {
        /* 8-bit single-cycle DMA digitized sound */
        SB_WriteCommand(0x14);
    }
    
    /* Output length - 1 */
    SB_WriteCommand((count - 1) & 0xFF);
    SB_WriteCommand(((count - 1) >> 8) & 0xFF);
}

/* Stop sound playback */
void SB_StopSound(void) {
    /* Reset DSP */
    SB_Reset();
    
    /* Turn on speaker again */
    SB_WriteCommand(0xD1);
}

/* Get a sample for a given program and note */
SF2Sample* getSample(int bank, int program, int note, int velocity) {
    /* Find the matching preset */
    for (int i = 0; i < g_presetCount; i++) {
        if (g_presets[i].bank == bank && g_presets[i].preset == program) {
            /* Get the instrument */
            int instrumentIdx = g_presets[i].instrumentIndex;
            
            /* Find the matching key range */
            for (int j = 0; j < g_instruments[instrumentIdx].sampleCount; j++) {
                if (note >= g_instruments[instrumentIdx].keyRangeStart[j] &&
                    note <= g_instruments[instrumentIdx].keyRangeEnd[j] &&
                    velocity >= g_instruments[instrumentIdx].velRangeStart[j] &&
                    velocity <= g_instruments[instrumentIdx].velRangeEnd[j]) {
                    
                    /* Return the sample */
                    int sampleIdx = g_instruments[instrumentIdx].sampleIndex2[j];
                    return &g_samples[sampleIdx];
                }
            }
            
            /* If no specific range found, use the first sample */
            return &g_samples[g_instruments[instrumentIdx].sampleIndex];
        }
    }
    
    /* Default to first sample if no match found */
    return &g_samples[0];
}

/* Find a free voice for a new note */
int allocateVoice(int channel, int note, int velocity) {
    /* First look for any inactive voice */
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            return i;
        }
    }
    
    /* If no free voice, steal the oldest one */
    /* In a more advanced implementation, we'd consider note priority */
    return rand() % MAX_VOICES;
}

/* Start playing a note */
void startNote(int channel, int note, int velocity) {
    if (velocity == 0) {
        /* Note on with velocity 0 is actually a note off */
        stopNote(channel, note);
        return;
    }
    
    /* Get the sample for this note */
    SF2Sample *sample = getSample(channelBank[channel], channelProgram[channel], note, velocity);
    
    /* Find a voice to play this note */
    int voice = allocateVoice(channel, note, velocity);
    
    /* Set up the voice */
    voices[voice].active = 1;
    voices[voice].channel = channel;
    voices[voice].note = note;
    voices[voice].velocity = velocity;
    voices[voice].sample = sample;
    voices[voice].position = 0;
    
    /* Calculate position increment based on note frequency vs. sample rate */
    double noteFreq = 440.0 * pow(2.0, (note - 69) / 12.0);
    double sampleFreq = 440.0 * pow(2.0, (sample->originalPitch - 69) / 12.0);
    double ratio = noteFreq / sampleFreq;
    
    /* Position increment is in fixed-point format (16.16) */
    voices[voice].increment = (unsigned long)(ratio * 65536.0);
    
    /* Mark note as active */
    activeNotes[channel][note] = 1;
    
    printf("Note On: Ch %d, Note %d, Vel %d -> Sample %s\n",
           channel, note, velocity, sample->name);
}

/* Stop playing a note */
void stopNote(int channel, int note) {
    /* Find any voice playing this note on this channel */
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active && voices[i].channel == channel && voices[i].note == note) {
            /* Deactivate the voice */
            voices[i].active = 0;
            
            /* Mark note as inactive */
            activeNotes[channel][note] = 0;
            
            printf("Note Off: Ch %d, Note %d\n", channel, note);
            break;
        }
    }
}

/* Handle MIDI event during playback */
void handleMidiEvent(int command, int channel, int data1, int data2) {
    /* Process MIDI event */
    switch (command) {
        case 0x90: /* Note On */
            startNote(channel, data1, data2);
            break;
            
        case 0x80: /* Note Off */
            stopNote(channel, data1);
            break;
            
        case 0xC0: /* Program Change */
            channelProgram[channel] = data1;
            printf("Program Change: Ch %d, Program %d\n", channel, data1);
            break;
            
        case 0xB0: /* Control Change */
            if (data1 == 0) { /* Bank Select MSB */
                channelBank[channel] = (channelBank[channel] & 0x7F) | (data2 << 7);
                printf("Bank Select MSB: Ch %d, Bank %d\n", channel, channelBank[channel]);
            } else if (data1 == 32) { /* Bank Select LSB */
                channelBank[channel] = (channelBank[channel] & 0x3F80) | data2;
                printf("Bank Select LSB: Ch %d, Bank %d\n", channel, channelBank[channel]);
            }
            break;
    }
}

/* Function to handle meta events */
void handleMetaEvent(MIDIFile *midiFile, unsigned char type, unsigned char *data, unsigned int length) {
    switch (type) {
        case 0x51: /* Set Tempo */
            if (length >= 3) {
                midiFile->tempo = (data[0] << 16) | (data[1] << 8) | data[2];
                printf("Tempo change: %lu microseconds per quarter note\n", midiFile->tempo);
            }
            break;
            
        case 0x2F: /* End of Track */
            printf("End of Track\n");
            break;
            
        case 0x03: /* Track Name */
            printf("Track Name: ");
            for (unsigned int i = 0; i < length; i++) {
                printf("%c", data[i]);
            }
            printf("\n");
            break;
    }
}

/* Fill the audio buffer with samples from active voices */
void mixAudioBuffer(unsigned char *buffer, int size) {
    /* Clear buffer */
    memset(buffer, 128, size);
    
    /* Mix samples from all active voices */
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active) {
            SF2Sample *sample = voices[i].sample;
            unsigned long start = sample->start;
            unsigned long end = sample->end;
            unsigned long loopStart = sample->loopStart;
            unsigned long loopEnd = sample->loopEnd;
            
            for (int j = 0; j < size; j++) {
                /* Calculate sample position */
                unsigned long pos = start + (voices[i].position >> 16);
                
                /* Check if we've reached the end of the sample */
                if (pos >= end) {
                    if (loopEnd > loopStart) {
                        /* Loop the sample */
                        unsigned long loopLength = loopEnd - loopStart;
                        voices[i].position = ((pos - start) % loopLength + loopStart) << 16;
                        pos = start + (voices[i].position >> 16);
                    } else {
                        /* End the note */
                        voices[i].active = 0;
                        break;
                    }
                }
                
                /* Read sample data (8-bit) */
                short sample_data;
                if (g_sf2File) {
                    fseek(g_sf2File, g_sampleDataOffset + pos * 2, SEEK_SET);
                    sample_data = readShort(g_sf2File);
                    /* Convert from 16-bit to 8-bit, centered at 128 */
                    sample_data = (sample_data >> 8) + 128;
                } else {
                    sample_data = 128; /* Silence if no file */
                }
                
                /* Mix with existing buffer data (simple addition) */
                int mixed = buffer[j] + (((sample_data - 128) * voices[i].velocity) >> 7);
                
                /* Clamp to 8-bit range */
                if (mixed < 0) mixed = 0;
                if (mixed > 255) mixed = 255;
                
                buffer[j] = mixed;
                
                /* Increment position */
                voices[i].position += voices[i].increment;
            }
        }
    }
}

/* Pre-process MIDI file to load all events */
int loadMIDIEvents(FILE *midiFile, MIDIFile *midi) {
    ChunkHeader header;
    unsigned char buffer[BUFFER_SIZE];
    
    /* Initialize MIDI file data */
    memset(midi, 0, sizeof(MIDIFile));
    midi->tempo = 500000; /* Default tempo (120 BPM) */
    
    /* Read the MThd header */
    if (!readChunkHeader(midiFile, &header) || !compareID(header.id, "MThd")) {
        printf("Error: Not a valid MIDI file\n");
        return 0;
    }
    
    /* Read MIDI header data */
    midi->format = readShort(midiFile);
    midi->numTracks = readShort(midiFile);
    midi->division = readShort(midiFile);
    
    /* Skip any remaining header bytes */
    if (header.size > 6) {
        fseek(midiFile, header.size - 6, SEEK_CUR);
    }
    
    printf("MIDI Format: %d, Tracks: %d, Division: %d\n", 
           midi->format, midi->numTracks, midi->division);
    
    if (midi->numTracks > MAX_MIDI_TRACKS) {
        printf("Warning: Too many tracks. Limiting to %d\n", MAX_MIDI_TRACKS);
        midi->numTracks = MAX_MIDI_TRACKS;
    }
    
    /* Read each track */
    for (int t = 0; t < midi->numTracks; t++) {
        /* Read the MTrk header */
        if (!readChunkHeader(midiFile, &header) || !compareID(header.id, "MTrk")) {
            printf("Error: Invalid track header\n");
            return 0;
        }
        
        printf("Track %d, Length: %lu bytes\n", t, header.size);
        
        /* Initialize track */
        midi->tracks[t].length = header.size;
        midi->tracks[t].position = 0;
        midi->tracks[t].endOfTrack = 0;
        midi->tracks[t].eventCount = 0;
        
        /* Allocate memory for events (we'll count them first) */
        long trackStart = ftell(midiFile);
        long trackEnd = trackStart + header.size;
        unsigned long absTime = 0;
        unsigned char status = 0;
        int eventCount = 0;
        
        /* First pass - count events */
        while (ftell(midiFile) < trackEnd && eventCount < MAX_MIDI_EVENTS) {
            /* Read delta time */
            unsigned long delta = readVarLen(midiFile);
            absTime += delta;
            
            /* Store file position for this event */
            long eventPos = ftell(midiFile);
            
            /* Read status byte */
            unsigned char byte = fgetc(midiFile);
            
            /* Check for running status */
            if (byte < 0x80) {
                /* Use previous status byte */
                fseek(midiFile, -1, SEEK_CUR);
            } else {
                status = byte;
            }
            
            /* Process based on status byte */
            if (status < 0xF0) {
                /* Channel message */
                unsigned char command = status & 0xF0;
                unsigned char channel = status & 0x0F;
                unsigned char data1 = 0, data2 = 0;
                
                /* Read data bytes based on command */
                data1 = fgetc(midiFile);
                if (command != 0xC0 && command != 0xD0) {
                    /* Two data bytes */
                    data2 = fgetc(midiFile);
                }
                
                /* Store event */
                midi->tracks[t].events[eventCount].deltaTime = delta;
                midi->tracks[t].events[eventCount].absTime = absTime;
                midi->tracks[t].events[eventCount].status = status;
                midi->tracks[t].events[eventCount].channel = channel;
                midi->tracks[t].events[eventCount].data1 = data1;
                midi->tracks[t].events[eventCount].data2 = data2;
                midi->tracks[t].events[eventCount].metaData = NULL;
                midi->tracks[t].events[eventCount].metaLength = 0;
                
                eventCount++;
            } else if (status == 0xFF) {
                /* Meta event */
                unsigned char type = fgetc(midiFile);
                unsigned long len = readVarLen(midiFile);
                
                /* Allocate memory for meta data */
                unsigned char* metaData = NULL;
                if (len > 0) {
                    metaData = (unsigned char*)malloc(len);
                    if (metaData) {
                        fread(metaData, 1, len, midiFile);
                    } else {
                        fseek(midiFile, len, SEEK_CUR);
                    }
                }
                
                /* Store event */
                midi->tracks[t].events[eventCount].deltaTime = delta;
                midi->tracks[t].events[eventCount].absTime = absTime;
                midi->tracks[t].events[eventCount].status = status;
                midi->tracks[t].events[eventCount].channel = type; /* Store meta type in channel */
                midi->tracks[t].events[eventCount].data1 = 0;
                midi->tracks[t].events[eventCount].data2 = 0;
                midi->tracks[t].events[eventCount].metaData = metaData;
                midi->tracks[t].events[eventCount].metaLength = len;
                
                /* Special handling for end of track */
                if (type == 0x2F) {
                    midi->tracks[t].endOfTrack = 1;
                }
                
                eventCount++;
            } else if (status == 0xF0 || status == 0xF7) {
                /* SysEx event - we'll skip these for simplicity */
                unsigned long len = readVarLen(midiFile);
                fseek(midiFile, len, SEEK_CUR);
                
                eventCount++;
            }
        }
        
        midi->tracks[t].eventCount = eventCount;
        midi->tracks[t].currentEvent = 0;
        
        /* Calculate the time of the first event */
        if (eventCount > 0) {
            midi->tracks[t].nextEvent = midi->tracks[t].events[0].absTime;
        } else {
            midi->tracks[t].nextEvent = 0xFFFFFFFF; /* No events */
        }
    }
    
    /* Reset file position */
    fseek(midiFile, 0, SEEK_SET);
    
    return 1;
}

/* Load a MIDI file */
int loadMIDIFile(const char *filename, MIDIFile *midi) {
    FILE *file;
    
    /* Open MIDI file */
    file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open MIDI file %s\n", filename);
        return 0;
    }
    
    printf("Loading MIDI file: %s\n", filename);
    
    /* Process the file */
    if (!loadMIDIEvents(file, midi)) {
        fclose(file);
        return 0;
    }
    
    /* Close file */
    fclose(file);
    
    return 1;
}

/* Play the MIDI file */
void playMIDIFile(MIDIFile *midi) {
    double ticksPerSecond;
    double currentTick = 0;
    int allTracksEnd = 0;
    clock_t startTime, currentTime;
    int numActiveVoices = 0;
    
    /* Initialize buffer and voice state */
    memset(voices, 0, sizeof(voices));
    memset(activeNotes, 0, sizeof(activeNotes));
    for (int i = 0; i < 16; i++) {
        channelProgram[i] = 0;
        channelBank[i] = 0;
    }
    
    /* Calculate ticks per second based on tempo and division */
    ticksPerSecond = (double)midi->division * 1000000.0 / (double)midi->tempo;
    
    printf("Starting playback: %d tracks, %d ticks per beat, %.2f ticks per second\n",
           midi->numTracks, midi->division, ticksPerSecond);
    
    /* Initialize current event for each track */
    for (int t = 0; t < midi->numTracks; t++) {
        midi->tracks[t].currentEvent = 0;
    }
    
    /* Set up Sound Blaster for playback */
    int buffer_size = DMA_BUFFER_SIZE / 2;
    memset(dma_buffer_aligned, 128, DMA_BUFFER_SIZE); /* Fill with silence (128 = center) */
    
    /* Start DMA auto-init playback */
    SB_PlaySound(dma_buffer_aligned, DMA_BUFFER_SIZE, 22050, 1);
    
    /* Get start time */
    startTime = clock();
    
    /* Main playback loop */
    while (!allTracksEnd && !kbhit()) {
        /* Calculate current time and tick position */
        currentTime = clock();
        double elapsedSeconds = (double)(currentTime - startTime) / CLOCKS_PER_SEC;
        currentTick = elapsedSeconds * ticksPerSecond;
        
        /* Process events for all tracks */
        allTracksEnd = 1;
        for (int t = 0; t < midi->numTracks; t++) {
            MIDITrack *track = &midi->tracks[t];
            
            if (track->currentEvent < track->eventCount) {
                allTracksEnd = 0; /* At least one track still has events */
                
                /* Process all events that have reached their time */
                while (track->currentEvent < track->eventCount &&
                       track->events[track->currentEvent].absTime <= currentTick) {
                    
                    MIDIEvent *event = &track->events[track->currentEvent];
                    
                    /* Process the event */
                    if (event->status < 0xF0) {
                        /* Channel message */
                        unsigned char command = event->status & 0xF0;
                        unsigned char channel = event->status & 0x0F;
                        
                        /* Handle the MIDI event */
                        handleMidiEvent(command, channel, event->data1, event->data2);
                    } else if (event->status == 0xFF) {
                        /* Meta event */
                        handleMetaEvent(midi, event->channel, event->metaData, event->metaLength);
                        
                        /* Update tempo if needed */
                        if (event->channel == 0x51) { /* Tempo change */
                            /* Recalculate ticks per second */
                            ticksPerSecond = (double)midi->division * 1000000.0 / (double)midi->tempo;
                        }
                    }
                    
                    /* Move to next event */
                    track->currentEvent++;
                }
            }
        }
        
        /* Fill audio buffer with current voices */
        /* This would be done in an interrupt handler in a real implementation */
        /* For simplicity, we're just updating periodically */
        static int lastBufferUpdate = 0;
        if (currentTime - lastBufferUpdate > CLOCKS_PER_SEC / 40) { /* ~40Hz update rate */
            /* Fill the next half of the DMA buffer */
            mixAudioBuffer(dma_buffer_aligned + (dma_current_block ? buffer_size : 0), buffer_size);
            
            /* Toggle current block */
            dma_current_block = !dma_current_block;
            
            lastBufferUpdate = currentTime;
        }
        
        /* Simple CPU yield */
        delay(1);
    }
    
    /* Stop playback */
    SB_StopSound();
    
    /* Clean up allocated meta event data */
    for (int t = 0; t < midi->numTracks; t++) {
        MIDITrack *track = &midi->tracks[t];
        
        for (int i = 0; i < track->eventCount; i++) {
            if (track->events[i].metaData) {
                free(track->events[i].metaData);
            }
        }
        
        if (track->events) {
            free(track->events);
        }
    }
    
    if (kbhit()) {
        getch(); /* Clear the key press */
        printf("\nPlayback interrupted by user\n");
    } else {
        printf("\nPlayback complete\n");
    }
}

/* Main function */
int main(int argc, char *argv[]) {
    MIDIFile midi;
    char *sf2File = NULL;
    char *midiFile = NULL;
    int sbPort = SB_BASE;
    int sbIrq = SB_IRQ;
    int sbDma = SB_DMA;
    
    printf("SF2MIDI - SoundFont 2 MIDI Player for DOS\n");
    printf("Compiled with DJGPP on %s at %s\n\n", __DATE__, __TIME__);
    
    /* Check arguments */
    if (argc < 3) {
        printf("Usage: %s <soundfont.sf2> <midifile.mid> [sbport] [sbirq] [sbdma]\n", argv[0]);
        return 1;
    }
    
    sf2File = argv[1];
    midiFile = argv[2];
    
    /* Parse optional Sound Blaster parameters */
    if (argc > 3) sbPort = strtol(argv[3], NULL, 16);
    if (argc > 4) sbIrq = atoi(argv[4]);
    if (argc > 5) sbDma = atoi(argv[5]);
    
    /* Initialize Sound Blaster */
    if (!SB_Init(sbPort, sbIrq, sbDma)) {
        printf("Failed to initialize Sound Blaster. Check your settings.\n");
        return 1;
    }
    
    /* Load the soundfont */
    if (!loadSF2(sf2File)) {
        printf("Failed to load SF2 file: %s\n", sf2File);
        return 1;
    }
    
    printf("\nSF2 file loaded successfully!\n");
    
    /* Load the MIDI file */
    if (!loadMIDIFile(midiFile, &midi)) {
        printf("Failed to load MIDI file: %s\n", midiFile);
        fclose(g_sf2File);
        return 1;
    }
    
    printf("\nMIDI file loaded successfully!\n");
    
    /* Play the MIDI file */
    printf("\nPress any key to stop playback...\n");
    playMIDIFile(&midi);
    
    /* Clean up */
    if (g_sf2File) {
        fclose(g_sf2File);
    }
    
    /* Disable near pointers */
    __djgpp_nearptr_disable();
    
    printf("\nThank you for using SF2MIDI!\n");
    
    return 0;
}
