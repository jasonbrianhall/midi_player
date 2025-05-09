# DOS OPL3 MIDI Player

A DOS-based MIDI player and converter using OPL3 emulation for high-quality FM synthesis audio playback on MS-DOS systems.

## Overview

This project is a complete MIDI player and converter for MS-DOS systems using DJGPP. It leverages the DOSBox OPL3 emulation code (DBOPL) to provide accurate FM synthesis similar to classic Sound Blaster cards.

Key features:
- Direct playback of MIDI files using OPL3 FM synthesis
- Conversion of MIDI files to WAV format
- Support for 16-bit stereo audio
- Various playback controls (pause, volume, etc.)
- Sound Blaster compatible WAV playback

## System Requirements

- MS-DOS system or DOSBox
- 386 processor or better
- 4MB RAM minimum
- Sound Blaster compatible sound card (for WAV playback)
- DJGPP compiler (for building from source)

## Usage

```
MIDPLAY [options] <midi_file>
```

### Options

- `-c, --convert WAV`  Convert MIDI to WAV file and exit
- `-v, --volume N`     Set volume (default: 1000%)
- `-h, --help`         Display help and exit

### Playback Controls

During playback, the following keys are available:
- **Space**: Pause/Resume playback
- **+/-**: Increase/Decrease volume
- **N**: Toggle volume normalization
- **Q/ESC**: Quit playback
- **Ctrl+C**: Stop playback

## Components

The project consists of several main components:

### 1. DBOPL Emulator
- `dbopl.cpp` / `dbopl.h`: Core OPL3 emulation code from DOSBox
- `dbopl_wrapper.cpp` / `dbopl_wrapper.h`: C++ wrapper for easier integration

### 2. MIDI Player
- `midiplayer_dos.cpp` / `midiplayer_dos.h`: MIDI file parser and player
- `virtual_mixer.cpp` / `virtual_mixer.h`: Software audio mixer

### 3. WAV Conversion and Playback
- `midi_to_wav.cpp` / `midi_to_wav.h`: MIDI to WAV converter
- `wav_converter.cpp` / `wav_converter.h`: WAV file writer
- `sb_player.cpp` / `sb_player.h`: Sound Blaster WAV player

### 4. DOS-specific Utilities
- `dos_utils.cpp` / `dos_utils.h`: DOS terminal and utility functions
- `sdl_minimal.cpp` / `sdl_minimal.h`: Minimal SDL-compatible API for DOS

### 5. Main Application
- `main_dos.cpp`: Main application entry point

## How It Works

The player works in two main modes:

### Direct Playback Mode
1. Parses the MIDI file and schedules events
2. Uses DBOPL to emulate an OPL3 chip in real-time
3. Converts OPL3 output to digital audio
4. Sends audio to the sound card

### Conversion Mode
1. Parses the MIDI file
2. Processes MIDI events through the OPL3 emulator
3. Captures audio output
4. Writes to a WAV file

## OPL3 Emulation Details

The OPL3 emulation is based on the DOSBox DBOPL emulator, which provides:
- Accurate emulation of the Yamaha YMF262 (OPL3) chip
- Support for all OPL3 features:
  - 18 2-operator channels or 6 4-operator channels
  - Stereo output
  - 4 waveforms per operator
  - Various modulation modes

## Building from Source

To build using DJGPP:

1. Install DJGPP compiler toolkit
2. Clone the repository
3. Run `make` to build the executable
4. The resulting `MIDPLAY.EXE` is the main application

## Configuration

The player automatically detects Sound Blaster settings from the BLASTER environment variable. Example:
```
SET BLASTER=A220 I7 D1 H5
```

Where:
- A220: I/O port address (220h)
- I7: IRQ number
- D1: 8-bit DMA channel
- H5: 16-bit DMA channel

## FM Instrument Mapping

The player includes a built-in set of 181 FM instrument definitions:
- Instruments 0-127: General MIDI instruments
- Instruments 128-180: General MIDI percussion instruments

## License

This software is released under the MIT License, which allows for use, modification, and distribution with minimal restrictions.

## Acknowledgments

- DOSBox Team for the original DBOPL OPL3 emulation code
- DJGPP project for the DOS C/C++ compiler and runtime environment
