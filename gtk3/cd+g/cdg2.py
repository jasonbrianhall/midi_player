#!/usr/bin/env python

import whisper
import cairocffi as cairo
import struct
import sys
import json
import os
import zipfile
from pydub import AudioSegment
from PIL import Image, ImagePalette # Added import from Pillow

# CRITICAL: CDG runs at 300 packets per second, not 75!
CDG_PACKETS_PER_SECOND = 300
CDG_PACKET_SIZE = 24
CDG_SCREEN_WIDTH = 50 # tiles (300 pixels / 6 pixels per tile)
CDG_SCREEN_HEIGHT = 18 # tiles (216 pixels / 12 pixels per tile)

class CDGPacket:
    def __init__(self, command, instruction, data):
        self.command = command
        self.instruction = instruction
        self.data = data

def create_memory_preset_packet(color, repeat=0):
    """Clear entire screen to a color"""
    data = bytearray(16)
    data[0] = color & 0x0F
    data[1] = repeat & 0x0F
    return CDGPacket(0x09, 1, data)

def create_border_preset_packet(color):
    """Set border color"""
    data = bytearray(16)
    data[0] = color & 0x0F
    return CDGPacket(0x09, 2, data)

def create_load_color_table_low_packet(colors):
    """Load colors 0-7 into color table with correct CDG bit packing"""
    data = bytearray(16)
    for i in range(8):
        if i < len(colors):
            r, g, b = colors[i]
            # CDG spec: high_byte = 00RRRRGG, low_byte = 00GGBBBB  
            # From the spec: [---high byte---] [---low byte----]
            #        X X r r r r g g  X X g g b b b b
            high_byte = ((r & 0x0F) << 2) | ((g & 0x0C) >> 2)
            low_byte = ((g & 0x03) << 4) | (b & 0x0F)
            data[i * 2] = high_byte & 0x3F   # Mask to 6 bits
            data[i * 2 + 1] = low_byte & 0x3F  # Mask to 6 bits
    return CDGPacket(0x09, 30, data)

def create_load_color_table_high_packet(colors):
    """Load colors 8-15 into color table with correct CDG bit packing"""
    data = bytearray(16)
    for i in range(8):
        if i < len(colors):
            r, g, b = colors[i]
            # CDG spec: high_byte = 00RRRRGG, low_byte = 00GGBBBB
            high_byte = ((r & 0x0F) << 2) | ((g & 0x0C) >> 2)
            low_byte = ((g & 0x03) << 4) | (b & 0x0F)
            data[i * 2] = high_byte & 0x3F   # Mask to 6 bits  
            data[i * 2 + 1] = low_byte & 0x3F  # Mask to 6 bits
    return CDGPacket(0x09, 31, data)

def create_tile_block_packet(color0, color1, row, column, tile_data):
    """Create a tile block packet (instruction 6)"""
    data = bytearray(16)
    data[0] = color0 & 0x0F
    data[1] = color1 & 0x0F
    data[2] = row & 0x1F
    data[3] = column & 0x3F
    for i in range(12):
        if i < len(tile_data):
            data[4 + i] = tile_data[i] & 0x3F
    return CDGPacket(0x09, 6, data)

def create_tile_block_xor_packet(color0, color1, row, column, tile_data):
    """Create a tile block XOR packet (instruction 38) for highlighting"""
    data = bytearray(16)
    data[0] = color0 & 0x0F
    data[1] = color1 & 0x0F
    data[2] = row & 0x1F
    data[3] = column & 0x3F
    for i in range(12):
        if i < len(tile_data):
            data[4 + i] = tile_data[i] & 0x3F
    return CDGPacket(0x09, 38, data)

