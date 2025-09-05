# 🎶 GTK Music Player

A minimalist music player with maximum personality. Built with GTK3 and powered by GStreamer, this app is your retro-modern jukebox for everything from high-fidelity FLAC to nostalgic MIDI jams.

## 🐧 Features That Slap

- 🎧 **Playback Controls**  
  Play, pause, rewind, fast-forward, skip, and stop—because sometimes you just need to slam that ■ button.

- 📜 **Queue Management**  
  Add tracks, clear the list, and loop your playlist like it’s 1999. The queue is visible, scrollable, and totally under your command.

- 🔊 **Volume Slider**  
  From whisper-soft to speaker-melting—scale from 10% to 300% with buttery smoothness.

- ⏱️ **Progress Bar + Timestamp**  
  Know exactly how far into your audio journey you are. Time waits for no one, but this player tracks it anyway.

- 🧠 **Format Support**  
  - **WAV** – Because raw is beautiful.  
  - **MP3** – The people's codec.  
  - **OGG** – Open-source and proud.  
  - **FLAC** – Audiophile-approved.  
  - **MIDI** – With real-time **OPL3 synthesis** for that sweet Sound Blaster nostalgia.  
    Yes, your player can literally sound like a DOS dungeon crawler.

## 🛠️ Building

Just run:

```
make
./musicplayer
```

Requires 
* libflac – For those lossless vibes.
* SDL2 – Powers the retro OPL3 MIDI synthesis.
* GTK3 – For the sleek UI.
* libVorbis  – Because OGG deserves love too.
* GStreamer – Handles all the audio magic.

## 💡 Philosophy

No bloated libraries. No streaming APIs. Just you, your files, and a freedom of choice.

## 🧙‍♂️ Bonus

Supports MIDI playback using **OPL3 emulation**, so your `.mid` files sound like they were ripped straight from a 1990s PC game. If you’ve ever wanted to hear Beethoven through a Yamaha chip, this is your moment.

---

Made with 🐧, 🎶, and a dash of absurdity by Jason Hall.

## 📄 License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).  
Feel free to remix, reuse, and redistribute—just don’t forget to give credit where it’s due.

