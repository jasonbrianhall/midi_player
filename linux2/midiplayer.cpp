// midiplayer.cpp
// MIDI Player implementation with OPL3 synthesis
// C++ version for Linux using SDL

#include "midiplayer.h"
#include "audio.h"
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <string.h>

MidiPlayer::MidiPlayer() 
    : midiFile(nullptr)
    , playing(false)
    , paused(false)
    , began(false)
    , playWait(0.0)
    , globalVolume(100)
    , useNormalization(true)
    , trackCount(0)
    , timeDivision(0)
    , invTimeDivision(0.0)
    , tempo(0.0)
    , bendSensitivity(2.0 / 8192.0)
    , loopWait(0.0)
    , foundLoopStart(false)
    , foundLoopEnd(false)
{
    // Initialize tracks
    for (auto& track : tracks) {
        track.filePosition = 0;
        track.delay = 0.0;
        track.runningStatus = 0;
        track.active = false;
    }
    
    // Initialize MIDI channels
    for (auto& channel : midiChannels) {
        channel.instrument = 0;
        channel.bendValue = 0.0;
        channel.volume = 127;
        channel.pan = 0;
        channel.vibrato = 0;
        channel.activeNotes = 0;
        
        for (int i = 0; i < MAX_NOTES; i++) {
            channel.tones[i] = 0;
            channel.channelMap[i] = 0;
            channel.velocities[i] = 0;
            channel.activeIndex[i] = 0;
        }
        
        for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
            channel.noteList[i] = 0;
        }
    }
    
    // Initialize OPL channels
    for (auto& channel : oplChannels) {
        channel.instrument = 0;
        channel.pan = 0;
        channel.pitch = 0;
        channel.active = false;
        channel.age = 0.0;
        channel.midiChannel = 0;
        channel.activeIndex = 0;
        channel.xPos = 0;
        channel.color = 0;
    }
}

MidiPlayer::~MidiPlayer() {
    if (midiFile) {
        fclose(midiFile);
        midiFile = nullptr;
    }
}

bool MidiPlayer::init() {
    // Initialize FM instruments
    initFMInstruments();
    
    // Reset OPL emulator
    oplReset();
    
    return true;
}

bool MidiPlayer::loadFile(const std::string& filename) {
    char buffer[256];
    char id[5] = {0};
    uint32_t headerLength;
    int format;
    
    this->filename = filename;
    
    // Open the MIDI file
    midiFile = fopen(filename.c_str(), "rb");
    if (!midiFile) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    // Read MIDI header
    if (readString(midiFile, 4, id) != 4 || strncmp(id, "MThd", 4) != 0) {
        std::cerr << "Error: Not a valid MIDI file" << std::endl;
        fclose(midiFile);
        midiFile = nullptr;
        return false;
    }
    
    // Read header length
    readString(midiFile, 4, buffer);
    headerLength = convertInteger(buffer, 4);
    if (headerLength != 6) {
        std::cerr << "Error: Invalid MIDI header length" << std::endl;
        fclose(midiFile);
        midiFile = nullptr;
        return false;
    }
    
    // Read format type
    readString(midiFile, 2, buffer);
    format = static_cast<int>(convertInteger(buffer, 2));
    
    // Read number of tracks
    readString(midiFile, 2, buffer);
    trackCount = static_cast<int>(convertInteger(buffer, 2));
    if (trackCount > MAX_TRACKS) {
        std::cerr << "Error: Too many tracks in MIDI file" << std::endl;
        fclose(midiFile);
        midiFile = nullptr;
        return false;
    }
    
    // Read time division
    readString(midiFile, 2, buffer);
    timeDivision = static_cast<int>(convertInteger(buffer, 2));
    
    // Calculate timing constants
    invTimeDivision = 1.0 / (double)timeDivision;
    tempo = 500000.0 * invTimeDivision; // Default tempo: 120 BPM (500,000 microseconds per beat)
    
    // Initialize track data
    for (int tk = 0; tk < trackCount; tk++) {
        // Read track header
        if (readString(midiFile, 4, id) != 4 || strncmp(id, "MTrk", 4) != 0) {
            std::cerr << "Error: Invalid track header" << std::endl;
            fclose(midiFile);
            midiFile = nullptr;
            return false;
        }
        
        // Read track length (we don't actually need this value)
        readString(midiFile, 4, buffer);
        uint32_t trackLength = convertInteger(buffer, 4);
        long pos = ftell(midiFile);
        
        // Read first event delay
        tracks[tk].delay = readVarLen(midiFile);
        tracks[tk].filePosition = ftell(midiFile);
        tracks[tk].runningStatus = 0;
        tracks[tk].active = true;
        
        // Skip to next track
        fseek(midiFile, pos + static_cast<long>(trackLength), SEEK_SET);
    }
    
    std::cout << "MIDI file loaded: " << filename << std::endl;
    std::cout << "Format: " << format << ", Tracks: " << trackCount 
              << ", Time Division: " << timeDivision << std::endl;
    
    return true;
}

