#include "sfmidi.h"

// Global data storage - removed duplicates since they're already in the header

// Forward declarations for missing functions
unsigned long readVarLen(FILE* f);
int readString(FILE* f, int len, char* str);
unsigned long convertInteger(char* str, int len);
void outOPL(int port, int reg, int val);

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
                
                // Initialize sample data pointer
                g_samples[i].sampleData = NULL;
                g_samples[i].sampleLength = g_samples[i].end - g_samples[i].start;
                
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

// Load actual sample data for a specific sample
int loadSampleData(FILE *file, int sampleIndex) {
    SF2Sample *sample = &g_samples[sampleIndex];
    
    // Skip if sample data already loaded
    if (sample->sampleData != NULL) {
        return 1;
    }
    
    // Calculate sample length
    int length = sample->end - sample->start;
    if (length <= 0) {
        printf("Warning: Invalid sample length for %s\n", sample->name);
        return 0;
    }
    
    // Allocate memory for sample data
    sample->sampleData = (short*)malloc(length * sizeof(short));
    if (sample->sampleData == NULL) {
        printf("Error: Out of memory for sample %s\n", sample->name);
        return 0;
    }
    
    // Seek to sample start
    fseek(file, g_sampleDataOffset + sample->start * sizeof(short), SEEK_SET);
    
    // Read sample data
    if (fread(sample->sampleData, sizeof(short), length, file) != length) {
        printf("Error: Failed to read sample data for %s\n", sample->name);
        free(sample->sampleData);
        sample->sampleData = NULL;
        return 0;
    }
    
    // Swap endianness (SoundFont uses big-endian for samples)
    for (int i = 0; i < length; i++) {
        short s = sample->sampleData[i];
        sample->sampleData[i] = ((s & 0xFF) << 8) | ((s & 0xFF00) >> 8);
    }
    
    return 1;
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
    long pdtaStart, pdtaEnd;
    
    // Open SoundFont file
    sf2File = fopen(filename, "rb");
    if (!sf2File) {
        printf("Error: Cannot open file %s\n", filename);
        return 0;
    }
    
    printf("Loading SoundFont file: %s\n", filename);
    
    // Find sample data
    if (!findSampleData(sf2File)) {
        fclose(sf2File);
        sf2File = NULL;
        return 0;
    }
    
    // Find pdta LIST
    if (!findListChunk(sf2File, "pdta", &pdtaStart, &pdtaEnd)) {
        printf("Error: Cannot find preset data\n");
        fclose(sf2File);
        sf2File = NULL;
        return 0;
    }
    
    printf("Found preset data from offset %ld to %ld\n", pdtaStart, pdtaEnd);
    
    // Read instruments
    if (!readInstruments(sf2File, pdtaStart, pdtaEnd)) {
        fclose(sf2File);
        sf2File = NULL;
        return 0;
    }
    
    // Read presets
    if (!readPresets(sf2File, pdtaStart, pdtaEnd)) {
        fclose(sf2File);
        sf2File = NULL;
        return 0;
    }
    
    // Read samples
    if (!readSamples(sf2File, pdtaStart, pdtaEnd)) {
        fclose(sf2File);
        sf2File = NULL;
        return 0;
    }
    
    // Create the MIDI program to sample mapping
    createMidiToSampleMapping();
    
    // Initialize active notes
    for (int i = 0; i < 32; i++) {
        g_activeNotes[i].isActive = 0;
    }
    
    // Pre-load a few common samples
    printf("Pre-loading common samples...\n");
    for (int i = 0; i < 10 && i < g_sampleCount; i++) {
        loadSampleData(sf2File, i);
    }
    
    return 1;
}

// Sound Blaster functions
// Write to DSP
void sbWriteDSP(int value) {
    int timeout;
    
    timeout = 0xFFFF;
    while ((inportb(sb_port + 0x0C) & 0x80) && --timeout)
        ;
    
    if (timeout) {
        outportb(sb_port + 0x0C, value);
    }
}

