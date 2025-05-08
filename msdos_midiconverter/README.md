# MIDI to WAV Converter for MS-DOS

A command-line tool for converting MIDI files to WAV format on MS-DOS systems, using the DOSBox DBOPL FM synthesis emulator for authentic OPL3 sound.

## Features

- Convert MIDI files to WAV audio files on MS-DOS systems
- High-quality OPL3 FM synthesis emulation (using DOSBox's DBOPL)
- General MIDI compatible instrument set
- Volume control
- DJGPP DPMI support for running in protected mode
- 16-bit stereo output at 44.1kHz

## System Requirements

- 80386 processor or better
- MS-DOS 5.0 or higher
- 4MB RAM
- DPMI provider (included)

## Usage

```
MIDICONV.EXE <input_midi> <output_wav> [volume]
```

Parameters:
- `input_midi`: Path to the input MIDI file
- `output_wav`: Path to the output WAV file
- `volume`: Optional volume percentage (default: 500%)

Example:
```
MIDICONV.EXE CANYON.MID CANYON.WAV 300
```

## Controls During Conversion

During the conversion process, you can:
- Press `Q` or `ESC` to cancel the conversion
- Press `Ctrl+C` to stop

## Building From Source

### Using Docker (recommended)

The project includes a Makefile for building with DJGPP using Docker:

1. Make sure you have Docker installed
2. Run `make msdos` to build the DOS executable
3. The resulting files will be:
   - `midiconv.exe` - The main executable
   - `CWSDPMI.EXE` - DPMI provider

### Local Build (if you have DJGPP installed)

If you have DJGPP installed locally:

1. Run `make local-build`
2. The executable will be created in the current directory

## Technical Details

### Implementation

This MIDI to WAV converter uses the DOSBox DBOPL emulator (a faithful emulation of the Yamaha YMF262 OPL3 FM synthesis chip) to render MIDI files with authentic FM synthesis sound. The project includes:

- DBOPL emulator from DOSBox
- Custom MIDI file parser
- OPL3 FM instrument definitions
- WAV file writer
- Minimal SDL implementation for DOS
- Virtual mixer for audio channel management

### OPL3 Synthesis

The converter generates sound using 2-operator FM synthesis with the following capabilities:
- 36 total operator channels
- 18 melodic voices in 2-operator mode
- Percussion mode support
- Stereo panning
- Pitch bend
- Volume control

### Source Code Organization

- `dbopl.cpp/h` - DOSBox OPL3 emulator
- `dbopl_wrapper.cpp/h` - MIDI-to-OPL mapping layer
- `midiplayer_dos.cpp/h` - MIDI file parser and playback
- `main_dos.cpp` - Main program and conversion logic
- `instruments.cpp` - FM instrument definitions
- `wav_converter.cpp/h` - WAV file creation
- `virtual_mixer.cpp/h` - Audio mixing system
- `sdl_minimal.cpp/h` - Minimal SDL implementation for DOS
- `dos_utils.cpp/h` - DOS-specific utilities

## Credits

- DOSBox Team - Original DBOPL emulator code
- The OPL3 instrument bank is derived from the classic AdLib MIDI player

## License

All projects are licensed under the MIT License.