def render_text_to_tiles(text, max_width_tiles=48, font_size=12):
    """Render text and return tile data with pixel width"""
    width_pixels = max_width_tiles * 6
    height_pixels = 24
    
    surface = cairo.ImageSurface(cairo.FORMAT_A8, width_pixels, height_pixels)
    cr = cairo.Context(surface)
    
    cr.set_source_rgb(1, 1, 1)
    cr.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    cr.set_font_size(font_size)
    
    # Get text extents for centering (returns tuple: x_bearing, y_bearing, width, height, x_advance, y_advance)
    extents = cr.text_extents(text)
    text_width = extents[2] # width is the 3rd element
    
    # Center the text
    x_pos = (width_pixels - text_width) / 2
    cr.move_to(x_pos, 19)
    cr.show_text(text)
    surface.flush()
    
    data = bytes(surface.get_data())
    stride = surface.get_stride()
    
    tiles = []
    for tile_x in range(max_width_tiles):
        for tile_row in range(2):
            tile_data = []
            for row in range(12):
                bits = 0
                for col in range(6):
                    x = tile_x * 6 + col
                    y = tile_row * 12 + row
                    if x < width_pixels and y < height_pixels:
                        # Use A8 format (1 byte per pixel, value is alpha/greyscale)
                        pixel = data[y * stride + x]
                        if pixel > 128:
                            bits |= (1 << (5 - col))
                tile_data.append(bits)
            tiles.append((tile_row, tile_data))
    
    return tiles, int(text_width)