// Read from DSP
int sbReadDSP() {
    int timeout;
    
    timeout = 0xFFFF;
    while (!(inportb(sb_port + 0x0E) & 0x80) && --timeout)
        ;
    
    if (timeout) {
        return inportb(sb_port + 0x0A);
    }
    
    return -1;
}

// Reset DSP
int sbResetDSP() {
    outportb(sb_port + 0x06, 1);
    delay(5);
    outportb(sb_port + 0x06, 0);
    delay(5);
    
    int timeout = 100;
    while (--timeout) {
        if (sbReadDSP() == 0xAA) {
            return 1;
        }
        delay(1);
    }
    
    return 0;
}

// Get DSP version
int sbGetDSPVersion() {
    int major, minor;
    
    sbWriteDSP(SB_DSP_GET_VERSION);
    major = sbReadDSP();
    minor = sbReadDSP();
    
    return (major << 8) | minor;
}

// Setup DMA transfer for Sound Blaster 16
void sbSetupDMA(int dma, void *buffer, int length, int auto_init) {
    // DMA setup code would go here
    // This is a placeholder for the actual DMA initialization
    printf("Setting up DMA channel %d for buffer at %p, length %d\n", 
           dma, buffer, length);
}

// Start playback on Sound Blaster 16
void sbStartPlayback(int rate) {
    // Set sample rate
    sbWriteDSP(0x41);  // Set output sample rate
    sbWriteDSP((rate >> 8) & 0xFF);
    sbWriteDSP(rate & 0xFF);
    
    // Start 16-bit playback
    sbWriteDSP(SB_DSP_DMA_16BIT);  // 16-bit auto-init PCM
    sbWriteDSP(0x10);  // 16-bit unsigned stereo
    sbWriteDSP(((DMA_BUFFER_SIZE - 1) >> 1) & 0xFF);  // Low byte of length - 1
    sbWriteDSP(((DMA_BUFFER_SIZE - 1) >> 1) >> 8);    // High byte of length - 1
}

// Initialize Sound Blaster
int initSoundBlaster() {
    printf("Initializing Sound Blaster at port 0x%x, IRQ %d, DMA %d, HDMA %d\n", 
           sb_port, sb_irq, sb_dma, sb_hdma);
    
    // Reset DSP
    if (!sbResetDSP()) {
        printf("Error: Failed to reset Sound Blaster DSP\n");
        return 0;
    }
    
    // Get DSP version
    sb_version = sbGetDSPVersion();
    printf("Sound Blaster DSP version %d.%d\n", sb_version >> 8, sb_version & 0xFF);
    
    // Check if we have at least Sound Blaster 16
    if (sb_version < 0x0400) {
        printf("Error: Sound Blaster 16 or better required\n");
        return 0;
    }
    
    // Allocate DMA buffer (4K aligned for 16-bit DMA)
    // For a real implementation, this would need to be page-aligned
    dma_buffer = (short*)malloc(DMA_BUFFER_SIZE * sizeof(short));
    if (dma_buffer == NULL) {
        printf("Error: Failed to allocate DMA buffer\n");
        return 0;
    }
    
    // Clear buffer
    memset(dma_buffer, 0, DMA_BUFFER_SIZE * sizeof(short));
    
    // Setup DMA
    sbSetupDMA(sb_hdma, dma_buffer, DMA_BUFFER_SIZE * sizeof(short), 1);
    
    // Turn on speaker
    sbWriteDSP(SB_DSP_SPEAKER_ON);
    
    sb_initialized = 1;
    return 1;
}

