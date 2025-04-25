# DJGPP MIDI Player with FM Synthesis

A MIDI file player for DOS systems that uses OPL3 FM synthesis to play MIDI files through AdLib-compatible sound cards.

## Features

- Plays standard MIDI files (.mid)
- Uses OPL FM synthesis for sound generation
- Includes 180+ FM instrument definitions based on General MIDI standard
- Full support for MIDI events (notes, controllers, program changes, etc.)
- Real-time volume control and normalization
- Visual display of active notes during playback
- Loop point support with "loopStart" and "loopEnd" text markers

## Requirements

- DOS-compatible system (or DOSBox)
- AdLib, Sound Blaster, or OPL3-compatible sound card
- DJGPP compiler for building from source

## Usage

```
midiplayer [filename.mid]
```

### Playback Controls

- **Q** or **ESC**: Quit the player
- **Space**: Pause/Resume playback
- **+/-**: Increase/decrease global volume
- **N**: Toggle volume normalization

## Technical Details

The player uses OPL3 FM synthesis to generate sounds. Each MIDI instrument is mapped to a corresponding FM instrument definition with carrier and modulator settings.

Key technical aspects:
- Supports up to 18 simultaneous polyphonic notes (9 notes in OPL2 mode)
- Intelligent channel allocation for note priorities
- MIDI percussion channel (10) uses dedicated percussion instrument definitions
- Handles all standard MIDI events including pitch bend, modulation, etc.

## Compilation

This project includes a Makefile for easy compilation. The build process requires Docker:

```
# Build using the included Makefile
make
```

The Makefile handles all the necessary steps to create a DOS-compatible executable using the DJGPP compiler in a Docker environment, eliminating the need to install DJGPP directly on your system.

If you prefer to compile manually and have DJGPP installed, you can use:

```
gcc -o midiplayer midiplayer.c instruments.c -lm
```

## Limitations

This version is designed for DOS systems with OPL3-compatible sound hardware. It has several limitations:

- Limited polyphony (maximum 18 simultaneous notes)
- Sound quality depends on the OPL3 FM synthesis capabilities
- Not all complex MIDI files may sound as intended due to FM synthesis limitations
- No support for external soundfonts or wavetable synthesis

## File Structure

- `midiplayer.c`: Main program code
- `instruments.c`: FM instrument definitions for General MIDI instruments
- `midiplayer.h`: Header file with declarations and constants
- `Makefile`: Build configuration for Docker-based compilation

## License

MIT License

```
Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Acknowledgments

This project is based on original QBasic/GWBasic code and adapted for C with DJGPP.