def render_word_to_tiles(word, font_size=12):
    """Render a single word and return tile data with exact boundaries"""
    # Render with generous padding
    width_pixels = 200
    height_pixels = 24
    
    surface = cairo.ImageSurface(cairo.FORMAT_A8, width_pixels, height_pixels)
    cr = cairo.Context(surface)
    
    cr.set_source_rgb(1, 1, 1)
    cr.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    cr.set_font_size(font_size)
    
    # Get text extents (returns tuple)
    extents = cr.text_extents(word)
    text_width = extents[2] # width is the 3rd element
    
    cr.move_to(10, 19)
    cr.show_text(word)
    surface.flush()
    
    data = bytes(surface.get_data())
    stride = surface.get_stride()
    
    # Find actual bounds of rendered text
    min_x, max_x = width_pixels, 0
    for y in range(height_pixels):
        for x in range(width_pixels):
            if data[y * stride + x] > 128:
                min_x = min(min_x, x)
                max_x = max(max_x, x)
    
    if max_x < min_x:
        return [], 0, 0
    
    # Convert to tiles
    tile_start = min_x // 6
    tile_end = (max_x // 6) + 1
    
    tiles = []
    for tile_x in range(tile_start, tile_end):
        for tile_row in range(2):
            tile_data = []
            for row in range(12):
                bits = 0
                for col in range(6):
                    x = tile_x * 6 + col
                    y = tile_row * 12 + row
                    if x < width_pixels and y < height_pixels:
                        pixel = data[y * stride + x]
                        if pixel > 128:
                            bits |= (1 << (5 - col))
                tile_data.append(bits)
            tiles.append((tile_row, tile_data))
    
    return tiles, tile_start, tile_end - tile_start

def quantize_color_to_4bit(r, g, b):
    """Quantize 8-bit RGB to 4-bit per channel"""
    return (r >> 4, g >> 4, b >> 4)

def load_and_render_image_color(image_path):
    """Load an image and convert it to color CDG tiles with 16-color palette"""
    try:
        # Imports are already at the top of the file
        img = Image.open(image_path)
        
        # Resize to fit CDG screen (300x216 pixels)
        img = img.resize((300, 216), Image.Resampling.LANCZOS)
        img = img.convert('RGB')
        
        # Quantize to 16 colors
        # Ensure palette is provided to quantize() for consistent results
        
        # Create a simple 16-color "web-safe" palette for quantization base
        # This is a common practice to avoid full quantization, but is not strictly necessary
        # We will use the default Image.quantize logic which generates an optimized palette
        img_quantized = img.quantize(colors=16, method=2)
        
        # Extract the 16-color palette
        # The palette is in R, G, B, R, G, B, ... format for 256 entries. We only need the first 16.
        palette_list = img_quantized.getpalette()
        palette = []
        for i in range(16):
            r = palette_list[i * 3]
            g = palette_list[i * 3 + 1]
            b = palette_list[i * 3 + 2]
            palette.append(quantize_color_to_4bit(r, g, b))
        
        print(f"Generated 16-color palette from image")
        
        # Convert image pixels to palette indices
        pixel_map = []
        for y in range(216):
            row = []
            for x in range(300):
                # getpixel returns the palette index (0-15) for a quantized image
                pixel = img_quantized.getpixel((x, y))
                row.append(pixel)
            pixel_map.append(row)
        
        # Convert to tiles
        tiles = []
        for tile_y in range(CDG_SCREEN_HEIGHT):
            for tile_x in range(CDG_SCREEN_WIDTH):
                tile_data = []
                # CDG tiles are 6x12 pixels
                # Pixels 0-5 use color0, Pixels 6-11 use color1
                # But for a full color image, we use Tile Block Set (Instr 6)
                # with Color 0 set to index 0-7 and Color 1 set to index 8-15
                # The bits define whether to use Color 0 (bit=0) or Color 1 (bit=1)
                for row in range(12):
                    bits = 0
                    for col in range(6):
                        x = tile_x * 6 + col
                        y = tile_y * 12 + row
                        if x < 300 and y < 216:
                            color_idx = pixel_map[y][x]
                            # Bit 0 means color 0-7, Bit 1 means color 8-15
                            if color_idx >= 8:
                                bits |= (1 << (5 - col))
                        
                    tile_data.append(bits)
                tiles.append((tile_y, tile_x, tile_data))
        
        return tiles, palette
    except ImportError:
        print("ERROR: PIL/Pillow not installed. Install with: pip install Pillow")
        return None, None
    except Exception as e:
        print(f"ERROR: Could not load image '{image_path}': {e}")
        import traceback
        traceback.print_exc()
        return None, None

def group_words_into_lines(transcript, max_chars_per_line=40, max_words_per_line=6):
    """Group words into lines with better spacing"""
    lines = []
    current_line_words = []
    current_line_text = ""
    
    for entry in transcript:
        word = entry['word'].strip()
        if not word:
            continue
        
        test_text = current_line_text + (' ' if current_line_text else '') + word
        
        if current_line_words and (len(test_text) > max_chars_per_line or len(current_line_words) >= max_words_per_line):
            lines.append({
                'words': current_line_words,
                'text': current_line_text,
                'start': current_line_words[0]['start'],
                'end': current_line_words[-1]['end']
            })
            current_line_words = []
            current_line_text = ""
            test_text = word
            
        current_line_words.append({
            'word': word,
            'start': entry['start'],
            'end': entry['end']
        })
        current_line_text = test_text
    
    if current_line_words:
        lines.append({
            'words': current_line_words,
            'text': current_line_text,
            'start': current_line_words[0]['start'],
            'end': current_line_words[-1]['end']
        })
    
    return lines

def generate_cdg_packets(transcript, song_duration, image_path=None):
    """Generate CDG packets with centered text and line highlighting"""
    total_packets = int(song_duration * CDG_PACKETS_PER_SECOND)
    
    print(f"Generating {total_packets} packets for {song_duration:.2f} seconds")
    print(f"({total_packets / CDG_PACKETS_PER_SECOND:.2f} seconds at {CDG_PACKETS_PER_SECOND} packets/sec)")
    
    packets = []
    
    # Load image first to get palette if present
    image_tiles = None
    image_palette = None
    if image_path and os.path.exists(image_path):
        print(f"\nLoading image: {image_path}")
        image_tiles, image_palette = load_and_render_image_color(image_path)
    elif image_path:
        print(f"WARNING: Image file not found: {image_path}")
    
    # Initialize color table
    # Color 0 (Black), Color 1 (White - Lyrics), Color 4 (Blue - Highlight Default)
    if image_palette:
        colors_low = image_palette[:8]
        colors_high = image_palette[8:16]
        
        # Use an unused slot for highlighting (e.g., slot 15 if available, otherwise slot 4)
        highlight_color = 15
        if len(colors_high) >= 8:
             # Ensure the highlight color is bright blue for contrast
            colors_high[7] = (6, 10, 15) # Put blue in slot 15 (index 7 of colors_high)
        else:
            highlight_color = 4
            colors_low[4] = (6, 10, 15)
            
        print(f"Added blue highlighting color to slot {highlight_color}")
    else:
        # Default Pallete: 0=Black, 1=White (Text), 4=Blue (Highlight)
        colors_low = [(0, 0, 0), (15, 15, 15), (15, 0, 0), (0, 15, 0),
                      (6, 10, 15), (0, 0, 0), (0, 0, 0), (0, 0, 0)]
        colors_high = [(0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0),
                       (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0)]
        highlight_color = 4
    
    packets.append(create_load_color_table_low_packet(colors_low))
    packets.append(create_load_color_table_high_packet(colors_high))
    
    # Clear screen to BLACK
    for repeat in range(16):
        packets.append(create_memory_preset_packet(0, repeat))
    
    packets.append(create_border_preset_packet(0))
    
    # Fill with no-ops up to the total size
    for i in range(len(packets), total_packets):
        packets.append(CDGPacket(0x09, 0, bytearray(16)))
    
    # Display image if loaded
    if image_tiles:
        # Image will display until cleared by lyrics
        print(f"Rendering color image to CDG...")
        start_packet = 50
        for idx, (tile_y, tile_x, tile_data) in enumerate(image_tiles):
            packet_idx = start_packet + idx
            if packet_idx < len(packets):
                # Using colors 0 and 8 for the color image (to cover 0-15)
                packets[packet_idx] = create_tile_block_packet(0, 8, tile_y, tile_x, tile_data)
    
    # Group words into lines
    print("\nGrouping words into lines...")
    lines = group_words_into_lines(transcript, max_chars_per_line=40, max_words_per_line=6)
    print(f"Created {len(lines)} lines\n")
    
    print("Adding lyrics to CDG with line highlighting:")
    print("-" * 60)
    
    # Define 4 line positions (vertically centered)
    line_rows = [5, 7, 9, 11]
    
    for line_idx, line in enumerate(lines):
        start_time = line['start']
        end_time = line['end']
        
        print(f"[{start_time:6.2f}s - {end_time:6.2f}s] Line {line_idx+1:3d}: {line['text']}")
        
        display_position = line_idx % 4
        base_row = line_rows[display_position]
        
        # 1. CLEAR PREVIOUS LINE or FULL SCREEN
        clear_packet_count = 0
        
        if line_idx == 0:
            # Clear full screen about 2 seconds before the first line starts
            clear_time = max(0, start_time - 2.0)
            print(f" [Clearing full screen at {clear_time:.2f}s]")
            # Apply clear memory preset (color 0) - this clears the full screen quickly
            # Repeat is necessary to ensure the command is sent enough times to cover the screen area
            clear_start_packet = int(clear_time * CDG_PACKETS_PER_SECOND)
            for i in range(16):
                idx = clear_start_packet + i
                if idx < len(packets):
                    packets[idx] = create_memory_preset_packet(0, i)
        
        # In a 4-line cycle, clear the line that is being overwritten
        if line_idx >= 4:
            # Get the line index that is *4 lines before* the current one
            old_line_idx = line_idx - 4
            old_line_row = line_rows[old_line_idx % 4]
            clear_time = max(0, start_time - 0.5) # Clear half a second before
            clear_start_packet = int(clear_time * CDG_PACKETS_PER_SECOND)
            
            # Clear the old line area (2 rows of tiles)
            empty_tile = [0] * 12
            packet_offset = 0
            for row in range(old_line_row, old_line_row + 2):
                for col in range(CDG_SCREEN_WIDTH):
                    idx = clear_start_packet + packet_offset
                    if idx < len(packets):
                        # Draw black over the old text area
                        packets[idx] = create_tile_block_packet(0, 0, row, col, empty_tile)
                    packet_offset += 1
            print(f" [Clearing old line area (row {old_line_row}) at {clear_time:.2f}s]")


        # 2. RENDER THE FULL LINE (centered) - WHITE COLOR (Color 1)
        tiles, text_width_pixels = render_text_to_tiles(line['text'], font_size=12)
        text_width_tiles = len(tiles) // 2 # 2 tile rows per column
        start_column = (CDG_SCREEN_WIDTH - text_width_tiles) // 2
        
        start_packet = int(start_time * CDG_PACKETS_PER_SECOND)
        
        # Draw the full line
        tile_idx = 0
        packet_offset = 0 # <--- FIX: Sequential packet counter for tile updates
        
        for col_offset in range(text_width_tiles):
            col = start_column + col_offset
            if 0 <= col < CDG_SCREEN_WIDTH:
                for row_offset in range(2):
                    if tile_idx < len(tiles):
                        row_in_tile, tile_data = tiles[tile_idx]
                        packet_idx = start_packet + packet_offset # <--- Use sequential offset
                        if packet_idx < len(packets):
                            # Text color is white (Color 1)
                            packets[packet_idx] = create_tile_block_packet(
                                0, 1, base_row + row_offset, col, tile_data
                            )
                        tile_idx += 1
                        packet_offset += 1 # <--- Increment sequential offset
        
        # 3. HIGHLIGHT THE FULL LINE - HIGHLIGHT COLOR (highlight_color)
        word_start_time = line['words'][0]['start'] if line['words'] else start_time
        highlight_start = int(word_start_time * CDG_PACKETS_PER_SECOND)
        
        print(f" [Highlighting line at {word_start_time:.2f}s]")
        
        tile_idx = 0
        packet_offset = 0 # <--- FIX: Sequential packet counter for tile updates
        for col_offset in range(text_width_tiles):
            col = start_column + col_offset
            if 0 <= col < CDG_SCREEN_WIDTH:
                for row_offset in range(2):
                    if tile_idx < len(tiles):
                        row_in_tile, tile_data = tiles[tile_idx]
                        packet_idx = highlight_start + packet_offset
                        if packet_idx < len(packets):
                            # Highlight color
                            packets[packet_idx] = create_tile_block_packet(
                                0, highlight_color, base_row + row_offset, col, tile_data
                            )
                        tile_idx += 1
                        packet_offset += 1 # <--- Increment sequential offset
        
        # 4. UN-HIGHLIGHT THE FULL LINE (OPTIONAL) - RESTORE TO WHITE (Color 1)
        # We restore the line to white after the line has been sung.
        unhighlight_time = end_time + 0.1 # Small buffer after the last word
        unhighlight_start = int(unhighlight_time * CDG_PACKETS_PER_SECOND)
        
        print(f" [Un-highlighting line at {unhighlight_time:.2f}s]")
        
        tile_idx = 0
        packet_offset = 0 # <--- FIX: Sequential packet counter for tile updates
        for col_offset in range(text_width_tiles):
            col = start_column + col_offset
            if 0 <= col < CDG_SCREEN_WIDTH:
                for row_offset in range(2):
                    if tile_idx < len(tiles):
                        row_in_tile, tile_data = tiles[tile_idx]
                        packet_idx = unhighlight_start + packet_offset
                        if packet_idx < len(packets):
                            # Restore to white (Color 1)
                            packets[packet_idx] = create_tile_block_packet(
                                0, 1, base_row + row_offset, col, tile_data
                            )
                        tile_idx += 1
                        packet_offset += 1 # <--- Increment sequential offset

    
    print("-" * 60)
    print(f"Total lines added: {len(lines)}\n")
    
    return packets

def write_cdg_file(packets, filename):
    """Write packets to CDG file"""
    with open(filename, 'wb') as f:
        for packet in packets:
            # Command byte: high 2 bits are 1s, low 6 are command (0x09)
            # Instruction byte: high 2 bits are 1s, low 6 are instruction
            f.write(struct.pack('BB', packet.command | 0xC0, packet.instruction | 0xC0))
            f.write(b'\x00\x00') # Parity bytes
            f.write(packet.data[:16])
            f.write(b'\x00\x00\x00\x00') # Parity bytes

def save_transcript_json(transcript, song_duration, filename):
    """Save transcript to JSON file"""
    data = {
        'song_duration': song_duration,
        'words': transcript
    }
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"Saved transcript to: {filename}")

