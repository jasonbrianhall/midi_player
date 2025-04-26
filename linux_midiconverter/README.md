# MIDI to WAV Converter with FM Synthesis

A high-quality MIDI to WAV conversion utility with OPL3 FM synthesis emulation, based on the DOSBox DBOPL emulator.

## Overview

This project provides a command-line utility for converting MIDI files to WAV format using accurate FM synthesis emulation. It implements a faithful recreation of the Yamaha OPL3 sound chip, which was commonly found in sound cards like the Sound Blaster 16 and AdLib during the 1990s.

## Features

- Accurate OPL3 FM synthesis emulation
- 181 instrument patches covering all General MIDI instruments
- Support for all 16 MIDI channels plus percussion
- OPL3 stereo output
- Volume, pan, and pitch bend control
- High-quality WAV file output
- Adjustable output volume

## Building

To build the project, you'll need:

- C/C++ compiler (GCC, Clang, or MSVC)
- SDL2 development libraries
- Make (optional, for easier building)

### Linux

```bash
# Install dependencies
sudo apt-get install gcc g++ libsdl2-dev cmake

# Build
make
```

### macOS

```bash
# Install dependencies with Homebrew
brew install sdl2

# Build

make
```

### Windows

Using MSYS2/MinGW:

```bash
# Install dependencies
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sdl2

# Build
make
```

## Usage

### Converting MIDI to WAV

```bash
./midiconverter input.mid output.wav [volume]
```

Example:
```bash
./midiconverter song.mid song.wav 400
```

The optional volume parameter (percentage) defaults to 500% (value of 500). This allows you to adjust the output volume to get appropriate levels in the generated WAV file. If your output is too quiet or distorted, try adjusting this value.

### Converting MIDI to WAV

```bash
./midiconverter input.mid output.wav [volume]
```

The optional volume parameter defaults to 500% (value of 500; max is 5000).

## Technical Details

### OPL3 Emulation

The FM synthesis emulation is based on the DOSBox DBOPL emulator, which accurately recreates the Yamaha YMF262 (OPL3) sound chip. Key features:

- Two-operator FM synthesis per voice
- 36 channels in OPL3 mode
- Support for 8 waveforms per operator
- Accurate tremolo and vibrato effects
- Multiple synthesis modes (FM, AM)
- Percussion mode support

### MIDI Implementation

The MIDI player supports:
- All 16 MIDI channels
- Program Change (instrument selection)
- Note On/Off events with velocity
- Pitch Bend
- Volume Control (CC7)
- Pan (CC10)
- Expression (CC11)
- Channel pressure (aftertouch)
- Loop markers via MIDI text events

## Project Structure

- `dbopl.cpp/h` - Core OPL3 emulation
- `dbopl_wrapper.cpp/h` - Wrapper for OPL3 emulation with MIDI support
- `midiplayer.cpp/h` - MIDI file parser and conversion logic
- `instruments.cpp` - FM instrument definitions (181 instruments)
- `virtual_mixer.cpp/h` - Audio mixing system
- `wav_converter.cpp/h` - WAV file output support
- `main.cpp` - MIDI to WAV converter application

## License

This project is licensed under the MIT License.

## Credits

- DOSBox Team - Original DBOPL emulator
- Yamaha - Original OPL3 (YMF262) chip design
- SDL - Audio output and cross-platform support
