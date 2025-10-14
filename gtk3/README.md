# 🎶 Zenamp

A minimalist music/audio player that doesn't mess around. Built with GTK3 and SDL2, this is your no-nonsense jukebox that happens to have excellent taste in both modern formats and retro MIDI synthesis.

## ✨ Features

- **🎧 Full Playback Control**  
  Play, pause, stop, rewind/fast-forward (5-second jumps), and skip between tracks. All the buttons you actually need, none of the bloat you don't.

- **📜 Visual Queue Management**  
  Add multiple files, see what's playing, remove tracks individually, drag-and-drop reordering, and toggle repeat mode. The queue is right there where you can see it—no hidden mysteries.  It is also filterable and sortable.

- **🔊 Flexible Volume Control**  
  Scale from whisper-quiet (10%) to surprisingly loud (300%) with a smooth slider that actually responds to your input.

- **⏱️ Progress Tracking**  
  Draggable progress bar with real-time timestamps. Know where you are, jump to where you want to be.

- **🎨 Audio Visualization**  
  Real-time visual representation of your music with multiple visualization modes:
  - Bubbles visualization
  - Matrix-style effects
  - Fireworks display
  - DNA helix patterns
  - Sudoku Solver
  - More as I have time

Press F9 for fullscreen visualization mode

- **🎚️ 10-Band Equalizer**  
  Fine-tune your audio with a professional-grade equalizer covering frequencies from 31Hz to 16kHz.

- **📝 Playlist Support**  
  Load and save M3U playlists. Organize your music collections and share them easily.

- **🎵 Comprehensive Format Support**  
  - **WAV** — Direct playback, no conversion needed
  - **MP3** — Decoded to WAV in memory using advanced audio conversion
  - **OGG Vorbis** — Converted on-the-fly with libvorbis
  - **FLAC** — Lossless audio, handled gracefully with libFLAC
  - **AIFF** (.aif/.aiff) — Mac's answer to WAV files
  - **OPUS** (.opus) — Open-source, royalty-free, low-latency audio codec
  - **MIDI** (.mid/.midi) — Here's where it gets interesting...
  - **M4A/AAC** - Advanced Codec (Different methods for Windows and Linux)
  - **WMA** - Windows Media Audio (Different methods for Windows and Linux)
  - **CDG** - CD+G files/karaoke files
  - **Generic Files** - Whatever is supported via libavcodec on Linux and natively on Windows *should* work (I opened a MP4 of Alien Romulus and it loaded the audio)

## 🎹 The MIDI Magic

This player includes **real-time OPL3 synthesis** for MIDI files. That means your `.mid` files get processed through an emulated Yamaha OPL3 chip—the same sound generator that powered classic DOS games and early PC audio cards.

Want to hear what your favorite MIDI sounded like on a 1990s Sound Blaster? Now you can. It's nerdy, it's nostalgic, and it sounds exactly like you remember (or wish you could remember).

## 🛠️ Build Requirements

### System Dependencies

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential pkg-config
sudo apt install libgtk-3-dev libsdl2-dev libsdl2-mixer-dev
sudo apt install libvorbis-dev libogg-dev libflac-dev
sudo apt install libopusfile-dev libopus-dev
sudo apt install ffmpeg-devel
```

#### Linux (Fedora/RHEL/CentOS)
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install pkg-config gtk3-devel SDL2-devel SDL2_mixer-devel
sudo dnf install libvorbis-devel libogg-devel flac-devel
sudo dnf install opusfile-devel opus-devel
sudo dnf install ffmpeg-devel
```

#### Linux (Arch)
```bash
sudo pacman -S base-devel pkg-config gtk3 sdl2 sdl2_mixer
sudo pacman -S libvorbis libogg flac opusfile opus
```

#### Cross-compilation for Windows (on Linux)
```bash
# Ubuntu/Debian
sudo apt install mingw-w64 mingw-w64-tools
sudo apt install mingw-w64-x86-64-dev

# For Windows libraries, you'll need mingw-w64 versions:
sudo apt install libgtk-3-dev:mingw-w64
# Or build/install mingw-w64 versions of the required libraries
```

### Required Libraries

| Library | Purpose | Minimum Version |
|---------|---------|-----------------|
| **GTK3** | User interface framework | 3.10+ |
| **SDL2** | Audio backend and cross-platform support | 2.0.5+ |
| **SDL2_mixer** | Additional audio mixing capabilities | 2.0.1+ |
| **libFLAC** | FLAC format decoding | 1.3.0+ |
| **libvorbis** | OGG Vorbis decoding | 1.3.0+ |
| **libogg** | OGG container support | 1.3.0+ |
| **opusfile** | Opus format decoding | 0.7+ |
| **opus** | Opus codec support | 1.1+ |
| **ffmpeg** | (Linux Only) For misc. audio conversion | Latest |

