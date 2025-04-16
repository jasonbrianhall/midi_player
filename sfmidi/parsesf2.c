// sf2_minimal.c - Minimal SoundFont 2 parser for MS-DOS MIDI player integration
// Compile with: gcc -o sf2_minimal sf2_minimal.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of instruments and samples to support
#define MAX_PRESETS 1024
#define MAX_INSTRUMENTS 4096
#define MAX_SAMPLES 8192

// Simplified structures for SF2 parsing
typedef struct {
    char id[4];
    unsigned long size;
} ChunkHeader;

// Simplified sample data
typedef struct {
    char name[20];
    unsigned long start;      // Start offset in sample data
    unsigned long end;        // End offset in sample data
    unsigned long loopStart;  // Loop start point
    unsigned long loopEnd;    // Loop end point
    unsigned long sampleRate; // Sample rate in Hz
    unsigned char originalPitch; // Original MIDI key number
    char pitchCorrection;     // Pitch correction in cents
    unsigned short sampleLink;// Stereo link
    unsigned short sampleType;// 1=mono, 2=right, 4=left
} SF2Sample;

// Simplified instrument data
typedef struct {
    char name[20];
    int sampleIndex;        // Index of first sample for this instrument
    int sampleCount;        // Number of samples for this instrument
    int keyRangeStart[128]; // First key in range for each sample
    int keyRangeEnd[128];   // Last key in range for each sample
    int sampleIndex2[128];  // Sample index for each key range
    int velRangeStart[128]; // First velocity in range for each sample
    int velRangeEnd[128];   // Last velocity in range for each sample
} SF2Instrument;

// Simplified preset data
typedef struct {
    char name[20];
    unsigned short preset;   // MIDI program number
    unsigned short bank;     // MIDI bank number
    int instrumentIndex;     // Index to instrument
} SF2Preset;

// Global data storage
SF2Sample g_samples[MAX_SAMPLES];
SF2Instrument g_instruments[MAX_INSTRUMENTS];
SF2Preset g_presets[MAX_PRESETS];
int g_sampleCount = 0;
int g_instrumentCount = 0;
int g_presetCount = 0;
unsigned long g_sampleDataOffset = 0; // Offset to sample data in file

// Functions to read data with correct endianness
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

// Read a chunk header
int readChunkHeader(FILE *file, ChunkHeader *header) {
    if (fread(header->id, 1, 4, file) != 4) {
        return 0;
    }
    header->size = readLong(file);
    return 1;
}

// Compare chunk ID
int compareID(const char *id1, const char *id2) {
    return strncmp(id1, id2, 4) == 0;
}

// Find a LIST chunk with specific type
int findListChunk(FILE *file, const char *listType, long *listStart, long *listEnd) {
    ChunkHeader header;
    char type[4];
    long filePos = ftell(file);
    
    // Remember file position
    fseek(file, 0, SEEK_SET);
    
    // Find RIFF header
    if (!readChunkHeader(file, &header) || !compareID(header.id, "RIFF")) {
        fseek(file, filePos, SEEK_SET);
        return 0;
    }
    
    long endPos = 8 + header.size; // End of RIFF chunk
    
    // Check sfbk identifier
    fread(type, 1, 4, file);
    if (!compareID(type, "sfbk")) {
        fseek(file, filePos, SEEK_SET);
        return 0;
    }
    
    // Start searching at beginning of data
    while (ftell(file) < endPos) {
        long chunkPos = ftell(file);
        
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "LIST")) {
            // Read list type
            fread(type, 1, 4, file);
            
            if (compareID(type, listType)) {
                *listStart = ftell(file);
                *listEnd = chunkPos + 8 + header.size;
                
                // Restore file position
                fseek(file, filePos, SEEK_SET);
                return 1;
            }
            
            // Skip rest of list
            fseek(file, header.size - 4, SEEK_CUR);
        } else {
            // Skip non-LIST chunk
            fseek(file, header.size, SEEK_CUR);
        }
    }
    
    // Restore file position
    fseek(file, filePos, SEEK_SET);
    return 0;
}

