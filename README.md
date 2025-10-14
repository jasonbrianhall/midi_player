# 🎵 MIDI FM Synthesis & Audio Player Suite

A comprehensive collection of audio tools centered around authentic OPL3 FM synthesis emulation. This repository houses four distinct projects that share a common foundation: faithful recreation of the legendary Yamaha OPL3 sound chip that defined PC audio in the 1990s.

## 📦 Projects Overview

### 1. 🎶 Zenamp (gtk3/) - Modern Audio Player & Karaoke System
**The flagship application** - A feature-rich, cross-platform audio player that combines modern format support with nostalgic MIDI synthesis, comprehensive karaoke capabilities, and over 30 mesmerizing visualizations.

**Key Highlights:**
- **Universal Format Support**: WAV, AIFF, FLAC, MP3, OGG, Opus, M4A/AAC, WMA, MIDI, CD+G, LRC, and more
- **30+ Audio Visualizations**: From classic waveforms to interactive puzzle solvers (Sudoku, Chess, Checkers)
- **Full Karaoke Support**: CD+G graphics and LRC lyric files with multiple visualization modes
- **10-Band Equalizer**: Professional frequency control (31Hz - 16kHz)
- **OPL3 MIDI Synthesis**: Authentic 1990s Sound Blaster FM synthesis for MIDI files
- **Advanced Queue Management**: Sortable, filterable playlist with full metadata display
- **Speed Control**: 0.1x to 4.0x playback without pitch changes
- **Cross-Platform**: Linux and Windows (via MinGW cross-compilation)

**Technologies:** GTK3, SDL2, SDL2_mixer, libFLAC, libvorbis, opus, ffmpeg  
**Documentation:** [Full Zenamp README](gtk3/README.md)

---

### 2. 🔄 MIDI to WAV Converter (linux_midiconverter/)
**High-Quality Offline Conversion** - Command-line utility for converting MIDI files to WAV format using OPL3 FM synthesis.

**Features:**
- 181 meticulously crafted FM instrument patches
- Complete General MIDI compatibility (128+ instruments)
- Adjustable output volume (default 500%)
- High-quality 16-bit PCM WAV output
- Support for all 16 MIDI channels plus percussion
- Loop point support via MIDI text markers

**Usage:**
```bash
./midi2wav input.mid output.wav [volume_percentage]
```

**Example:**
```bash
./midi2wav soundtrack.mid output.wav 500  # Default 500% volume
./midi2wav quieter.mid output.wav 200     # Quieter output
```

**Build:**
```bash
cd linux_midiconverter
mkdir build && cd build
cmake ..
make
```

**Technologies:** C++, SDL2, CMake, DBOPL emulator

---

### 3. 🎹 Linux MIDI Player (linux_midiplayer/)
**Real-Time Interactive Playback** - Lightweight MIDI player with real-time OPL3 synthesis and interactive controls.

**Features:**
- **Real-Time Controls:**
  - `+/-` - Volume adjustment
  - `Space` - Pause/Resume
  - `N` - Toggle volume normalization
  - `Q` or `Ctrl+C` - Quit
- 128 General MIDI instrument patches with FM synthesis
- Minimal resource usage
- Support for complex MIDI arrangements
- Loop marker detection and handling

**Usage:**
```bash
./midiplayer soundtrack.mid
```

**Build:**
```bash
cd linux_midiplayer
make
```

**Technologies:** C, SDL2, DBOPL emulator

---

### 4. 💾 DOS MIDI Player (msdos/)
**Authentic Hardware Experience** - MIDI player for DOS systems using actual OPL3 hardware or emulation.

**Features:**
- Native DOS compatibility (real hardware or DOSBox)
- Direct OPL3 chip access on compatible sound cards (AdLib, Sound Blaster)
- Visual display of active notes during playback
- 180+ FM instrument definitions
- Period-correct audio experience

**Playback Controls:**
- `Q` or `ESC` - Quit
- `Space` - Pause/Resume
- `+/-` - Volume control
- `N` - Toggle normalization