void MidiPlayer::play() {
    if (!midiFile) {
        std::cerr << "Error: No MIDI file loaded" << std::endl;
        return;
    }
    
    // Reset playback state
    playing = true;
    paused = false;
    began = false;
    playWait = 0.0;
    foundLoopStart = true; // Force a loop point at the start
    foundLoopEnd = false;
    
    // Reset MIDI state
    for (auto& channel : midiChannels) {
        channel.instrument = 0;
        channel.bendValue = 0.0;
        channel.volume = 127;
        channel.pan = 0;
        channel.vibrato = 0;
        channel.activeNotes = 0;
        
        for (int i = 0; i < MAX_NOTES; i++) {
            channel.activeIndex[i] = 0;
        }
    }
    
    // Reset OPL channels
    for (auto& channel : oplChannels) {
        channel.active = false;
        channel.age = 0.0;
    }
    
    // Silence all channels
    oplSilence();
    
    std::cout << "Starting playback..." << std::endl;
}

void MidiPlayer::stop() {
    playing = false;
    oplSilence();
    
    if (midiFile) {
        // Rewind file to beginning for next play
        fseek(midiFile, 0, SEEK_SET);
        
        // Re-parse the file header and track info
        if (!loadFile(filename)) {
            std::cerr << "Failed to reload MIDI file" << std::endl;
        }
    }
}

void MidiPlayer::togglePause() {
    paused = !paused;
    
    if (paused) {
        std::cout << "Playback paused" << std::endl;
    } else {
        std::cout << "Playback resumed" << std::endl;
    }
}

