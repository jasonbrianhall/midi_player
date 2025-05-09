# Sound Blaster WAV Player

A DOS-based .WAV file player for Sound Blaster compatible sound cards with double-buffering for smooth playback.

## Features

- Sound Blaster detection and configuration through BLASTER environment variable
- Supports 8-bit and 16-bit WAV files
- Supports mono and stereo playback
- Automatic buffer size optimization based on audio format
- Double-buffering for smooth, continuous playback
- Compatible with all Sound Blaster cards (SB, SB Pro, SB16, AWE32/64)
- Displays detailed WAV file information
- Shows playback progress and duration

## Requirements

- DOS or DOS-compatible environment (DOSBox, FreeDOS, etc.)
- Compatible Sound Blaster card or emulation
- DJGPP compiler for building from source

## Usage

```
WAVEPLAYER <wav_file>
```

Example:
```
WAVEPLAYER SONG.WAV
```

## Sound Blaster Configuration

The program uses the BLASTER environment variable if it exists, falling back to default settings (A220 I7 D1 T5) if not found.

To set the BLASTER environment variable in DOS:
```
SET BLASTER=A220 I7 D1 T5
```

Parameters in the BLASTER variable:
- A: Base I/O port address (A220 = 0x220)
- I: IRQ number (I7 = IRQ 7)
- D: 8-bit DMA channel (D1 = DMA channel 1)
- H: 16-bit DMA channel (H5 = DMA channel 5) 
- T: Sound card type (T5 = SB16)

Common BLASTER settings:
- Sound Blaster 1.0/2.0: `A220 I7 D1 T1`
- Sound Blaster Pro: `A220 I7 D1 T3`
- Sound Blaster 16: `A220 I5 D1 H5 T6`
- Sound Blaster AWE32: `A220 I5 D1 H5 T8`

If you're unsure of your Sound Blaster settings, check your existing BLASTER environment variable:
```
ECHO %BLASTER%
```

## Building from Source

To build with DJGPP:
```
gcc -o waveplayer.exe waveplayer.c
```

For optimized build:
```
gcc -O2 -o waveplayer.exe waveplayer.c
```

## Supported WAV Format

The player supports standard PCM WAV files with the following characteristics:
- Sample rates: 8000Hz - 44100Hz 
- Sample sizes: 8-bit or 16-bit
- Channels: Mono or Stereo
- Format: PCM only (No compression)

## Troubleshooting

- **No sound**: Verify your Sound Blaster is working with other programs. Check the BLASTER environment variable.
- **Distorted sound**: Try a different DMA buffer size or sample rate.
- **Program crashing**: Ensure you're using the correct IRQ and DMA settings for your card.
- **Sound card not detected**: Make sure the base address, IRQ, and DMA settings match your hardware configuration.

## Technical Notes

- Uses DMA double-buffering for smooth playback
- Automatically calculates optimal buffer size based on audio format
- Directly programs the Sound Blaster DSP and DMA controller
- Uses timing-based buffer switching (no interrupt handlers)
- Handles both 8-bit and 16-bit DMA transfers
- Compatible with all Sound Blaster DSP versions (1.0 through 4.x)

## Limitations

- No volume control (uses system mixer settings)
- No seeking within WAV files (plays from beginning to end)
- No support for compressed WAV formats (MP3, ADPCM, etc.)
- Limited to Sound Blaster compatible cards
- No interrupt-based playback (uses timing)

## License

This program is provided as-is, free for educational and personal use.