def load_transcript_json(filename):
    """Load transcript from JSON file"""
    with open(filename, 'r', encoding='utf-8') as f:
        data = json.load(f)
    return data['words'], data['song_duration']

def add_silence_to_mp3(input_mp3, output_mp3, silence_duration_ms):
    """Add silence to the beginning of an MP3 file"""
    print(f"Adding {silence_duration_ms/1000:.1f} seconds of silence to audio...")
    try:
        audio = AudioSegment.from_mp3(input_mp3)
        silence = AudioSegment.silent(duration=silence_duration_ms)
        combined = silence + audio
        combined.export(output_mp3, format="mp3")
        print(f"Created audio with silence: {output_mp3}")
        return output_mp3
    except Exception as e:
        print(f"Error processing audio with pydub: {e}")
        print("Ensure ffmpeg is installed and accessible in your path.")
        sys.exit(1)


def transcribe_mp3(mp3_path, image_path=None):
    """Transcribe MP3 and adjust for silence if needed"""
    silence_offset = 0.0
    modified_mp3 = mp3_path
    
    # 1. First, determine if silence is needed without running full transcription
    # Note: We can't easily check 'first_word_time' without transcribing.
    
    # Define silence based on image or a fixed buffer
    if image_path:
        # Add 7 seconds for the image/logo display
        silence_offset = 7.0
        print(f"\nImage specified: Will add 7.0 seconds of silence for logo display.")
    else:
        # Add 2 seconds default buffer
        silence_offset = 2.0
        print(f"\nNo image: Will add 2.0 seconds of silence for introductory buffer.")
    
    # Apply silence
    if silence_offset > 0:
        temp_mp3 = mp3_path.rsplit('.', 1)[0] + '_with_silence.mp3'
        modified_mp3 = add_silence_to_mp3(mp3_path, temp_mp3, int(silence_offset * 1000))
    else:
        modified_mp3 = mp3_path

    # 2. Transcribe the modified MP3 (or original if no silence)
    print(f"\nTranscribing {modified_mp3} with Whisper...")
    model = whisper.load_model("base")
    result = model.transcribe(modified_mp3, word_timestamps=True)
    
    # 3. Calculate final duration and adjust timestamps
    
    # The song duration is the length of the modified_mp3
    audio = AudioSegment.from_mp3(modified_mp3)
    song_duration = len(audio) / 1000.0
    
    # The transcript needs no adjustment because we transcribed the *already silenced* audio.
    # The timestamps in 'result' are relative to the start of the modified MP3.
    
    transcript = []
    for segment in result['segments']:
        for word in segment['words']:
            transcript.append({
                'word': word['word'],
                'start': word['start'],
                'end': word['end']
            })
    
    return transcript, song_duration, modified_mp3