**Usage:**
```
midiplayer [filename.mid]
```

**Build (via Docker):**
```bash
cd msdos
make
```

**Technologies:** C, DJGPP compiler, Hardware OPL3 I/O

---

## 🎼 The OPL3 Story: Why FM Synthesis?

### What is OPL3?
The **Yamaha YMF262 (OPL3)** was the sound chip that powered iconic sound cards like the **Sound Blaster 16** and **AdLib Gold** in the early-to-mid 1990s. Instead of playing recorded samples, OPL3 generates sound through **Frequency Modulation (FM) synthesis** - a mathematical approach where one waveform modulates the frequency of another.

### The Distinctive Sound
OPL3 FM synthesis produces a sound that's immediately recognizable to anyone who played DOS games in the 90s:

**Character Traits:**
- **Bright, crystalline tones** with a digital edge
- **Metallic, bell-like quality** for many instruments
- **Chiptune aesthetic** that's both retro and charming
- **Crisp percussion** with distinctive attack
- **Nostalgic warmth** that modern sample-based synthesis can't replicate

**Perfect For:**
- Retro gaming soundtracks and remixes
- Demo scene productions
- Chiptune and vintage electronic music
- Educational projects about synthesis history
- Anyone seeking that authentic 90s PC gaming sound

### FM vs. Sample-Based Synthesis

| **OPL3 FM Synthesis** | **Modern Sample Synthesis (FluidSynth)** |
|----------------------|------------------------------------------|
| Mathematical waveform generation | Recorded audio samples |
| Distinctive "retro" character | Realistic instrument reproduction |
| Small memory footprint (<1MB) | Large SoundFont files (100-500MB) |
| 36-note polyphony max | 256+ note polyphony |
| Instant sound generation | Sample playback with interpolation |
| Iconic 90s gaming sound | Professional orchestra sound |

### Why Use FM Synthesis Today?

1. **Nostalgia**: Recreates the exact sound of classic DOS games (Doom, Duke Nukem 3D, Commander Keen)
2. **Character**: Unique sonic fingerprint that stands out in modern productions
3. **Efficiency**: Minimal CPU and memory usage
4. **Education**: Learn about synthesis history and techniques
5. **Art**: Intentional retro aesthetic for creative projects

---

## 🛠️ Technical Architecture

### Shared Core: DBOPL Emulation
All projects use the **DOSBox DBOPL emulator** - a cycle-accurate software recreation of the OPL3 chip:

**Capabilities:**
- **Two-operator FM synthesis** per voice
- **36 channels** in OPL3 mode (18 stereo pairs)
- **8 waveforms** per operator (sine, half-sine, abs-sine, pulse-sine, etc.)
- **Tremolo and vibrato** effects
- **Multiple synthesis modes** (FM, Additive)
- **Dedicated percussion mode** with 5 rhythm instruments
- **Stereo panning** support

**Accuracy:**
- Cycle-accurate timing
- Authentic envelope generators (ADSR)
- Proper frequency scaling and detuning
- Hardware-accurate operator configuration

### MIDI Implementation
All projects implement comprehensive MIDI support:

**Standard MIDI Messages:**
- Note On/Off with velocity
- Program Change (instrument selection)
- Control Change (CC):
  - CC7: Volume
  - CC10: Pan
  - CC11: Expression
  - CC64: Sustain pedal
- Pitch Bend
- Channel Pressure (aftertouch)

**Special Features:**
- **16 melodic channels** + dedicated percussion (channel 10)
- **Loop markers** via MIDI text meta-events (`loopStart`, `loopEnd`)
- **Real-time parameter updates** during playback
- **Velocity-sensitive dynamics**

### Instrument Library
The projects share a comprehensive library of **128+ General MIDI instruments**, meticulously crafted for FM synthesis:

**Categories:**
- Piano (8 variants: Acoustic Grand, Bright, Electric, Honky-tonk, etc.)
- Chromatic Percussion (8: Celesta, Glockenspiel, Music Box, Vibraphone, etc.)
- Organ (8: Drawbar, Percussive, Rock, Church, Reed, Accordion, etc.)
- Guitar (8: Acoustic, Electric, Distortion, Harmonics, etc.)
- Bass (8: Acoustic, Electric, Fingered, Picked, Fretless, Slap, etc.)
- Strings (8: Ensemble, Slow, Synth, Orchestra, etc.)
- Ensemble (8: Choir, Voice, Brass, String sections, etc.)
- Brass (8: Trumpet, Trombone, Tuba, Muted, French Horn, etc.)
- Reed (8: Saxophone family, Oboe, Clarinet, etc.)
- Pipe (8: Flute, Recorder, Pan Flute, Ocarina, etc.)
- Synth Lead (8: Square, Sawtooth, Calliope, etc.)
- Synth Pad (8: Warm, Polysynth, Choir, Metallic, etc.)
- Synth Effects (8: Rain, Soundtrack, Crystal, Atmosphere, etc.)
- Ethnic (8: Sitar, Banjo, Shamisen, Koto, Kalimba, etc.)
- Percussive (8: Tinkle Bell, Agogo, Steel Drums, Woodblock, etc.)
- Sound Effects (8: Guitar Fret Noise, Seashore, Bird Tweet, Helicopter, etc.)

**Plus 47 Percussion Instruments** for channel 10

---

## 🚀 Quick Start Guide

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install build-essential pkg-config cmake
sudo apt install libsdl2-dev libsdl2-mixer-dev
sudo apt install libgtk-3-dev  # For Zenamp only
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake pkg-config SDL2-devel SDL2_mixer-devel
sudo dnf install gtk3-devel  # For Zenamp only
```

**Linux (Arch):**
```bash
sudo pacman -S base-devel cmake pkg-config sdl2 sdl2_mixer
sudo pacman -S gtk3  # For Zenamp only
```

### Building Everything

**1. Clone Repository:**
```bash
git clone https://github.com/jasonbrianhall/midi_player.git
cd midi_player
```

**2. Build Zenamp (Audio Player):**
```bash
cd gtk3
make
./build/linux/zenamp
```

**3. Build MIDI to WAV Converter:**
```bash
cd linux_midiconverter
mkdir build && cd build
cmake ..
make
./midi2wav ../test/theme.mid output.wav
```

**4. Build Linux MIDI Player:**
```bash
cd linux_midiplayer
make
./midiplayer ../test/theme.mid
```

**5. Build DOS MIDI Player:**
```bash
cd msdos
make  # Requires Docker
```

---

## 📁 Repository Structure

```
midi_player/
│
├── gtk3/                          # Zenamp - Full-featured audio player
│   ├── *.cpp, *.h                 # All source and header files (flat structure)
│   ├── Makefile                   # Build system (creates build/linux and build/windows)
│   ├── zenamp.spec                # RPM packaging
│   └── README.md                  # Comprehensive documentation
│
├── linux_midiconverter/           # MIDI to WAV converter
│   ├── dbopl.cpp/h                # OPL3 emulation core
│   ├── dbopl_wrapper.cpp/h        # C++ wrapper for OPL3
│   ├── instruments.cpp            # 181 FM instrument patches
│   ├── midiplayer.cpp/h           # MIDI parser and logic
│   ├── virtual_mixer.cpp/h        # Audio mixing system
│   ├── wav_converter.cpp/h        # WAV file output
│   ├── main.cpp                   # Application entry point
│   └── CMakeLists.txt             # CMake build configuration
│
├── linux_midiplayer/              # Real-time MIDI player
│   ├── dbopl.cpp/h                # OPL3 emulation (DOSBox)
│   ├── dbopl_wrapper.cpp/h        # C/C++ interface layer
│   ├── instruments.c              # FM instrument definitions
│   ├── midiplayer.c/h             # Playback engine
│   ├── main.c                     # Player application
│   └── Makefile                   # Build configuration
│
├── msdos/                         # DOS MIDI player
│   ├── instruments.c              # FM patches for DOS
│   ├── midiplayer.c/h             # DOS-specific implementation
│   ├── Makefile                   # Docker-based DJGPP build
│   └── Dockerfile                 # DJGPP build environment
│
└── test/                          # Test MIDI files
    ├── futuristic.mid
    ├── Take-Me-Home-Country-Roads.mid
    ├── TetrimoneA.mid
    ├── TetrimoneB.mid
    ├── TetrimoneC.mid
    ├── theme.mid
    └── title-screen.mid