// Mix active notes into the output buffer
void mixActiveNotes(short* buffer, int length) {
    int i, j;
    double sample;
    
    // Clear buffer
    memset(buffer, 0, length * sizeof(short));
    
    // Mix each active note
    for (i = 0; i < 32; i++) {
        if (!g_activeNotes[i].isActive) continue;
        
        SF2Sample* sampleData = g_activeNotes[i].sample;
        if (!sampleData || !sampleData->sampleData) continue;
        
        // Calculate volume (0.0 - 1.0)
        double volume = (g_activeNotes[i].velocity / 127.0) * 
                       (ChVolume[g_activeNotes[i].midiChannel] / 127.0) * 
                       (globalVolume / 100.0);
        
        // Mix samples into buffer
        for (j = 0; j < length; j++) {
            // Calculate sample position (with pitch adjustment)
            double pos = g_activeNotes[i].currentPos;
            int pos_int = (int)pos;
            double pos_frac = pos - pos_int;
            
            // Check if we've reached the end of the sample
            if (pos_int >= sampleData->sampleLength - 1) {
                if (sampleData->loopEnd > sampleData->loopStart) {
                    // Loop sample
                    pos_int = sampleData->loopStart + 
                             (pos_int - sampleData->loopStart) % 
                             (sampleData->loopEnd - sampleData->loopStart);
                } else {
                    // End of sample - deactivate note
                    g_activeNotes[i].isActive = 0;
                    break;
                }
            }
            
            // Linear interpolation between samples
            if (pos_int < sampleData->sampleLength - 1) {
                sample = sampleData->sampleData[pos_int] * (1.0 - pos_frac) + 
                         sampleData->sampleData[pos_int + 1] * pos_frac;
            } else {
                sample = sampleData->sampleData[pos_int];
            }
            
            // Apply volume and mix
            buffer[j] += (short)(sample * volume);
            
            // Advance position based on playback rate
            g_activeNotes[i].currentPos += g_activeNotes[i].playbackRate;
        }
    }
}

