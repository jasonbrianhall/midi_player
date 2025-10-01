import whisper
import cairocffi as cairo
import struct
import sys
import json
import os
import zipfile
from pydub import AudioSegment

# CRITICAL: CDG runs at 300 packets per second, not 75!
CDG_PACKETS_PER_SECOND = 300
CDG_PACKET_SIZE = 24
CDG_SCREEN_WIDTH = 50  # tiles (300 pixels / 6 pixels per tile)
CDG_SCREEN_HEIGHT = 18  # tiles (216 pixels / 12 pixels per tile)

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
            #                X X r r r r g g   X X g g b b b b
            high_byte = ((r & 0x0F) << 2) | ((g & 0x0C) >> 2)
            low_byte = ((g & 0x03) << 4) | (b & 0x0F)
            data[i * 2] = high_byte & 0x3F      # Mask to 6 bits
            data[i * 2 + 1] = low_byte & 0x3F   # Mask to 6 bits
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
            data[i * 2] = high_byte & 0x3F      # Mask to 6 bits  
            data[i * 2 + 1] = low_byte & 0x3F   # Mask to 6 bits
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
    text_width = extents[2]  # width is the 3rd element
    
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
    text_width = extents[2]  # width is the 3rd element
    
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
        from PIL import Image
        img = Image.open(image_path)
        
        # Resize to fit CDG screen (300x216 pixels)
        img = img.resize((300, 216), Image.Resampling.LANCZOS)
        img = img.convert('RGB')
        
        # Quantize to 16 colors
        img_quantized = img.quantize(colors=16, method=2)
        img_rgb = img_quantized.convert('RGB')
        
        # Extract the 16-color palette
        palette_img = img_quantized.getpalette()
        palette = []
        for i in range(16):
            r = palette_img[i * 3]
            g = palette_img[i * 3 + 1]
            b = palette_img[i * 3 + 2]
            palette.append(quantize_color_to_4bit(r, g, b))
        
        print(f"Generated 16-color palette from image")
        
        # Convert image pixels to palette indices
        pixel_map = []
        for y in range(216):
            row = []
            for x in range(300):
                pixel = img_quantized.getpixel((x, y))
                row.append(pixel)
            pixel_map.append(row)
        
        # Convert to tiles
        tiles = []
        for tile_y in range(CDG_SCREEN_HEIGHT):
            for tile_x in range(CDG_SCREEN_WIDTH):
                tile_data = []
                for row in range(12):
                    bits = 0
                    for col in range(6):
                        x = tile_x * 6 + col
                        y = tile_y * 12 + row
                        if x < 300 and y < 216:
                            color_idx = pixel_map[y][x]
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
    """Generate CDG packets with centered text and word-by-word XOR highlighting"""
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
    if image_palette:
        # Keep the original image palette intact, just add one highlighting color
        colors_low = image_palette[:8]
        colors_high = image_palette[8:16]
        
        # Only modify one unused slot for highlighting, try slot 15 (last one)
        if len(colors_high) >= 8:
            colors_high[7] = (6, 10, 15)  # Put blue in slot 15
            highlight_color = 15
        else:
            colors_high.append((6, 10, 15))
            highlight_color = 15
            
        print(f"Added blue highlighting color to slot {highlight_color}")
    else:
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
    
    # Fill with no-ops
    for i in range(len(packets), total_packets):
        packets.append(CDGPacket(0x09, 0, bytearray(16)))
    
    # Display image if loaded
    first_lyric_time = transcript[0]['start'] if transcript else 5.0
    
    if image_tiles:
        print(f"Rendering color image to CDG (will display until lyrics at {first_lyric_time:.2f}s)...")
        start_packet = 50
        for idx, (tile_y, tile_x, tile_data) in enumerate(image_tiles):
            packet_idx = start_packet + idx
            if packet_idx < len(packets):
                packets[packet_idx] = create_tile_block_packet(0, 8, tile_y, tile_x, tile_data)
    
    # Group words into lines
    print("\nGrouping words into lines...")
    lines = group_words_into_lines(transcript, max_chars_per_line=40, max_words_per_line=6)
    print(f"Created {len(lines)} lines\n")
    
    print("Adding lyrics to CDG with word highlighting:")
    print("-" * 60)
    
    # Define 4 line positions (vertically centered)
    line_rows = [5, 7, 9, 11]
    
    for line_idx, line in enumerate(lines):
        start_time = line['start']
        end_time = line['end']
        
        print(f"[{start_time:6.2f}s - {end_time:6.2f}s] Line {line_idx+1:3d}: {line['text']}")
        
        # Clear entire screen before first lyric
        if line_idx == 0 and start_time >= 1.0:
            clear_start_time = max(0.5, start_time - 3.0)
            clear_packet = int(clear_start_time * CDG_PACKETS_PER_SECOND)
            
            if clear_packet > 50:
                empty_tile = [0] * 12
                packet_offset = 0
                for row in range(CDG_SCREEN_HEIGHT):
                    for col in range(CDG_SCREEN_WIDTH):
                        idx = clear_packet + packet_offset
                        if idx < len(packets):
                            packets[idx] = create_tile_block_packet(0, 0, row, col, empty_tile)
                        packet_offset += 1
                
                print(f"  [Clearing full screen at {clear_start_time:.2f}s]")
        
        display_position = line_idx % 4
        
        # BETTER CLEARING: Clear the line position we're about to use, but with more time buffer
        if line_idx >= 1:
            clear_time = max(0, start_time - 1.0)  # Clear 1 second before
            clear_packet = int(clear_time * CDG_PACKETS_PER_SECOND)
            
            if clear_packet > 50:
                empty_tile = [0] * 12
                packet_offset = 0
                for row in range(line_rows[display_position], line_rows[display_position] + 2):
                    for col in range(CDG_SCREEN_WIDTH):
                        idx = clear_packet + packet_offset
                        if idx < len(packets):
                            packets[idx] = create_tile_block_packet(0, 0, row, col, empty_tile)
                        packet_offset += 1
        
        # Render the full line (centered)
        tiles, text_width_pixels = render_text_to_tiles(line['text'], font_size=12)
        text_width_tiles = len(tiles) // 2
        start_column = (CDG_SCREEN_WIDTH - text_width_tiles) // 2
        
        start_packet = int(start_time * CDG_PACKETS_PER_SECOND)
        base_row = line_rows[display_position]
        
        # Draw the full line
        tile_idx = 0
        for col_offset in range(text_width_tiles):
            col = start_column + col_offset
            if 0 <= col < CDG_SCREEN_WIDTH:
                for row_offset in range(2):
                    if tile_idx < len(tiles):
                        row_in_tile, tile_data = tiles[tile_idx]
                        packet_idx = start_packet + col_offset * 2 + row_offset
                        if packet_idx < len(packets):
                            packets[packet_idx] = create_tile_block_packet(
                                0, 1, base_row + row_offset, col, tile_data
                            )
                        tile_idx += 1
        
        # Back to tile-based highlighting that only affects text
        first_word = line['words'][0] if line['words'] else None
        if first_word:
            word_start_time = first_word['start']
            
            print(f"  [Highlighting first word '{first_word['word']}' with tile-based method]")
            
            # Highlight using the proper highlight color slot
            highlight_start = int(word_start_time * CDG_PACKETS_PER_SECOND)
            
            tile_idx = 0
            for col_offset in range(text_width_tiles):
                col = start_column + col_offset
                if 0 <= col < CDG_SCREEN_WIDTH:
                    for row_offset in range(2):
                        if tile_idx < len(tiles):
                            row_in_tile, tile_data = tiles[tile_idx]
                            packet_idx = highlight_start + col_offset * 2 + row_offset
                            if packet_idx < len(packets):
                                # Use the highlight color we defined
                                packets[packet_idx] = create_tile_block_packet(
                                    0, highlight_color, base_row + row_offset, col, tile_data
                                )
                            tile_idx += 1
            
            # Restore to white after 2 seconds
            unhighlight_start = int((word_start_time + 2.0) * CDG_PACKETS_PER_SECOND)
            
            tile_idx = 0
            for col_offset in range(text_width_tiles):
                col = start_column + col_offset
                if 0 <= col < CDG_SCREEN_WIDTH:
                    for row_offset in range(2):
                        if tile_idx < len(tiles):
                            row_in_tile, tile_data = tiles[tile_idx]
                            packet_idx = unhighlight_start + col_offset * 2 + row_offset
                            if packet_idx < len(packets):
                                # Restore to white
                                packets[packet_idx] = create_tile_block_packet(
                                    0, 1, base_row + row_offset, col, tile_data
                                )
                            tile_idx += 1
    
    print("-" * 60)
    print(f"Total lines added: {len(lines)}\n")
    
    return packets