// Find the sample data chunk and store its position
int findSampleData(FILE *file) {
    long sdtaStart, sdtaEnd;
    ChunkHeader header;
    
    // Find sdta LIST
    if (!findListChunk(file, "sdta", &sdtaStart, &sdtaEnd)) {
        printf("Error: Cannot find sample data\n");
        return 0;
    }
    
    // Find smpl chunk within sdta
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
        
        // Skip to next chunk
        fseek(file, header.size, SEEK_CUR);
    }
    
    printf("Error: Cannot find sample data chunk\n");
    return 0;
}

// Read instrument data from the inst chunk
int readInstruments(FILE *file, long pdtaStart, long pdtaEnd) {
    ChunkHeader header;
    long pos = ftell(file);
    
    // Reset counters
    g_instrumentCount = 0;
    
    // First, find the inst chunk
    fseek(file, pdtaStart, SEEK_SET);
    
    while (ftell(file) < pdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "inst")) {
            // Calculate instrument count (minus terminal record)
            g_instrumentCount = header.size / 22 - 1;
            
            if (g_instrumentCount > MAX_INSTRUMENTS) {
                printf("Warning: Too many instruments. Limiting to %d\n", MAX_INSTRUMENTS);
                g_instrumentCount = MAX_INSTRUMENTS;
            }
            
            printf("Reading %d instruments\n", g_instrumentCount);
            
            // Read instrument names and bag indices
            for (int i = 0; i < g_instrumentCount; i++) {
                // Clear instrument data
                memset(&g_instruments[i], 0, sizeof(SF2Instrument));
                
                // Read name
                fread(g_instruments[i].name, 1, 20, file);
                
                // Read bag index (we don't use this directly)
                readShort(file);
                
                // Initialize instrument data
                g_instruments[i].sampleCount = 0;
            }
            
            // Skip terminal record
            fseek(file, 22, SEEK_CUR);
            
            // Restore position
            fseek(file, pos, SEEK_SET);
            return 1;
        }
        
        // Skip chunk
        fseek(file, header.size, SEEK_CUR);
    }
    
    // Restore position
    fseek(file, pos, SEEK_SET);
    printf("Error: Cannot find instrument chunk\n");
    return 0;
}

// Read preset data from the phdr chunk
int readPresets(FILE *file, long pdtaStart, long pdtaEnd) {
    ChunkHeader header;
    long pos = ftell(file);
    
    // Reset counters
    g_presetCount = 0;
    
    // First, find the phdr chunk
    fseek(file, pdtaStart, SEEK_SET);
    
    while (ftell(file) < pdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "phdr")) {
            // Calculate preset count (minus terminal record)
            g_presetCount = header.size / 38 - 1;
            
            if (g_presetCount > MAX_PRESETS) {
                printf("Warning: Too many presets. Limiting to %d\n", MAX_PRESETS);
                g_presetCount = MAX_PRESETS;
            }
            
            printf("Reading %d presets\n", g_presetCount);
            
            // Read preset data
            for (int i = 0; i < g_presetCount; i++) {
                // Read name
                fread(g_presets[i].name, 1, 20, file);
                
                // Read preset and bank
                g_presets[i].preset = readShort(file);
                g_presets[i].bank = readShort(file);
                
                // Skip preset bag index
                readShort(file);
                
                // Skip library, genre, morphology
                readLong(file);
                readLong(file);
                readLong(file);
                
                // For simplicity, set instrument index to match preset
                g_presets[i].instrumentIndex = i % g_instrumentCount;
            }
            
            // Skip terminal record
            fseek(file, 38, SEEK_CUR);
            
            // Restore position
            fseek(file, pos, SEEK_SET);
            return 1;
        }
        
        // Skip chunk
        fseek(file, header.size, SEEK_CUR);
    }
    
    // Restore position
    fseek(file, pos, SEEK_SET);
    printf("Error: Cannot find preset chunk\n");
    return 0;
}