// Start a new note
int startNote(int channel, int note, int velocity) {
    int i;
    SF2Sample* sample = NULL;
    
    // Find the sample to use for this note/program
    if (enableSamplePlayback) {
        // Find the matching preset
        for (i = 0; i < g_presetCount; i++) {
            if (g_presets[i].bank == 0 && g_presets[i].preset == ChPatch[channel]) {
                // Get the instrument
                int instrumentIdx = g_presets[i].instrumentIndex;
                
                // Find the matching key range
                for (int j = 0; j < g_instruments[instrumentIdx].sampleCount; j++) {
                    if (note >= g_instruments[instrumentIdx].keyRangeStart[j] &&
                        note <= g_instruments[instrumentIdx].keyRangeEnd[j] &&
                        velocity >= g_instruments[instrumentIdx].velRangeStart[j] &&
                        velocity <= g_instruments[instrumentIdx].velRangeEnd[j]) {
                        
                        // Get the sample
                        int sampleIdx = g_instruments[instrumentIdx].sampleIndex2[j];
                        sample = &g_samples[sampleIdx];
                        
                        // Load sample data if not already loaded
                        if (!sample->sampleData) {
                            loadSampleData(sf2File, sampleIdx);
                        }
                        break;
                    }
                }
                
                // If no specific range found, use the first sample
                if (!sample) {
                    int sampleIdx = g_instruments[instrumentIdx].sampleIndex;
                    sample = &g_samples[sampleIdx];
                    
                    // Load sample data if not already loaded
                    if (!sample->sampleData) {
                        loadSampleData(sf2File, sampleIdx);
                    }
                }
                
                break;
            }
        }
        
        // If no matching preset found, use first sample
        if (!sample && g_sampleCount > 0) {
            sample = &g_samples[0];
            
            // Load sample data if not already loaded
            if (!sample->sampleData) {
                loadSampleData(sf2File, 0);
            }
        }
    }
    
    // Find a free slot for the note
    for (i = 0; i < 32; i++) {
        if (!g_activeNotes[i].isActive) {
            // Initialize note parameters
            g_activeNotes[i].isActive = 1;
            g_activeNotes[i].midiChannel = channel;
            g_activeNotes[i].note = note;
            g_activeNotes[i].velocity = velocity;
            g_activeNotes[i].sample = sample;
            g_activeNotes[i].currentPos = 0;
            
            // Calculate playback rate
            if (sample) {
                double noteFreq = 440.0 * pow(2.0, (note - 69) / 12.0);
                double sampleBaseFreq = 440.0 * pow(2.0, (sample->originalPitch - 69) / 12.0);
                g_activeNotes[i].playbackRate = noteFreq / sampleBaseFreq * 
                                               (sample->sampleRate / (double)SB_SAMPLE_RATE);
            } else {
                g_activeNotes[i].playbackRate = 1.0;
            }
            
            // Initialize Adlib channel for FM fallback if sample playback fails
            if (!sample || !sample->sampleData) {
                // Find a free Adlib channel (first 9 channels)
                for (int c = 0; c < 9; c++) {
                    if (g_activeNotes[i].adlibChannel == 0) {
                        g_activeNotes[i].adlibChannel = c + 1;
                        
                        // Set up FM synthesis
                        int fmInstrument = ChPatch[channel];
                        if (channel == 9) fmInstrument = 128 + note - 35; // Percussion
                        
                        // Load FM instrument settings from adl array
                        if (fmInstrument >= 0 && fmInstrument < 181) {
                            outOPL(OPL_PORT, 0x20 + c, g_fmInstruments[fmInstrument].modChar1);
                            outOPL(OPL_PORT, 0x23 + c, g_fmInstruments[fmInstrument].carChar1);
                            outOPL(OPL_PORT, 0x40 + c, g_fmInstruments[fmInstrument].modChar2);
                            outOPL(OPL_PORT, 0x43 + c, g_fmInstruments[fmInstrument].carChar2);
                            outOPL(OPL_PORT, 0x60 + c, g_fmInstruments[fmInstrument].modChar3);
                            outOPL(OPL_PORT, 0x63 + c, g_fmInstruments[fmInstrument].carChar3);
                            outOPL(OPL_PORT, 0x80 + c, g_fmInstruments[fmInstrument].modChar4);
                            outOPL(OPL_PORT, 0x83 + c, g_fmInstruments[fmInstrument].carChar4);
                            outOPL(OPL_PORT, 0xE0 + c, g_fmInstruments[fmInstrument].modChar5);
                            outOPL(OPL_PORT, 0xE3 + c, g_fmInstruments[fmInstrument].carChar5);
                            outOPL(OPL_PORT, 0xC0 + c, g_fmInstruments[fmInstrument].fbConn);
                            
                            // Set volume based on velocity
                            int vol = (velocity * ChVolume[channel]) / 127;
                            outOPL(OPL_PORT, 0x43 + c, (g_fmInstruments[fmInstrument].carChar2 | 0x3F) - (vol >> 1));
                            
                            // Calculate frequency
                            double freq = 440.0 * pow(2.0, (note - 69) / 12.0);
                            int block = 3;
                            while (freq >= 1023.5) {
                                freq /= 2.0;
                                block++;
                            }
                            
                            int fnum = (int)freq;
                            outOPL(OPL_PORT, 0xA0 + c, fnum & 0xFF);
                            outOPL(OPL_PORT, 0xB0 + c, ((block & 7) << 2) | ((fnum >> 8) & 3) | 0x20);
                        }
                        break;
                    }
                }
            }
            
            return i;  // Return active note index
        }
    }
    
    return -1;  // No free slots
}

// Stop a note
void stopNote(int channel, int note) {
    // Find the active note
    for (int i = 0; i < 32; i++) {
        if (g_activeNotes[i].isActive && 
            g_activeNotes[i].midiChannel == channel &&
            g_activeNotes[i].note == note) {
            
            // Stop the Adlib channel if using FM synthesis
            if (g_activeNotes[i].adlibChannel > 0) {
                int c = g_activeNotes[i].adlibChannel - 1;
                outOPL(OPL_PORT, 0xB0 + c, 0); // Note off
            }
            
            g_activeNotes[i].isActive = 0;
        }
    }
}