### Development Tools
- **GCC** or **Clang** (C++11 support required)
- **Make** (GNU Make recommended)
- **pkg-config** (for library detection)
- **MinGW-w64** (for Windows cross-compilation)

## 🔨 Building

### Quick Start (Linux)
```bash
git clone https://github.com/jasonbrianhall/midi_player.git
cd midi_player
make
./build/linux/zenamp
```

### Build Options

| Command | Description |
|---------|-------------|
| `make` or `make linux` | Build for Linux (default) |
| `make windows` | Cross-compile for Windows |
| `make debug` | Build debug versions for both platforms |
| `make zenamp-linux-debug` | Build Linux version with debug symbols |
| `make zenamp-windows-debug` | Build Windows version with debug symbols |
| `make clean` | Remove build artifacts |
| `make clean-all` | Remove entire build directory |
| `make install` | Install to /usr/local/bin (Linux only) |
| `make help` | Show all available targets |

### Build Outputs

- **Linux**: `build/linux/zenamp`
- **Windows**: `build/windows/zenamp.exe`
- **Debug versions**: Available in respective debug directories

### Windows Build Notes

The Windows build automatically:
- Includes an embedded icon if `icon.rc` is present
- Collects necessary DLL files if `collect_dlls.sh` script exists
- Uses MinGW-w64 cross-compilation toolchain

Required Windows DLLs will be copied to the build directory, including:
- SDL2.dll
- libgtk-3-0.dll
- libglib-2.0-0.dll
- And various GTK3/audio codec libraries

## 🚀 Usage

### Command Line
```bash
# Play a single file
zenamp song.mp3

# Load multiple files
zenamp *.wav *.mp3 *.mid

# Load a playlist
zenamp playlist.m3u
```

### Keyboard Shortcuts
- **Spacebar**: Play/Pause
- **Left/Right Arrow**: Skip backward/forward 5 seconds
- **Up/Down Arrow**: Previous/Next track
- **F9**: Toggle fullscreen visualization
- **F11**: Toggle fullscreen

### File Operations
- **Drag & Drop**: Add files to queue by dragging them into the window
- **Queue Reordering**: Drag items within the queue to reorder
- **Playlist Support**: Load/save M3U playlists via File menu

## 🎨 Audio Features

### Equalizer
The 10-band equalizer covers these frequencies:
- 31 Hz, 62 Hz, 125 Hz, 250 Hz, 500 Hz
- 1 kHz, 2 kHz, 4 kHz, 8 kHz, 16 kHz

### Volume Control
- Range: 10% to 300% of original volume
- Real-time adjustment during playback
- Persistent across track changes

### Visualization Modes
Multiple real-time visualization effects that respond to audio frequency content. Press F9 for immersive fullscreen experience.

## 🔧 Troubleshooting

### Common Build Issues

**Missing pkg-config files**: 
```bash
# Check if libraries are properly installed
pkg-config --list-all | grep gtk
pkg-config --libs --cflags gtk+-3.0
```

**SDL2 not found**:
```bash
# Verify SDL2 installation
sdl2-config --version
pkg-config --libs sdl2
```

**Windows cross-compilation fails**:
```bash
# Check MinGW installation
x86_64-w64-mingw32-gcc --version
mingw64-pkg-config --version
```

### Runtime Issues

**No audio output**: Check that SDL2 can initialize your audio system and that volume isn't muted.

**MIDI files sound wrong**: This is expected! The OPL3 synthesis intentionally recreates classic FM synthesis sound, not modern General MIDI.

**Missing DLLs on Windows**: The build process should collect required DLLs automatically. If manual collection is needed, check the MinGW installation directory.

## 💭 Design Philosophy

This player does one thing well: plays your local audio files without fuss. No streaming APIs, no cloud integration, no telemetry, no subscription prompts. Just you, your music collection, and a player that gets out of the way.

The interface is clean but not minimalist to a fault—you can see your queue, control playback intuitively, and the progress bar actually lets you seek to specific positions. Revolutionary concepts, apparently.

## 🎮 Technical Details

### Virtual File System
Format conversions happen in memory using a virtual file system, so your disk doesn't get cluttered with temporary files. Conversions are cached for improved performance on repeated playback.

### Audio Pipeline
1. Files are loaded and converted to 16-bit PCM WAV format
2. Audio passes through the 10-band equalizer
3. Volume scaling is applied
4. Data is fed to both the SDL2 audio callback and visualizer
5. Real-time visualization processes frequency domain data

### MIDI Synthesis
The OPL3 emulation provides authentic FM synthesis with:
- 18 operator channels
- Real-time parameter changes
- Authentic instrument patches
- Period-correct sound characteristics

---

## 📄 License

MIT License — use it, modify it, distribute it. Just don't remove the attribution when you do.

**Author:** Jason Hall  
**Repository:** [https://github.com/jasonbrianhall/midi_player](https://github.com/jasonbrianhall/midi_player)

---

*Made with C++, SDL2, and an appreciation for both modern convenience and retro authenticity.*
