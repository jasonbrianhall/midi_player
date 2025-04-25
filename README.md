# MIDI FM Synthesis Projects

This repository contains three related MIDI projects that use OPL3 FM synthesis emulation:

1. **MIDI to WAV Converter** (linux_midiconverter) - Converts MIDI files to WAV using OPL3 FM synthesis
2. **MIDI Player for Linux** (linux_midiplayer) - Real-time MIDI player with OPL3 emulation
3. **DJGPP MIDI Player for DOS** (msdos) - MIDI player for DOS using hardware OPL3 synthesis

All three projects share the same core technology: faithful recreation of the Yamaha OPL3 sound chip commonly found in sound cards like the Sound Blaster 16 and AdLib during the 1990s.

## Common Features

- OPL3 FM synthesis emulation (based on DOSBox DBOPL)
- Complete General MIDI instrument support (128+ instrument patches)
- Dedicated percussion channel support
- MIDI event handling including notes, controllers, program changes, etc.
- Volume control and normalization
- Loop point support via "loopStart" and "loopEnd" text markers

## Project Specifics

### MIDI to WAV Converter (linux_midiconverter)

A command-line utility for converting MIDI files to WAV format with high-quality FM synthesis output.

**Key Features:**
- 181 instrument patches covering all General MIDI instruments
- Support for all 16 MIDI channels plus percussion
- Adjustable output volume
- High-quality WAV file output

**Usage:**
```bash
./midi2wav input.mid output.wav [volume]
```

The optional volume parameter (percentage) defaults to 500% (value of 500).

### MIDI Player for Linux (linux_midiplayer)

A real-time MIDI file player that uses OPL3 FM synthesis emulation for playback.

**Key Features:**
- Real-time controls during playback:
  - Volume control (+/-)
  - Pause/resume (Space)
  - Volume normalization toggle (n)
  - Quit (q or Ctrl+C)
- Full General MIDI support with 128 instrument patches

**Usage:**
```bash
./midiplayer filename.mid
```

### DJGPP MIDI Player for DOS (msdos)

A MIDI player for DOS systems that uses OPL3 FM synthesis to play through AdLib-compatible sound cards.

**Key Features:**
- Designed for real DOS systems or DOSBox
- Uses actual hardware OPL3 chip when available
- Visual display of active notes during playback
- Support for 180+ FM instrument definitions

**Usage:**
```
midiplayer [filename.mid]
```

**Playback Controls:**
- **Q** or **ESC**: Quit the player
- **Space**: Pause/Resume playback
- **+/-**: Increase/decrease global volume
- **N**: Toggle volume normalization

## Building

### Linux Projects (MIDI to WAV and Linux MIDI Player)

Requirements:
- C/C++ compiler (GCC, Clang)
- SDL2 development libraries
- CMake (for MIDI to WAV converter)

**Building MIDI to WAV Converter:**
```bash
cd linux_midiconverter
mkdir build && cd build
cmake ..
make
```

**Building Linux MIDI Player:**
```bash
cd linux_midiplayer
make
```

### DOS MIDI Player

The DOS project uses a Dockerfile for easy compilation:

```bash
cd msdos
make
```

The Makefile handles all the necessary steps to create a DOS-compatible executable using the DJGPP compiler in a Docker environment.

## Technical Details

### OPL3 Emulation

The FM synthesis emulation is based on the DOSBox DBOPL emulator, which accurately recreates the Yamaha YMF262 (OPL3) sound chip:

- Two-operator FM synthesis per voice
- Up to 36 channels in OPL3 mode (18 stereo)
- Support for 8 waveforms per operator
- Accurate tremolo and vibrato effects
- Multiple synthesis modes (FM, AM)
- Percussion mode support

### MIDI Implementation

All projects support:
- Program Change (instrument selection)
- Note On/Off events with velocity
- Pitch Bend
- Volume Control (CC7)
- Pan (CC10)
- Expression (CC11)
- Channel pressure (aftertouch)
- Loop markers via MIDI text events

## Project Structure

```
.
├── linux_midiconverter      # MIDI to WAV converter
│   ├── dbopl.cpp/h          # Core OPL3 emulation
│   ├── dbopl_wrapper.cpp/h  # Wrapper for OPL3 emulation with MIDI support
│   ├── instruments.cpp      # FM instrument definitions (181 instruments)
│   ├── main.cpp             # MIDI to WAV converter application
│   ├── midiplayer.cpp/h     # MIDI file parser and conversion logic
│   ├── virtual_mixer.cpp/h  # Audio mixing system
│   └── wav_converter.cpp/h  # WAV file output support
├── linux_midiplayer         # Linux MIDI player
│   ├── dbopl.cpp/h          # OPL3 emulation (from DOSBox)
│   ├── dbopl_wrapper.cpp/h  # Interface between C MIDI player and C++ OPL3 emulator
│   ├── instruments.c        # FM instrument definitions
│   ├── main.c               # Player application
│   └── midiplayer.c/h       # MIDI playback engine and file parser
├── msdos                    # DOS MIDI player
│   ├── instruments.c        # FM instrument definitions for General MIDI
│   ├── midiplayer.c/h       # Main program code
│   └── Makefile             # Build configuration for Docker-based compilation
└── test                     # Test MIDI files
    ├── futuristic.mid
    ├── Take-Me-Home-Country-Roads.mid
    ├── TetrimoneA.mid
    ├── TetrimoneB.mid
    ├── TetrimoneC.mid
    ├── theme.mid
    └── title-screen.mid
```

## Sound Character: FM Synthesis vs. Sample-Based Synthesis

### OPL3 FM Synthesis Sound
The OPL3 FM synthesis used in these projects produces a distinctive retro sound characteristic of 1990s PC gaming and demo scene music. Unlike modern sample-based synthesizers, OPL3 generates sounds through mathematical frequency modulation, creating:

- A "chiptune-like" quality reminiscent of classic DOS games
- Clear, bright tones with distinctive artificial character
- Unique timbre that's immediately recognizable as "AdLib" or "Sound Blaster"
- Crisp, digital sound with limited dynamic range
- Characteristic "metallic" quality for many instruments
- Simple yet charming percussion sounds

### Comparison with FluidSynth
In contrast, FluidSynth and other modern sample-based synthesizers:
- Use recorded audio samples of real instruments
- Produce more realistic, natural instrument sounds
- Have greater dynamic range and expressive capabilities
- Sound more like a professional MIDI orchestra or band
- Require much more memory (SoundFonts can be hundreds of MB)
- Lack the nostalgic "retro gaming" character

The OPL3 synthesis in these projects is intentionally designed to recreate the sound of early PC gaming and music production rather than realistic instrument reproduction. This gives MIDI files played through these tools a distinctive retro gaming aesthetic that many find appealing for its nostalgic qualities.

## Limitations

- FM synthesis is not sample-based, so instruments won't sound exactly like their real counterparts
- Maximum polyphony is limited to 36 notes in OPL3 mode (18 in OPL2 mode)
- Sound quality depends on the OPL3 FM synthesis capabilities
- Not all complex MIDI files may sound as intended due to FM synthesis limitations

## License

All projects are licensed under the MIT License.

## Credits

- DOSBox Team - Original DBOPL emulator
- Yamaha - Original OPL3 (YMF262) chip design
- SDL - Audio output and cross-platform support