def create_zip_file(cdg_file, mp3_file, output_zip):
    """Create a ZIP file containing the CDG and MP3"""
    try:
        with zipfile.ZipFile(output_zip, 'w', zipfile.ZIP_DEFLATED) as zipf:
            zipf.write(cdg_file, os.path.basename(cdg_file))
            zipf.write(mp3_file, os.path.basename(mp3_file))
        print(f"Created karaoke ZIP: {output_zip}")
        print(f" - {os.path.basename(cdg_file)}")
        print(f" - {os.path.basename(mp3_file)}")
    except Exception as e:
        print(f"Error creating ZIP file: {e}")

def main():
    if len(sys.argv) < 3:
        print("Usage:")
        print(" python cdg.py input.mp3 output.cdg [--json transcript.json] [--image cover.jpg] [--zip output.zip]")
        print(" python cdg.py --from-json transcript.json output.cdg [--image cover.jpg]")
        sys.exit(1)
    
    image_path = None
    if '--image' in sys.argv:
        img_idx = sys.argv.index('--image')
        if img_idx + 1 < len(sys.argv):
            image_path = sys.argv[img_idx + 1]
    
    zip_path = None
    if '--zip' in sys.argv:
        zip_idx = sys.argv.index('--zip')
        if zip_idx + 1 < len(sys.argv):
            zip_path = sys.argv[zip_idx + 1]
    
    if sys.argv[1] == '--from-json':
        json_path = sys.argv[2]
        output_cdg = sys.argv[3]
        
        print(f"Loading transcript from JSON: {json_path}")
        transcript, song_duration = load_transcript_json(json_path)
        print(f"Loaded {len(transcript)} words, duration: {song_duration:.2f}s")
        final_mp3 = None
        
    else:
        mp3_path = sys.argv[1]
        output_cdg = sys.argv[2]
        
        json_path = None
        if '--json' in sys.argv:
            json_idx = sys.argv.index('--json')
            if json_idx + 1 < len(sys.argv):
                json_path = sys.argv[json_idx + 1]
        
        print(f"Starting CD+G generation process...")
        # Transcribe handles adding silence and returns the final MP3 path
        transcript, song_duration, final_mp3 = transcribe_mp3(mp3_path, image_path)
        
        print(f"Song duration: {song_duration:.2f} seconds")
        print(f"Found {len(transcript)} words")
        
        if json_path:
            save_transcript_json(transcript, song_duration, json_path)
    
    print(f"Generating CD+G packets...")
    packets = generate_cdg_packets(transcript, song_duration, image_path)
    
    print(f"Writing CDG file to {output_cdg}...")
    write_cdg_file(packets, output_cdg)
    
    file_size = len(packets) * CDG_PACKET_SIZE
    expected_duration = len(packets) / CDG_PACKETS_PER_SECOND
    print(f"Done. CDG file size: {file_size} bytes ({len(packets)} packets)")
    print(f"CDG duration: {expected_duration:.2f} seconds")
    
    if final_mp3 and (zip_path or True): # Always try to create a zip if an MP3 was processed
        if not zip_path:
            # Create a default zip name
            zip_path = output_cdg.rsplit('.', 1)[0] + '.zip'
        
        # Determine the name of the MP3 file *inside* the zip
        # It should match the CDG name (e.g., SONG.mp3 and SONG.cdg)
        base_name = os.path.splitext(os.path.basename(output_cdg))[0]
        final_mp3_target = os.path.join(os.path.dirname(final_mp3), base_name + '.mp3')
        
        # Rename the processed MP3 to match the CDG name before zipping
        if os.path.abspath(final_mp3) != os.path.abspath(final_mp3_target):
            os.rename(final_mp3, final_mp3_target)
            print(f"Renamed processed audio to: {os.path.basename(final_mp3_target)}")
            final_mp3 = final_mp3_target

        create_zip_file(output_cdg, final_mp3, zip_path)

if __name__ == "__main__":
    # Ensure correct working directory/imports for Cairo/Pillow before main()
    try:
        main()
    except Exception as e:
        print(f"\n--- CRITICAL ERROR ---")
        print(f"An unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