// Process MIDI events from file
void processMidiEvents() {
    unsigned char status, data1, data2;
    unsigned char buffer[256];
    unsigned char evtype;
    int len;
    
    // Process events for all tracks that are due
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] < 0 || tkDelay[tk] > 0) continue;
        
        // Get file position
        fseek(midiFile, tkPtr[tk], SEEK_SET);
        
        // Read status byte or use running status
        if (fread(&status, 1, 1, midiFile) != 1) {
            tkStatus[tk] = -1;
            continue;
        }
        
        // Check for running status
        if (status < 0x80) {
            fseek(midiFile, tkPtr[tk], SEEK_SET); // Go back one byte
            status = tkStatus[tk]; // Use running status
        } else {
            tkStatus[tk] = status; // Save new status
        }
        
        int midCh = status & 0x0F;
        
        // Handle different event types
        switch (status & 0xF0) {
            case NOTE_OFF: {
                // Note Off event
                fread(&data1, 1, 1, midiFile);
                fread(&data2, 1, 1, midiFile);
                
                stopNote(midCh, data1);
                break;
            }
            
            case NOTE_ON: {
                // Note On event
                fread(&data1, 1, 1, midiFile);
                fread(&data2, 1, 1, midiFile);
                
                // Note on with velocity 0 is treated as note off
                if (data2 == 0) {
                    stopNote(midCh, data1);
                } else {
                    startNote(midCh, data1, data2);
                }
                break;
            }
            
            case CONTROL_CHANGE: {
                // Control Change
                fread(&data1, 1, 1, midiFile);
                fread(&data2, 1, 1, midiFile);
                
                switch (data1) {
                    case 7:  // Channel Volume
                        ChVolume[midCh] = data2;
                        break;
                        
                    case 10: // Pan
                        ChPanning[midCh] = data2;
                        break;
                }
                break;
            }
            
            case PROGRAM_CHANGE: {
                // Program Change
                fread(&data1, 1, 1, midiFile);
                ChPatch[midCh] = data1;
                break;
            }
            
            case META_EVENT: case SYSTEM_MESSAGE: {
                // Meta events and system exclusive
                if (status == META_EVENT) {
                    // Meta event
                    fread(&evtype, 1, 1, midiFile);
                    unsigned long len = readVarLen(midiFile);
                    
                    if (evtype == META_END_OF_TRACK) {
                        tkStatus[tk] = -1;  // Mark track as ended
                        fseek(midiFile, len, SEEK_CUR);  // Skip event data
                    } else if (evtype == META_TEMPO) {
                        // Tempo change
                        char tempo[4] = {0};
                        readString(midiFile, (int)len, tempo);
                        unsigned long tempoVal = convertInteger(tempo, (int)len);
                        Tempo = tempoVal * InvDeltaTicks;
                    } else if (evtype == META_TEXT) {
                        // Text event - check for loop markers
                        char text[256] = {0};
                        readString(midiFile, (int)len, text);
                        
                        if (strcmp(text, "loopStart") == 0) {
                            loopStart = 1;
                        } else if (strcmp(text, "loopEnd") == 0) {
                            loopEnd = 1;
                        }
                        
                        // Display meta event text
                        printf("Meta: %s\n", text);
                    } else {
                        // Skip other meta events
                        fseek(midiFile, len, SEEK_CUR);
                    }
                } else {
                    // System exclusive - skip
                    unsigned long len = readVarLen(midiFile);
                    fseek(midiFile, (long)len, SEEK_CUR);
                }
                break;
            }
            
            default: {
                // Unsupported event type - skip
                fseek(midiFile, 2, SEEK_CUR);  // Skip two data bytes
                break;
            }
        }
        
        // Read next event delay
        unsigned long nextDelay = readVarLen(midiFile);
        tkDelay[tk] += nextDelay;
        
        // Save new file position
        tkPtr[tk] = ftell(midiFile);
    }
    
    // Find the shortest delay from all tracks
    double nextDelay = -1;
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] < 0) continue;
        if (nextDelay == -1 || tkDelay[tk] < nextDelay) {
            nextDelay = tkDelay[tk];
        }
    }
    
    // Update all track delays
    if (nextDelay > 0) {
        for (int tk = 0; tk < TrackCount; tk++) {
            tkDelay[tk] -= nextDelay;
        }
        
        // Schedule next event
        double t = nextDelay * Tempo;
        if (began) playwait += t;
    }
    
    // Check if all tracks ended
    int allEnded = 1;
    for (int tk = 0; tk < TrackCount; tk++) {
        if (tkStatus[tk] >= 0) {
            allEnded = 0;
            break;
        }
    }
    
    // Handle end of song or loop
    if (allEnded || loopEnd) {
        if (loopStart) {
            // Restart playback from loop start markers
            for (int tk = 0; tk < TrackCount; tk++) {
                if (loPtr[tk] > 0) {
                    tkPtr[tk] = loPtr[tk];
                    tkDelay[tk] = loDelay[tk];
                    tkStatus[tk] = loStatus[tk];
                }
            }
            loopEnd = 0;
        } else {
            // End playback
            isPlaying = 0;
        }
    }
}