// Read sample data from the shdr chunk
int readSamples(FILE *file, long pdtaStart, long pdtaEnd) {
    ChunkHeader header;
    long pos = ftell(file);
    
    // Reset counters
    g_sampleCount = 0;
    
    // First, find the shdr chunk
    fseek(file, pdtaStart, SEEK_SET);
    
    while (ftell(file) < pdtaEnd) {
        if (!readChunkHeader(file, &header)) {
            break;
        }
        
        if (compareID(header.id, "shdr")) {
            // Calculate sample count (minus terminal record)
            g_sampleCount = header.size / 46 - 1;
            
            if (g_sampleCount > MAX_SAMPLES) {
                printf("Warning: Too many samples. Limiting to %d\n", MAX_SAMPLES);
                g_sampleCount = MAX_SAMPLES;
            }
            
            printf("Reading %d samples\n", g_sampleCount);
            
            // Read sample data
            for (int i = 0; i < g_sampleCount; i++) {
                // Read name
                fread(g_samples[i].name, 1, 20, file);
                
                // Read sample parameters
                g_samples[i].start = readLong(file);
                g_samples[i].end = readLong(file);
                g_samples[i].loopStart = readLong(file);
                g_samples[i].loopEnd = readLong(file);
                g_samples[i].sampleRate = readLong(file);
                g_samples[i].originalPitch = fgetc(file);
                g_samples[i].pitchCorrection = fgetc(file);
                g_samples[i].sampleLink = readShort(file);
                g_samples[i].sampleType = readShort(file);
                
                // Basic sample validation
                if (i < 10) {
                    printf("Sample %d: %s, Start: %lu, End: %lu, Rate: %lu, Pitch: %d\n",
                           i, g_samples[i].name, g_samples[i].start, g_samples[i].end,
                           g_samples[i].sampleRate, g_samples[i].originalPitch);
                }
            }
            
            // Skip terminal record
            fseek(file, 46, SEEK_CUR);
            
            // Restore position
            fseek(file, pos, SEEK_SET);
            return 1;
        }
        
        // Skip chunk
        fseek(file, header.size, SEEK_CUR);
    }
    
    // Restore position
    fseek(file, pos, SEEK_SET);
    printf("Error: Cannot find sample headers chunk\n");
    return 0;
}

// Create a simplified mapping from MIDI programs to samples
void createMidiToSampleMapping() {
    printf("\nCreating MIDI program to sample mapping...\n");
    
    // For this simple implementation, we'll just map:
    // - Each preset to its matching instrument index
    // - Each instrument has one sample (the first one available)
    
    // Assign one sample to each instrument based on name matching
    for (int i = 0; i < g_instrumentCount; i++) {
        // Initialize instrument data
        g_instruments[i].sampleCount = 1;
        g_instruments[i].sampleIndex = i % g_sampleCount;
        
        // Set up a simple key range (all keys use the same sample)
        g_instruments[i].keyRangeStart[0] = 0;
        g_instruments[i].keyRangeEnd[0] = 127;
        g_instruments[i].sampleIndex2[0] = g_instruments[i].sampleIndex;
        g_instruments[i].velRangeStart[0] = 0;
        g_instruments[i].velRangeEnd[0] = 127;
    }
    
    // Print out some examples of the mapping
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

// Load SF2 file and extract essential data
int loadSF2(const char *filename) {
    FILE *file;
    long pdtaStart, pdtaEnd;
    
    // Open SoundFont file
    file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return 0;
    }
    
    printf("Loading SoundFont file: %s\n", filename);
    
    // Find sample data
    if (!findSampleData(file)) {
        fclose(file);
        return 0;
    }
    
    // Find pdta LIST
    if (!findListChunk(file, "pdta", &pdtaStart, &pdtaEnd)) {
        printf("Error: Cannot find preset data\n");
        fclose(file);
        return 0;
    }
    
    printf("Found preset data from offset %ld to %ld\n", pdtaStart, pdtaEnd);
    
    // Read instruments
    if (!readInstruments(file, pdtaStart, pdtaEnd)) {
        fclose(file);
        return 0;
    }
    
    // Read presets
    if (!readPresets(file, pdtaStart, pdtaEnd)) {
        fclose(file);
        return 0;
    }
    
    // Read samples
    if (!readSamples(file, pdtaStart, pdtaEnd)) {
        fclose(file);
        return 0;
    }
    
    // Create the MIDI program to sample mapping
    createMidiToSampleMapping();
    
    fclose(file);
    return 1;
}

// Example Sound Blaster sample playback functions
// These would interface with your existing MIDI player

// Setup Sound Blaster DSP
void SB_Init(int basePort, int irq, int dma) {
    printf("Initializing Sound Blaster: Port 0x%03X, IRQ %d, DMA %d\n", 
           basePort, irq, dma);
    // Your Sound Blaster init code would go here
}