void MidiPlayer::update() {
    if (!playing || paused) {
        return;
    }
    
    try {
        // Update the OPL emulator to generate more audio
        Audio::updateOPL();
        
        // Count down playback wait time
        if (began) {
            playWait -= 1.0;
        }
        
        // Process events until we have enough delay
        while (playWait < 16.0 && playing) {
            processEvents();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in update: " << e.what() << std::endl;
        playing = false; // Stop if we encounter an error
    }
    catch (...) {
        std::cerr << "Unknown exception in update" << std::endl;
        playing = false;
    }
}

bool MidiPlayer::isPlaying() const {
    return playing;
}

bool MidiPlayer::isPaused() const {
    return paused;
}

void MidiPlayer::increaseVolume() {
    if (globalVolume < 300) {
        globalVolume += 10;
    }
    
    // Update volumes for all active notes
    for (int m = 0; m < MAX_CHANNELS; m++) {
        for (int a = 0; a < midiChannels[m].activeNotes; a++) {
            int note = midiChannels[m].noteList[a];
            int c = midiChannels[m].channelMap[note];
            oplSetVolumeScaled(c, midiChannels[m].velocities[note] * midiChannels[m].volume);
        }
    }
    
    std::cout << "Volume: " << globalVolume << "%" << std::endl;
}

void MidiPlayer::decreaseVolume() {
    if (globalVolume > 10) {
        globalVolume -= 10;
    }
    
    // Update volumes for all active notes
    for (int m = 0; m < MAX_CHANNELS; m++) {
        for (int a = 0; a < midiChannels[m].activeNotes; a++) {
            int note = midiChannels[m].noteList[a];
            int c = midiChannels[m].channelMap[note];
            oplSetVolumeScaled(c, midiChannels[m].velocities[note] * midiChannels[m].volume);
        }
    }
    
    std::cout << "Volume: " << globalVolume << "%" << std::endl;
}

void MidiPlayer::toggleNormalization() {
    useNormalization = !useNormalization;
    
    // Update volumes for all active notes
    for (int m = 0; m < MAX_CHANNELS; m++) {
        for (int a = 0; a < midiChannels[m].activeNotes; a++) {
            int note = midiChannels[m].noteList[a];
            int c = midiChannels[m].channelMap[note];
            oplSetVolumeScaled(c, midiChannels[m].velocities[note] * midiChannels[m].volume);
        }
    }
    
    std::cout << "Normalization: " << (useNormalization ? "ON" : "OFF") << std::endl;
}

int MidiPlayer::getVolume() const {
    return globalVolume;
}

bool MidiPlayer::isNormalizationEnabled() const {
    return useNormalization;
}

void MidiPlayer::writeOPL(uint32_t reg, uint8_t val) {
    // Pass through to the audio system
    Audio::writeOPL(reg, val);
}

void MidiPlayer::oplSetupParams(int channel, int& port, int& regOffset, int& operatorOffset) {
    port = (channel / 9) * 2;
    regOffset = channel % 9;
    operatorOffset = (regOffset % 3) + 8 * (regOffset / 3);
}

void MidiPlayer::oplNoteOff(int channel) {
    int port, regOffset, operatorOffset;
    oplSetupParams(channel, port, regOffset, operatorOffset);
    
    // Turn off the note by clearing the key-on bit in register 0xB0+regOffset
    writeOPL(0xB0 + regOffset + (port * 0x100), oplChannels[channel].pitch & 0xDF);
}

void MidiPlayer::oplNoteOn(int channel, double hertz) {
    int port, regOffset, operatorOffset;
    oplSetupParams(channel, port, regOffset, operatorOffset);
    
    int fnum = 0x2000; // Initial F-Number with octave 0
    
    // Adjust frequency to correct octave
    while (hertz >= 1023.5) {
        hertz = hertz / 2.0;
        fnum += 0x400; // Increment octave
    }
    
    // Add the frequency value
    fnum += static_cast<int>(hertz);
    
    // Write low 8 bits to register 0xA0+regOffset
    writeOPL(0xA0 + regOffset + (port * 0x100), fnum & 0xFF);
    
    // Write high bits to register 0xB0+regOffset with key-on bit set
    fnum = fnum >> 8;
    writeOPL(0xB0 + regOffset + (port * 0x100), fnum | 0x20);
    
    // Store the pitch value for later
    oplChannels[channel].pitch = fnum;
}

void MidiPlayer::oplSetVolume(int channel, int velocity) {
    // The formula below: SOLVE(V=127*127 * 2^( (A-63) / 8), A)
    int scaledVolume = 0;
    
    if (velocity < 72) {
        scaledVolume = 0;
    } else {
        scaledVolume = static_cast<int>(log(velocity) * 11.541561 - 48.818955);
    }
    
    oplSetVolumeScaled(channel, scaledVolume);
}

void MidiPlayer::oplSetVolumeScaled(int channel, int scaledVolume) {
    int port, regOffset, operatorOffset;
    oplSetupParams(channel, port, regOffset, operatorOffset);
    
    int instrumentIndex = oplChannels[channel].instrument;
    
    // Apply global volume adjustment if normalization is enabled
    if (useNormalization) {
        // Scale the volume value by the global volume
        scaledVolume = (scaledVolume * globalVolume) / 100;
        
        // Make sure volume never goes below a minimum threshold
        // This helps with quieter instruments
        if (scaledVolume > 0 && scaledVolume < 20) {
            scaledVolume = 20;
        }
    }
    
    // Set volume for operator 1 (modulator)
    int vol1 = (instruments[instrumentIndex].modChar2 | 63) - scaledVolume + 
               ((instruments[instrumentIndex].modChar2 & 63) * scaledVolume) / 63;
    writeOPL(0x40 + operatorOffset + (port * 0x100), vol1);
    
    // Set volume for operator 2 (carrier)
    int vol2 = (instruments[instrumentIndex].carChar2 | 63) - scaledVolume +
               ((instruments[instrumentIndex].carChar2 & 63) * scaledVolume) / 63;
    writeOPL(0x43 + operatorOffset + (port * 0x100), vol2);
}

int MidiPlayer::readString(FILE* file, int length, char* buffer) {
    return static_cast<int>(fread(buffer, 1, length, file));
}

uint32_t MidiPlayer::readVarLen(FILE* file) {
    uint8_t c;
    uint32_t value = 0;
    
    if (fread(&c, 1, 1, file) != 1) return 0;
    
    value = c;
    if (c & 0x80) {
        value &= 0x7F;
        do {
            if (fread(&c, 1, 1, file) != 1) return value;
            value = (value << 7) + (c & 0x7F);
        } while (c & 0x80);
    }
    
    return value;
}

void MidiPlayer::oplSilence() {
    // Turn off all notes
    for (int c = 0; c < OPL_CHANNELS; c++) {
        oplNoteOff(c);
        oplSetVolumeScaled(c, 0);
        oplChannels[c].active = false;
    }
}

void MidiPlayer::deallocateActiveNote(int midiChannel, int activeIndex) {
    auto& channel = midiChannels[midiChannel];
    
    if (activeIndex < 1 || activeIndex > channel.activeNotes) {
        return; // Invalid index
    }
    
    // Get the note and OPL channel
    int note = channel.noteList[activeIndex - 1];
    int oplChannel = channel.channelMap[note];
    
    // Reset OPL channel
    oplChannels[oplChannel].active = false;
    oplChannels[oplChannel].age = 0;
    oplNoteOff(oplChannel);
    
    // Remove note from active list
    channel.activeIndex[note] = 0;
    
    // If it's not the last note in the list, move the last note to this slot
    if (activeIndex < channel.activeNotes) {
        int lastNote = channel.noteList[channel.activeNotes - 1];
        channel.noteList[activeIndex - 1] = lastNote;
        channel.activeIndex[lastNote] = activeIndex;
        oplChannels[channel.channelMap[lastNote]].activeIndex = activeIndex;
    }
    
    // Decrement active note count
    channel.activeNotes--;
}

void MidiPlayer::oplSetInstrument(int channel) {
    int port, regOffset, operatorOffset;
    oplSetupParams(channel, port, regOffset, operatorOffset);
    
    // Set up the instrument parameters
    int i = oplChannels[channel].instrument;
    
    // Operator 1 parameters
    writeOPL(0x20 + operatorOffset + (port * 0x100), instruments[i].modChar1);
    writeOPL(0x60 + operatorOffset + (port * 0x100), instruments[i].modChar3);
    writeOPL(0x80 + operatorOffset + (port * 0x100), instruments[i].modChar4);
    writeOPL(0xE0 + operatorOffset + (port * 0x100), instruments[i].modChar5);
    
    // Operator 2 parameters
    writeOPL(0x23 + operatorOffset + (port * 0x100), instruments[i].carChar1);
    writeOPL(0x63 + operatorOffset + (port * 0x100), instruments[i].carChar3);
    writeOPL(0x83 + operatorOffset + (port * 0x100), instruments[i].carChar4);
    writeOPL(0xE3 + operatorOffset + (port * 0x100), instruments[i].carChar5);
}

void MidiPlayer::oplSetPanning(int channel) {
    int port, regOffset, operatorOffset;
    oplSetupParams(channel, port, regOffset, operatorOffset);
    
    int i = oplChannels[channel].instrument;
    int pan = oplChannels[channel].pan;
    
    // Set the feedback/connection register with panning
    writeOPL(0xC0 + regOffset + (port * 0x100), instruments[i].fbConn - pan);
}

void MidiPlayer::processEvents() {
    // Check for null file pointer
    if (!midiFile) {
        playing = false;
        return;
    }
    
    // Save rollback info for each track
    for (int tk = 0; tk < trackCount; tk++) {
        rollbackPoints[tk] = tracks[tk];
        
        // Handle events for tracks that are due
        if (tracks[tk].active && tracks[tk].delay <= 0) {
            try {
                handleMidiEvent(tk);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in handleMidiEvent: " << e.what() << std::endl;
                playing = false;
                return;
            }
            catch (...) {
                std::cerr << "Unknown exception in handleMidiEvent" << std::endl;
                playing = false;
                return;
            }
        }
    }
    
    // Handle loop points
    if (foundLoopStart) {
        // Save loop beginning point
        for (int tk = 0; tk < trackCount; tk++) {
            loopPoints[tk] = rollbackPoints[tk];
        }
        loopWait = playWait;
        foundLoopStart = false;
    } else if (foundLoopEnd) {
        // Return to loop beginning
        for (int tk = 0; tk < trackCount; tk++) {
            tracks[tk] = loopPoints[tk];
        }
        foundLoopEnd = false;
        playWait = loopWait;
    }
    
    // Find the shortest delay from all active tracks
    double nextDelay = -1;
    for (int tk = 0; tk < trackCount; tk++) {
        if (!tracks[tk].active) continue;
        if (nextDelay < 0 || tracks[tk].delay < nextDelay) {
            nextDelay = tracks[tk].delay;
        }
    }
    
    // Check if all tracks have ended
    if (nextDelay < 0) {
        // All tracks have ended, stop playback
        playing = false;
        return;
    }
    
    // Update all track delays
    for (int tk = 0; tk < trackCount; tk++) {
        tracks[tk].delay -= nextDelay;
    }
    
    // Schedule next event
    double t = nextDelay * tempo;
    if (began) playWait += t;
    
    // Age all channels
    for (int a = 0; a < OPL_CHANNELS; a++) {
        oplChannels[a].age += t;
    }
}

void MidiPlayer::handleMidiEvent(int trackIndex) {
    uint8_t status, data1, data2;
    uint8_t evtype;
    int len;
    char buffer[256];
    
    // Get file position
    fseek(midiFile, tracks[trackIndex].filePosition, SEEK_SET);
    
    // Read status byte or use running status
    if (fread(&status, 1, 1, midiFile) != 1) return;
    
    // Check for running status
    if (status < 0x80) {
        fseek(midiFile, tracks[trackIndex].filePosition, SEEK_SET); // Go back one byte
        status = tracks[trackIndex].runningStatus; // Use running status
    } else {
        tracks[trackIndex].runningStatus = status; // Save status for next event
    }
    
    int midiCh = status & 0x0F;
    
    // Handle different event types
    switch (status & 0xF0) {
        case NOTE_OFF: {
            // Note Off event
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            if (fread(&data2, 1, 1, midiFile) != 1) break;
            
            // Reset pitch bend
            midiChannels[midiCh].bendValue = 0;
            
            // Find the active note
            int activeIndex = midiChannels[midiCh].activeIndex[data1];
            if (activeIndex == 0) break; // Note not active
            
            // Release the note
            deallocateActiveNote(midiCh, activeIndex);
            break;
        }
        
        case NOTE_ON: {
            // Note On event
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            if (fread(&data2, 1, 1, midiFile) != 1) break;
            
            // Note on with velocity 0 is treated as note off
            if (data2 == 0) {
                // Reset pitch bend
                midiChannels[midiCh].bendValue = 0;
                
                // Find the active note
                int activeIndex = midiChannels[midiCh].activeIndex[data1];
                if (activeIndex == 0) break; // Note not active
                
                // Release the note
                deallocateActiveNote(midiCh, activeIndex);
                break;
            }
            
            // Ignore repeat notes without note off
            if (midiChannels[midiCh].activeIndex[data1]) break;
            
            // Determine instrument and note
            int note = data1;
            int tone = note;
            int instrumentIndex = midiChannels[midiCh].instrument;
            
            // MIDI channel 9 always plays percussion
            if (midiCh == 9) {
                instrumentIndex = 128 + note - 35;
                if (instrumentIndex < 128 || instrumentIndex >= 181) {
                    break; // Invalid percussion instrument
                }
                tone = instruments[instrumentIndex].percNote;
            }
            
            // Allocate an OPL channel
            double bestScore = -9;
            int allocatedChannel = -1;
            
            for (int a = 0; a < OPL_CHANNELS; a++) {
                double score = oplChannels[a].age;
                if (!oplChannels[a].active) score += 3e3;             // Empty channel = privileged
                if (oplChannels[a].instrument == instrumentIndex) score += 0.2;  // Same instrument = good
                if (instrumentIndex < 128 && oplChannels[a].instrument >= 128) score = score*2+9; // Percussion is inferior
                if (score > bestScore) { bestScore = score; allocatedChannel = a; }
            }
            
            // If no channel found, give up
            if (allocatedChannel < 0) break;
            
            // Handle collision with existing note
            if (oplChannels[allocatedChannel].active) {
                int midiChannel = oplChannels[allocatedChannel].midiChannel;
                int noteIndex = oplChannels[allocatedChannel].activeIndex;
                deallocateActiveNote(midiChannel, noteIndex);
            }
            
            // Set up new note in OPL channel
            oplChannels[allocatedChannel].active = true;
            oplChannels[allocatedChannel].instrument = instrumentIndex;
            oplChannels[allocatedChannel].age = 0;
            began = true;
            
            // Allocate active note for MIDI channel
            int activeIndex = ++midiChannels[midiCh].activeNotes;
            midiChannels[midiCh].noteList[activeIndex - 1] = note;
            midiChannels[midiCh].activeIndex[note] = activeIndex;
            
            // Record info about this note
            midiChannels[midiCh].tones[note] = tone;
            midiChannels[midiCh].channelMap[note] = allocatedChannel;
            midiChannels[midiCh].velocities[note] = data2;
            oplChannels[allocatedChannel].midiChannel = midiCh;
            oplChannels[allocatedChannel].activeIndex = activeIndex;
            
            // Set up display info (for debug/visual purposes)
            oplChannels[allocatedChannel].xPos = 1 + (tone + 63) % 80;
            oplChannels[allocatedChannel].color = 9 + (oplChannels[allocatedChannel].instrument % 6);
            
            // Set up OPL channel
            oplSetInstrument(allocatedChannel);
            oplSetPanning(allocatedChannel);
            oplSetVolume(allocatedChannel, data2 * midiChannels[midiCh].volume / 127);
            
            // Calculate and play the note
            double hertz = 172.00093 * exp(0.057762265 * (tone + midiChannels[midiCh].bendValue));
            oplNoteOn(allocatedChannel, hertz);
            
            break;
        }
        
        case POLY_PRESSURE: {
            // Polyphonic Key Pressure
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            if (fread(&data2, 1, 1, midiFile) != 1) break;
            
            // Find the active note
            int activeIndex = midiChannels[midiCh].activeIndex[data1];
            if (activeIndex == 0) break; // Note not active
            
            // Update the note velocity
            int c = midiChannels[midiCh].channelMap[data1];
            midiChannels[midiCh].velocities[data1] = data2;
            oplSetVolume(c, data2 * midiChannels[midiCh].volume / 127);
            
            break;
        }
        
        case CONTROL_CHANGE: {
            // Control Change
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            if (fread(&data2, 1, 1, midiFile) != 1) break;
            
            switch (data1) {
                case 1:  // Modulation wheel
                    midiChannels[midiCh].vibrato = data2;
                    break;
                    
                case 6:  // Data Entry MSB
                    bendSensitivity = data2 / 8192.0;
                    break;
                    
                case 7:  // Channel Volume
                    midiChannels[midiCh].volume = data2;
                    
                    // Update all notes on this channel
                    for (int a = 0; a < midiChannels[midiCh].activeNotes; a++) {
                        int note = midiChannels[midiCh].noteList[a];
                        int c = midiChannels[midiCh].channelMap[note];
                        oplSetVolume(c, midiChannels[midiCh].velocities[note] * data2 / 127);
                    }
                    break;
                    
                case 10: // Pan
                    if (data2 < 48)
                        midiChannels[midiCh].pan = 32;
                    else if (data2 > 79)
                        midiChannels[midiCh].pan = 16;
                    else
                        midiChannels[midiCh].pan = 0;
                    
                    // Update all notes on this channel
                    for (int a = 0; a < midiChannels[midiCh].activeNotes; a++) {
                        int note = midiChannels[midiCh].noteList[a];
                        int c = midiChannels[midiCh].channelMap[note];
                        oplChannels[c].pan = midiChannels[midiCh].pan;
                        oplSetPanning(c);
                    }
                    break;
                    
                case 121: // Reset All Controllers
                    midiChannels[midiCh].bendValue = 0;
                    midiChannels[midiCh].vibrato = 0;
                    midiChannels[midiCh].pan = 0;
                    
                    // Update all notes
                    for (int a = 0; a < midiChannels[midiCh].activeNotes; a++) {
                        int note = midiChannels[midiCh].noteList[a];
                        int c = midiChannels[midiCh].channelMap[note];
                        int tone = midiChannels[midiCh].tones[note];
                        
                        // Update pitch
                        double hertz = 172.00093 * exp(0.057762265 * (tone + midiChannels[midiCh].bendValue));
                        oplNoteOn(c, hertz);
                        
                        // Update volume
                        oplSetVolume(c, midiChannels[midiCh].velocities[note] * midiChannels[midiCh].volume / 127);
                        
                        // Update panning
                        oplChannels[c].pan = midiChannels[midiCh].pan;
                        oplSetPanning(c);
                    }
                    break;
                    
                case 123: // All Notes Off
                    // Turn off all notes on this channel
                    for (int a = midiChannels[midiCh].activeNotes; a > 0; a--) {
                        deallocateActiveNote(midiCh, a);
                    }
                    break;
            }
            break;
        }
        
        case PROGRAM_CHANGE: {
            // Program Change
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            midiChannels[midiCh].instrument = data1;
            break;
        }
        
        case CHAN_PRESSURE: {
            // Channel Pressure
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            
            // Update all note volumes
            for (int a = 0; a < midiChannels[midiCh].activeNotes; a++) {
                int note = midiChannels[midiCh].noteList[a];
                int c = midiChannels[midiCh].channelMap[note];
                midiChannels[midiCh].velocities[note] = data1;
                oplSetVolume(c, data1 * midiChannels[midiCh].volume / 127);
            }
            break;
        }
        
        case PITCH_BEND: {
            // Pitch Bend
            if (fread(&data1, 1, 1, midiFile) != 1) break;
            if (fread(&data2, 1, 1, midiFile) != 1) break;
            
            midiChannels[midiCh].bendValue = (data1 + data2 * 128 - 8192) * bendSensitivity;
            
            // Update all note pitches
            for (int a = 0; a < midiChannels[midiCh].activeNotes; a++) {
                int note = midiChannels[midiCh].noteList[a];
                int c = midiChannels[midiCh].channelMap[note];
                int tone = midiChannels[midiCh].tones[note];
                double hertz = 172.00093 * exp(0.057762265 * (tone + midiChannels[midiCh].bendValue));
                oplNoteOn(c, hertz);
            }
            break;
        }
        
        case META_EVENT: case SYSTEM_MESSAGE: {
            // Meta events and system exclusive
            if (status == META_EVENT) {
                // Meta event
                if (fread(&evtype, 1, 1, midiFile) != 1) break;
                uint32_t len = readVarLen(midiFile);
                
                if (evtype == META_END_OF_TRACK) {
                    tracks[trackIndex].active = false;  // Mark track as ended
                    fseek(midiFile, len, SEEK_CUR);  // Skip event data
                } else if (evtype == META_TEMPO) {
                    // Tempo change
                    char buffer[4] = {0};
                    readString(midiFile, static_cast<int>(len), buffer);
                    uint32_t tempoVal = convertInteger(buffer, static_cast<int>(len));
                    tempo = tempoVal * invTimeDivision;
                } else if (evtype == META_TEXT) {
                    // Text event - check for loop markers
                    char text[256] = {0};
                    readString(midiFile, static_cast<int>(len), text);
                    
                    if (strcmp(text, "loopStart") == 0) {
                        foundLoopStart = true;
                    } else if (strcmp(text, "loopEnd") == 0) {
                        foundLoopEnd = true;
                    }
                    
                    // Display meta event text for debug
                    std::cout << "Meta " << static_cast<int>(evtype) << ": " << text << std::endl;
                } else {
                    // Skip other meta events
                    fseek(midiFile, len, SEEK_CUR);
                }
            } else {
                // System exclusive - skip
                uint32_t len = readVarLen(midiFile);
                fseek(midiFile, static_cast<long>(len), SEEK_CUR);
            }
            break;
        }
    }
    
    // Read next event delay
    uint32_t nextDelay = readVarLen(midiFile);
    tracks[trackIndex].delay += nextDelay;
    
    // Save new file position
    tracks[trackIndex].filePosition = ftell(midiFile);
}

uint32_t MidiPlayer::convertInteger(const char* buffer, int length) {
    if (!buffer || length <= 0) return 0;
    uint32_t value = 0;
    for (int i = 0; i < length; i++) {
        value = value * 256 + (uint8_t)buffer[i];
    }
    return value;
}

void MidiPlayer::oplReset() {
    // Reset the OPL3 chip
    
    // Enable waveform selection
    writeOPL(0x01, 0x20);
    
    // Set melodic mode, no percussion
    writeOPL(0xBD, 0x00);
    
    // Enable OPL3 features (must use port 2)
    writeOPL(0x105, 0x01);
    
    // Set OPL3 mode
    writeOPL(0x104, 0x00);
    
    // Silence all channels
    oplSilence();
}