// Sound Blaster interrupt handler
void sbInterruptHandler() {
    // Acknowledge interrupt
    inportb(sb_port + 0x0E);
    outportb(0xA0, 0x20);  // EOI to slave PIC
    outportb(0x20, 0x20);  // EOI to master PIC
    
    // Toggle current buffer
    current_buffer = 1 - current_buffer;
    
    // Fill new buffer with audio data
    mixActiveNotes(dma_buffer + current_buffer * (DMA_BUFFER_SIZE / 2), DMA_BUFFER_SIZE / 2);
}

// Play the MIDI file using SoundFont samples
void playMidiFile(const char* midiFilename, const char* sf2Filename) {
    char input;
    
    // Initialize OPL (for FM fallback)
    outOPL(OPL_PORT, 1, 0x20);     // Enable waveform selection
    outOPL(OPL_PORT, 0x8, 0x40);   // Turn on CSW mode
    outOPL(OPL_PORT, 0xBD, 0x00);  // Set melodic mode, no rhythm
    
    // Initialize sound blaster
    if (!initSoundBlaster()) {
        printf("Failed to initialize Sound Blaster. Using FM synthesis only.\n");
        enableSamplePlayback = 0;
    }
    
    // Load the FM instrument definitions
    initFMInstruments();
    
    // Load SoundFont file
    if (enableSamplePlayback) {
        if (!loadSF2(sf2Filename)) {
            printf("Failed to load SoundFont file. Using FM synthesis only.\n");
            enableSamplePlayback = 0;
        }
    }
    
    // Initialize variables
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 0;
        ChVibrato[i] = 0;
    }
    
    // Initialize active notes
    for (int i = 0; i < 32; i++) {
        g_activeNotes[i].isActive = 0;
        g_activeNotes[i].adlibChannel = 0;
    }
    
    // Load MIDI file
    midiFile = fopen(midiFilename, "rb");
    if (!midiFile) {
        printf("Error: Cannot open MIDI file %s\n", midiFilename);
        
        // Clean up
        if (sf2File) {
            fclose(sf2File);
            sf2File = NULL;
        }
        
        return;
    }
    
    // Read MIDI header
    char id[5] = {0};
    unsigned char buffer[256];
    
    if (readString(midiFile, 4, id) != 4 || strncmp(id, "MThd", 4) != 0) {
        printf("Error: Not a valid MIDI file\n");
        fclose(midiFile);
        
        if (sf2File) {
            fclose(sf2File);
            sf2File = NULL;
        }
        
        return;
    }
    
    // Read header length
    readString(midiFile, 4, buffer);
    unsigned long headerLength = convertInteger(buffer, 4);
    
    // Read format type
    readString(midiFile, 2, buffer);
    int format = (int)convertInteger(buffer, 2);
    
    // Read number of tracks
    readString(midiFile, 2, buffer);
    TrackCount = (int)convertInteger(buffer, 2);
    
    // Read time division
    readString(midiFile, 2, buffer);
    DeltaTicks = (int)convertInteger(buffer, 2);
    
    InvDeltaTicks = 1.0 / (double)DeltaTicks;
    Tempo = 500000 * InvDeltaTicks;  // Default tempo: 120 BPM
    bendsense = 2.0 / 8192.0;
    
    printf("MIDI file loaded: %s\n", midiFilename);
    printf("Format: %d, Tracks: %d, Time Division: %d\n", format, TrackCount, DeltaTicks);
    
    // Initialize track data
    for (int tk = 0; tk < TrackCount; tk++) {
        // Read track header
        if (readString(midiFile, 4, id) != 4 || strncmp(id, "MTrk", 4) != 0) {
            printf("Error: Invalid track header in track %d\n", tk);
            fclose(midiFile);
            
            if (sf2File) {
                fclose(sf2File);
                sf2File = NULL;
            }
            
            return;
        }
        
        // Read track length
        readString(midiFile, 4, buffer);
        unsigned long trackLength = convertInteger(buffer, 4);
        long pos = ftell(midiFile);
        
        // Save track position and read first event delay
        tkPtr[tk] = pos;
        tkDelay[tk] = readVarLen(midiFile);
        tkStatus[tk] = 0;
        
        // Skip to next track
        fseek(midiFile, pos + (long)trackLength, SEEK_SET);
    }
    
    // Initialize loop tracking
    for (int tk = 0; tk < TrackCount; tk++) {
        loPtr[tk] = tkPtr[tk];
        loDelay[tk] = tkDelay[tk];
        loStatus[tk] = tkStatus[tk];
    }
    
    // Start playback
    if (enableSamplePlayback && sb_initialized) {
        // Fill initial buffers
        mixActiveNotes(dma_buffer, DMA_BUFFER_SIZE / 2);
        mixActiveNotes(dma_buffer + DMA_BUFFER_SIZE / 2, DMA_BUFFER_SIZE / 2);
        
        // Start Sound Blaster playback
        sbStartPlayback(SB_SAMPLE_RATE);
    }
    
    // Main playback loop
    isPlaying = 1;
    paused = 0;
    loopStart = 0;
    loopEnd = 0;
    playwait = 0;
    began = 0;
    
    printf("\nPlaying MIDI file: %s\n", midiFilename);
    printf("Using SoundFont: %s\n", sf2Filename);
    printf("Controls:\n");
    printf("  Q     - Quit\n");
    printf("  Space - Pause/Resume\n");
    printf("  +/-   - Increase/Decrease Volume\n");
    printf("  F     - Toggle between Sample/FM playback\n");
    
    while (isPlaying) {
        // Check for keypresses
        if (kbhit()) {
            input = getch();
            if (input == ' ') {
                paused = !paused;
                printf("Playback %s\n", paused ? "paused" : "resumed");
            } else if (input == 'q' || input == 'Q' || input == 27) {
                isPlaying = 0;
            } else if (input == '+') {
                if (globalVolume < 200) globalVolume += 10;
                printf("Volume: %d%%\n", globalVolume);
            } else if (input == '-') {
                if (globalVolume > 10) globalVolume -= 10;
                printf("Volume: %d%%\n", globalVolume);
            } else if (input == 'f' || input == 'F') {
                enableSamplePlayback = !enableSamplePlayback;
                printf("Using %s playback\n", enableSamplePlayback ? "sample" : "FM");
            }
        }
        
        // Process MIDI events
        if (!paused) {
            processMidiEvents();
        }
        
        // Simple delay to prevent CPU hogging
        delay(10);
    }
    
    // Stop playback
    if (sb_initialized) {
        sbWriteDSP(SB_DSP_DMA_STOP);
        sbWriteDSP(SB_DSP_SPEAKER_OFF);
    }
    
    // Clean up
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }
    
    if (sf2File) {
        // Free any loaded samples
        for (int i = 0; i < g_sampleCount; i++) {
            if (g_samples[i].sampleData) {
                free(g_samples[i].sampleData);
                g_samples[i].sampleData = NULL;
            }
        }
        
        fclose(sf2File);
        sf2File = NULL;
    }
    
    printf("Playback finished.\n");
}

int main(int argc, char* argv[]) {
    // Simple command-line argument handling
    if (argc < 3) {
        printf("Usage: %s <midi_file> <soundfont_file>\n", argv[0]);
        return 1;
    }

    // Play the MIDI file using the specified SoundFont
    playMidiFile(argv[1], argv[2]);

    return 0;
}