def write_cdg_file(packets, filename):
    """Write packets to CDG file"""
    with open(filename, 'wb') as f:
        for packet in packets:
            f.write(struct.pack('BB', packet.command & 0x3F, packet.instruction & 0x3F))
            f.write(b'\x00\x00')
            f.write(packet.data[:16])
            f.write(b'\x00\x00\x00\x00')

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
    audio = AudioSegment.from_mp3(input_mp3)
    silence = AudioSegment.silent(duration=silence_duration_ms)
    combined = silence + audio
    combined.export(output_mp3, format="mp3")
    print(f"Created audio with silence: {output_mp3}")
    return output_mp3

def transcribe_mp3(mp3_path, image_path=None):
    """Transcribe MP3 and adjust for silence if needed"""
    silence_offset = 0.0
    modified_mp3 = mp3_path
    
    model = whisper.load_model("base")
    result = model.transcribe(mp3_path, word_timestamps=True)
    
    first_word_time = float('inf')
    for segment in result['segments']:
        if segment['words']:
            first_word_time = min(first_word_time, segment['words'][0]['start'])
    
    if image_path:
        silence_offset = 7.0
        print(f"\nImage specified: Adding 7 seconds of silence for logo display")
        temp_mp3 = mp3_path.rsplit('.', 1)[0] + '_with_silence.mp3'
        modified_mp3 = add_silence_to_mp3(mp3_path, temp_mp3, 7000)
    elif first_word_time < 2.0:
        silence_offset = 2.0
        print(f"\nAudio starts at {first_word_time:.2f}s: Adding 2 seconds of silence")
        temp_mp3 = mp3_path.rsplit('.', 1)[0] + '_with_silence.mp3'
        modified_mp3 = add_silence_to_mp3(mp3_path, temp_mp3, 2000)
    
    song_duration = 0
    for segment in result['segments']:
        if segment['end'] > song_duration:
            song_duration = segment['end']
    
    song_duration += silence_offset
    
    transcript = []
    for segment in result['segments']:
        for word in segment['words']:
            transcript.append({
                'word': word['word'],
                'start': word['start'] + silence_offset,
                'end': word['end'] + silence_offset
            })
    
    return transcript, song_duration, modified_mp3

def create_zip_file(cdg_file, mp3_file, output_zip):
    """Create a ZIP file containing the CDG and MP3"""
    with zipfile.ZipFile(output_zip, 'w', zipfile.ZIP_DEFLATED) as zipf:
        zipf.write(cdg_file, os.path.basename(cdg_file))
        zipf.write(mp3_file, os.path.basename(mp3_file))
    print(f"Created karaoke ZIP: {output_zip}")
    print(f"  - {os.path.basename(cdg_file)}")
    print(f"  - {os.path.basename(mp3_file)}")

def main():
    if len(sys.argv) < 3:
        print("Usage:")
        print("  python cdg.py input.mp3 output.cdg [--json transcript.json] [--image cover.jpg] [--zip output.zip]")
        print("  python cdg.py --from-json transcript.json output.cdg [--image cover.jpg]")
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
        
        print(f"Transcribing {mp3_path} with Whisper...")
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
    
    if final_mp3 and zip_path:
        create_zip_file(output_cdg, final_mp3, zip_path)
    elif final_mp3 and not zip_path:
        auto_zip = output_cdg.rsplit('.', 1)[0] + '.zip'
        create_zip_file(output_cdg, final_mp3, auto_zip)

if __name__ == "__main__":
    main()