```

---

## 🎯 Use Cases & Applications

### For Musicians & Producers
- Create authentic retro game soundtracks
- Convert MIDI libraries to WAV for distribution
- Experiment with FM synthesis techniques
- Generate chiptune-style backing tracks

### For Game Developers
- Add authentic 90s sound to retro-style games
- Generate in-game music with minimal file sizes
- Real-time MIDI playback in SDL2-based engines
- Period-correct DOS game audio

### For Retro Computing Enthusiasts
- Experience authentic Sound Blaster audio
- Play MIDI files on real DOS hardware
- Archive and convert vintage MIDI collections
- Demonstrate historical sound technology

### For Content Creators
- Background music with distinctive retro character
- Karaoke creation and hosting
- Audio visualization for streams and videos
- Music library management and playback

### For Educators
- Teach synthesis concepts and history
- Demonstrate MIDI protocol implementation
- Show evolution of computer audio
- Hands-on FM synthesis experiments

---

## 🔧 Performance & Specifications

### Zenamp Performance
| Metric | Typical | Heavy Load |
|--------|---------|------------|
| Memory Usage | 50-150 MB | 200-300 MB (large queue) |
| CPU Usage (Playing) | 10-20% | ~100% (Beat Chess viz) |
| CPU Usage (Minimized) | ~3% | ~5% |
| Startup Time | <1 second | - |
| File Load Time | Instant | 1-2 seconds (large files) |

### MIDI Player Performance
| Metric | Value |
|--------|-------|
| Memory Usage | <50 MB |
| CPU Usage | 5-15% |
| Latency | <10ms |
| Max Polyphony | 36 notes |

### Converter Performance
| Metric | Value |
|--------|-------|
| Conversion Speed | 10-20x real-time |
| Memory Usage | <100 MB |
| Max File Size | No practical limit |

---

## 🎨 Audio Quality Specifications

**Sample Rate:** 44.1 kHz (CD quality)  
**Bit Depth:** 16-bit signed PCM  
**Channels:** Stereo (2-channel)  
**Dynamic Range:** ~90 dB (hardware OPL3), ~96 dB (emulation)  
**Polyphony:** 36 voices (OPL3 mode), 18 voices (OPL2 mode)  
**Frequency Range:** 20 Hz - 20 kHz  

---

## ⚠️ Known Limitations

### General
- **Polyphony Limit:** Maximum 36 simultaneous notes (OPL3 hardware constraint)
- **Mono Sources:** Some instruments may sound monophonic due to FM synthesis limitations
- **Not Sample-Based:** Instruments won't sound identical to real orchestral recordings

### Platform-Specific
- **DOS Player:** Requires OPL3-compatible hardware or DOSBox emulation
- **Windows (Zenamp):** M4A/WMA support requires Windows 7+ codecs
- **Linux (Zenamp):** FFmpeg required for exotic format support

### Performance Notes
- **Sudoku Visualization:** Not multi-threaded, may cause brief pauses during solving
- **Beat Chess:** Uses ~100% CPU (intentionally multi-threaded)
- **Large Queues:** 1000+ files may increase memory usage significantly

---

## 🤝 Contributing

Contributions are welcome across all projects! Here's how you can help:

### Areas for Contribution
1. **Bug Fixes** - Improve stability and compatibility
2. **New Visualizations** - Add creative audio-reactive displays
3. **Format Support** - Extend audio format compatibility
4. **Documentation** - Improve READMEs and comments
5. **Instrument Patches** - Refine FM synthesis presets
6. **Translations** - Add internationalization support
7. **Platform Support** - Port to macOS, BSD, etc.

### Development Process
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with clear commit messages
4. Test thoroughly on your platform
5. Submit a pull request with detailed description

### Community Requests
Feature suggestions are welcomed! Submit ideas via GitHub Issues or Discussions.

### Code Style
- Follow existing code formatting
- Comment complex algorithms
- Keep functions focused and modular
- Use meaningful variable names
- Add error handling where appropriate

---

## 🐛 Troubleshooting

### Build Issues

**"SDL2 not found":**
```bash
# Verify installation
pkg-config --modversion sdl2
sdl2-config --version

