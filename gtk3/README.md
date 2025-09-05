# 🎶 GTK Music Player

A minimalist music player that doesn't mess around. Built with GTK3 and SDL2, this is your no-nonsense jukebox that happens to have excellent taste in both modern formats and retro MIDI synthesis.

## ✨ What It Does

- **🎧 Full Playback Control**  
  Play, pause, stop, rewind/fast-forward (5-second jumps), and skip between tracks. All the buttons you actually need, none of the bloat you don't.

- **📜 Visual Queue Management**  
  Add multiple files, see what's playing, remove tracks individually, and toggle repeat mode. The queue is right there where you can see it—no hidden mysteries.

- **🔊 Flexible Volume Control**  
  Scale from whisper-quiet (10%) to surprisingly loud (300%) with a smooth slider that actually responds to your input.

- **⏱️ Progress Tracking**  
  Draggable progress bar with real-time timestamps. Know where you are, jump to where you want to be.

- **🎵 Comprehensive Format Support**  
  - **WAV** – Direct playback, no conversion needed
  - **MP3** – Decoded to WAV in memory  
  - **OGG Vorbis** – Converted on-the-fly
  - **FLAC** – Lossless audio, handled gracefully
  - **MIDI** (.mid/.midi) – Here's where it gets interesting...

## 🎹 The MIDI Magic

This player includes **real-time OPL3 synthesis** for MIDI files. That means your `.mid` files get processed through an emulated Yamaha OPL3 chip—the same sound generator that powered classic DOS games and early PC audio cards. 

Want to hear what your favorite MIDI sounded like on a 1990s Sound Blaster? Now you can. It's nerdy, it's nostalgic, and it sounds exactly like you remember (or wish you could remember).

## 🛠️ Building This Thing

### Dependencies
You'll need these libraries installed:
- **GTK3** – For the user interface
- **SDL2** – Audio backend and OPL3 emulation
- **libFLAC** – FLAC format support  
- **libvorbis** – OGG Vorbis decoding
- **Standard build tools** – gcc, make, the usual suspects

### Compilation
```bash
make
./build/linux/musicplayer
```

That's it. No configure scripts, no cmake mysteries, just straightforward compilation.

## 💭 Design Philosophy

This player does one thing well: plays your local audio files without fuss. No streaming APIs, no cloud integration, no telemetry, no subscription prompts. Just you, your music collection, and a player that gets out of the way.

The interface is clean but not minimalist to a fault—you can see your queue, control playback intuitively, and the progress bar actually lets you seek to specific positions. Revolutionary concepts, apparently.

## 🎮 Bonus Points

The OPL3 MIDI synthesis isn't just a gimmick—it's a faithful recreation of how these files were meant to sound on period hardware. If you've got a collection of game music MIDIs or just appreciate the warm, slightly crunchy tone of FM synthesis, this feature alone makes the player worth trying.

Plus, the virtual file system handles format conversions in memory, so your disk doesn't get cluttered with temporary WAV files every time you play an MP3.

---

**Made with C++, SDL2, and an appreciation for both modern convenience and retro authenticity.**

## 📄 License

MIT License – use it, modify it, distribute it. Just don't remove the attribution when you do.

**Author:** Jason Hall  
**Repository:** [https://github.com/jasonbrianhall/midi_player](https://github.com/jasonbrianhall/midi_player)
