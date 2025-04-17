// midiplayer.h
// Header file for MIDI player with OPL3 synthesis
// C++ version for Linux using SDL

#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <memory>

// MIDI constants
constexpr int MAX_TRACKS = 100;
constexpr int MAX_CHANNELS = 16;
constexpr int MAX_NOTES = 128;
constexpr int MAX_ACTIVE_NOTES = 100;

// MIDI event types
constexpr uint8_t NOTE_OFF = 0x80;
constexpr uint8_t NOTE_ON = 0x90;
constexpr uint8_t POLY_PRESSURE = 0xA0;
constexpr uint8_t CONTROL_CHANGE = 0xB0;
constexpr uint8_t PROGRAM_CHANGE = 0xC0;
constexpr uint8_t CHAN_PRESSURE = 0xD0;
constexpr uint8_t PITCH_BEND = 0xE0;
constexpr uint8_t SYSTEM_MESSAGE = 0xF0;
constexpr uint8_t META_EVENT = 0xFF;

// MIDI meta event types
constexpr uint8_t META_END_OF_TRACK = 0x2F;
constexpr uint8_t META_TEMPO = 0x51;
constexpr uint8_t META_TEXT = 0x01;

// OPL3 constants
constexpr int OPL_CHANNELS = 18;  // 9 channels in OPL2, 18 in OPL3

// FM Instrument data structure
struct FMInstrument {
    uint8_t modChar1;
    uint8_t carChar1;
    uint8_t modChar2;
    uint8_t carChar2;
    uint8_t modChar3;
    uint8_t carChar3;
    uint8_t modChar4;
    uint8_t carChar4;
    uint8_t modChar5;
    uint8_t carChar5;
    uint8_t fbConn;
    uint8_t percNote;
};

// Track state
struct TrackState {
    long filePosition;       // Current position in the file
    double delay;            // Delay before next event
    int runningStatus;       // Running status byte
    bool active;             // Track is still active
};

// MIDI Channel state
struct MidiChannel {
    int instrument;                              // Current program/patch
    double bendValue;                            // Current pitch bend value
    int volume;                                  // Channel volume (0-127)
    int pan;                                     // Panning (0=center, -32=left, 32=right)
    int vibrato;                                 // Vibrato depth
    int activeNotes;                             // Count of active notes
    std::array<int, MAX_NOTES> tones;            // Actual note being played
    std::array<int, MAX_NOTES> channelMap;       // Map to OPL channel
    std::array<int, MAX_NOTES> velocities;       // Note velocities
    std::array<int, MAX_NOTES> activeIndex;      // Index in active notes list
    std::array<int, MAX_ACTIVE_NOTES> noteList;  // List of active notes
};

// OPL Channel state
struct OPLChannel {
    int instrument;          // Current instrument
    int pan;                 // Panning value
    int pitch;               // Current pitch value
    bool active;             // Channel is active
    double age;              // Time since channel was allocated
    int midiChannel;         // MIDI channel that owns this
    int activeIndex;         // Index in the MIDI channel's active notes
    int xPos;                // X position for display
    int color;               // Color for display
};

class MidiPlayer {
public:
    MidiPlayer();
    ~MidiPlayer();
    
    // Initialize the player
    bool init();
    
    // Load a MIDI file
    bool loadFile(const std::string& filename);
    
    // Play the loaded MIDI file
    void play();
    
    // Stop playback
    void stop();
    
    // Pause/resume playback
    void togglePause();
    
    // Update the player state
    void update();
    
    // Check if playing
    bool isPlaying() const;
    
    // Check if paused
    bool isPaused() const;
    
    // Volume control
    void increaseVolume();
    void decreaseVolume();
    void toggleNormalization();
    int getVolume() const;
    bool isNormalizationEnabled() const;
    
private:
    // Initialize OPL3 FM instruments
    void initFMInstruments();
    
    // OPL3 emulation functions
    void writeOPL(uint32_t reg, uint8_t val);
    void oplSetupParams(int channel, int& port, int& regOffset, int& operatorOffset);
    void oplNoteOff(int channel);
    void oplNoteOn(int channel, double hertz);
    void oplSetVolume(int channel, int velocity);
    void oplSetVolumeScaled(int channel, int scaledVolume);
    void oplSetInstrument(int channel);
    void oplSetPanning(int channel);
    void oplReset();
    void oplSilence();
    
    // MIDI file parsing
    int readString(FILE* file, int length, char* buffer);
    uint32_t readVarLen(FILE* file);
    uint32_t convertInteger(const char* buffer, int length);
    
    // MIDI event handling
    void processEvents();
    void handleMidiEvent(int trackIndex);
    void deallocateActiveNote(int midiChannel, int activeIndex);
    
    // Data members
    std::string filename;
    FILE* midiFile;
    
    // Playback state
    bool playing;
    bool paused;
    bool began;
    double playWait;
    int globalVolume;
    bool useNormalization;
    
    // MIDI file data
    int trackCount;
    int timeDivision;
    double invTimeDivision;
    double tempo;
    double bendSensitivity;
    
    // Track state data
    std::array<TrackState, MAX_TRACKS> tracks;
    std::array<TrackState, MAX_TRACKS> loopPoints;
    std::array<TrackState, MAX_TRACKS> rollbackPoints;
    double loopWait;
    bool foundLoopStart;
    bool foundLoopEnd;
    
    // MIDI channels
    std::array<MidiChannel, MAX_CHANNELS> midiChannels;
    
    // OPL channels
    std::array<OPLChannel, OPL_CHANNELS> oplChannels;
    
    // FM instrument definitions
    std::array<FMInstrument, 181> instruments;
};

#endif // MIDIPLAYER_H