# Reinstall if needed
sudo apt install libsdl2-dev  # Ubuntu/Debian
```

**"GTK3 not found" (Zenamp):**
```bash
# Check installation
pkg-config --modversion gtk+-3.0

# Install if missing
sudo apt install libgtk-3-dev
```

**CMake configuration fails:**
```bash
# Clear cache and retry
rm -rf build/
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Runtime Issues

**No audio output (Linux):**
```bash
# Check SDL2 audio driver
SDL_AUDIODRIVER=pulse ./zenamp  # Try PulseAudio
SDL_AUDIODRIVER=alsa ./zenamp   # Try ALSA

# Verify audio device
aplay -l  # List playback devices
```

**Missing DLLs (Windows Zenamp):**
- Check `build/windows/` directory for all required DLLs
- Re-run `collect_dlls.sh` if necessary
- Ensure MinGW runtime libraries are included

**MIDI sounds wrong:**
- This is expected! OPL3 FM synthesis intentionally recreates 1990s sound
- Not meant to sound like modern orchestral MIDI
- For realistic MIDI, use FluidSynth with SoundFonts instead

**Visualizations lag:**
- Reduce sensitivity slider
- Close other GPU-intensive applications
- Minimize window to reduce CPU usage
- Avoid CPU-heavy visualizations (Beat Chess, Sudoku)

---

## 📚 Additional Resources

### Documentation
- [Zenamp Full Documentation](gtk3/README.md)
- [OPL3 Chip Specification](http://www.shikadi.net/moddingwiki/OPL_chip)
- [General MIDI Standard](https://www.midi.org/specifications)
- [FM Synthesis Guide](https://en.wikipedia.org/wiki/Frequency_modulation_synthesis)

### Community
- [GitHub Issues](https://github.com/jasonbrianhall/midi_player/issues) - Bug reports & features
- [GitHub Discussions](https://github.com/jasonbrianhall/midi_player/discussions) - Q&A & ideas

### Related Projects
- [DOSBox](https://www.dosbox.com/) - DOS emulator with OPL3 support
- [FluidSynth](https://www.fluidsynth.org/) - Sample-based MIDI synthesis
- [Audacious](https://audacious-media-player.org/) - Alternative Linux audio player

---

## 📄 License

All projects in this repository are licensed under the **MIT License**.

```
MIT License

Copyright (c) 2025 Jason Hall

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

---

## 🙏 Acknowledgments

### Core Technology
- **DOSBox Team** - Original DBOPL OPL3 emulator
- **Yamaha** - OPL3 (YMF262) chip design
- **DJ Delorie** - DJGPP DOS compiler

### Libraries & Frameworks
- **SDL Team** - SDL2 audio and cross-platform support
- **GTK Team** - GTK3 user interface framework
- **Xiph.Org** - libFLAC, libvorbis, opus codecs
- **FFmpeg Team** - Universal media conversion
- **miniz** - Compact ZIP file handling

### Special Thanks
- Contributors and bug reporters
- Retro computing community
- Demoscene musicians and producers
- Everyone who believes local music players still matter

---

## 📞 Contact & Support

**Author:** Jason Hall  
**Repository:** [github.com/jasonbrianhall/midi_player](https://github.com/jasonbrianhall/midi_player)  
**Issues:** [GitHub Issues Page](https://github.com/jasonbrianhall/midi_player/issues)

---

**⭐ Star this repository if these projects bring joy to your audio experience!**

*Made with C++, C, SDL2, and a deep appreciation for both cutting-edge features and authentic retro sound. Long live local audio!*