// Play a sample on Sound Blaster
void SB_PlaySample(FILE *sf2File, SF2Sample *sample, int note, int velocity) {
    // Calculate pitch adjustment based on note vs. original pitch
    int pitchDiff = note - sample->originalPitch;
    
    printf("Playing sample %s for note %d with velocity %d\n", 
           sample->name, note, velocity);
    
    // Your Sound Blaster DSP code would go here:
    // 1. Seek to sample start in SF2 file
    // 2. Configure DMA for audio transfer
    // 3. Set Sound Blaster playback parameters
    // 4. Start playback
}

// Get a sample for a given program and note
SF2Sample* getSample(int bank, int program, int note, int velocity) {
    // Find the matching preset
    for (int i = 0; i < g_presetCount; i++) {
        if (g_presets[i].bank == bank && g_presets[i].preset == program) {
            // Get the instrument
            int instrumentIdx = g_presets[i].instrumentIndex;
            
            // Find the matching key range
            for (int j = 0; j < g_instruments[instrumentIdx].sampleCount; j++) {
                if (note >= g_instruments[instrumentIdx].keyRangeStart[j] &&
                    note <= g_instruments[instrumentIdx].keyRangeEnd[j] &&
                    velocity >= g_instruments[instrumentIdx].velRangeStart[j] &&
                    velocity <= g_instruments[instrumentIdx].velRangeEnd[j]) {
                    
                    // Return the sample
                    int sampleIdx = g_instruments[instrumentIdx].sampleIndex2[j];
                    return &g_samples[sampleIdx];
                }
            }
            
            // If no specific range found, use the first sample
            return &g_samples[g_instruments[instrumentIdx].sampleIndex];
        }
    }
    
    // Default to first sample if no match found
    return &g_samples[0];
}

// Example integration with MIDI player - MIDI event handler
void handleMidiEvent(int command, int channel, int data1, int data2) {
    static int channelProgram[16] = {0};
    static int channelBank[16] = {0};
    
    // Process MIDI event
    switch (command) {
        case 0x90: // Note On
            if (data2 > 0) {
                // Get the sample for this note
                SF2Sample *sample = getSample(channelBank[channel], 
                                              channelProgram[channel], 
                                              data1, data2);
                
                // Play the sample (this would need your SF2 file handle)
                // SB_PlaySample(sf2File, sample, data1, data2);
                printf("Note On: Ch %d, Note %d, Vel %d -> Sample %s\n",
                       channel, data1, data2, sample->name);
            } else {
                // Note Off (Note On with velocity 0)
                printf("Note Off: Ch %d, Note %d\n", channel, data1);
            }
            break;
            
        case 0x80: // Note Off
            printf("Note Off: Ch %d, Note %d\n", channel, data1);
            break;
            
        case 0xC0: // Program Change
            channelProgram[channel] = data1;
            printf("Program Change: Ch %d, Program %d\n", channel, data1);
            break;
            
        case 0xB0: // Control Change
            if (data1 == 0) { // Bank Select MSB
                channelBank[channel] = (channelBank[channel] & 0x7F) | (data2 << 7);
                printf("Bank Select MSB: Ch %d, Bank %d\n", channel, channelBank[channel]);
            } else if (data1 == 32) { // Bank Select LSB
                channelBank[channel] = (channelBank[channel] & 0x3F80) | data2;
                printf("Bank Select LSB: Ch %d, Bank %d\n", channel, channelBank[channel]);
            }
            break;
    }
}

// Main function - demonstration
int main(int argc, char *argv[]) {
    // Check arguments
    if (argc < 2) {
        printf("Usage: %s <soundfont.sf2>\n", argv[0]);
        return 1;
    }
    
    // Load the soundfont
    if (!loadSF2(argv[1])) {
        printf("Failed to load SF2 file: %s\n", argv[1]);
        return 1;
    }
    
    printf("\nSF2 file loaded successfully!\n");
    
    // Example of how the MIDI events would be handled
    printf("\nExample MIDI event handling:\n");
    handleMidiEvent(0xC0, 0, 0, 0);  // Program Change to Piano on channel 0
    handleMidiEvent(0x90, 0, 60, 64); // Note On C4 with velocity 64 on channel 0
    handleMidiEvent(0x80, 0, 60, 0);  // Note Off C4 on channel 0
    
    return 0;
}
