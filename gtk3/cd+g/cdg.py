import whisper
import cairocffi as cairo
import struct
import sys
import json
import os

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
    """Load colors 0-7 into color table"""
    data = bytearray(16)
    for i in range(8):
        if i < len(colors):
            r, g, b = colors[i]
            # Each color is 12 bits: 4R 4G 4B
            # High byte: X X r r r r g g
            # Low byte:  X X g g b b b b
            high_byte = ((r & 0x0F) << 2) | ((g & 0x0C) >> 2)
            low_byte = ((g & 0x03) << 4) | (b & 0x0F)
            data[i * 2] = high_byte
            data[i * 2 + 1] = low_byte
    return CDGPacket(0x09, 30, data)

def create_tile_block_packet(color0, color1, row, column, tile_data):
    """Create a tile block packet"""
    data = bytearray(16)
    data[0] = color0 & 0x0F
    data[1] = color1 & 0x0F
    data[2] = row & 0x1F
    data[3] = column & 0x3F
    # Copy exactly 12 bytes of tile data
    for i in range(12):
        if i < len(tile_data):
            data[4 + i] = tile_data[i] & 0x3F
    return CDGPacket(0x09, 6, data)

def render_text_to_tiles(text, max_width_tiles=40):
    """Render text and return tile data"""
    width_pixels = max_width_tiles * 6
    height_pixels = 12
    
    surface = cairo.ImageSurface(cairo.FORMAT_A8, width_pixels, height_pixels)
    cr = cairo.Context(surface)
    
    cr.set_source_rgb(1, 1, 1)
    cr.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    cr.set_font_size(10)
    cr.move_to(2, 10)
    cr.show_text(text)
    surface.flush()
    
    data = bytes(surface.get_data())
    stride = surface.get_stride()
    
    # Extract tiles
    tiles = []
    for tile_x in range(max_width_tiles):
        tile_data = []
        for row in range(12):
            bits = 0
            for col in range(6):
                x = tile_x * 6 + col
                y = row
                if x < width_pixels:
                    pixel = data[y * stride + x]
                    if pixel > 128:
                        # Set bit from left to right (bit 5 is leftmost)
                        bits |= (1 << (5 - col))
            tile_data.append(bits)
        tiles.append(tile_data)
    
    return tiles

def group_words_into_lines(transcript, max_words_per_line=6, time_window=3.0):
    """Group words into displayable lines"""
    lines = []
    current_line = []
    current_start = None
    
    for entry in transcript:
        word = entry['word'].strip()
        if not word:
            continue
        
        if current_start is None:
            current_start = entry['start']
        
        current_line.append(word)
        
        if len(current_line) >= max_words_per_line or \
           (entry['start'] - current_start > time_window):
            lines.append({
                'text': ' '.join(current_line),
                'start': current_start,
                'end': entry['end']
            })
            current_line = []
            current_start = None
    
    if current_line:
        lines.append({
            'text': ' '.join(current_line),
            'start': current_start,
            'end': transcript[-1]['end'] if transcript else 0
        })
    
    return lines

def generate_cdg_packets(transcript, song_duration):
    """Generate CDG packets"""
    # Calculate exact number of packets needed
    total_packets = int(song_duration * CDG_PACKETS_PER_SECOND)
    
    print(f"Generating {total_packets} packets for {song_duration:.2f} seconds")
    print(f"({total_packets / CDG_PACKETS_PER_SECOND:.2f} seconds at 75 packets/sec)")
    
    packets = []
    
    # Initialize color table
    # Color 0: BLACK (0,0,0), Color 1: WHITE (15,15,15) 
    colors = [(0, 0, 0), (15, 15, 15), (0, 0, 0), (0, 0, 0), 
              (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0)]
    packets.append(create_load_color_table_low_packet(colors))
    
    # Clear screen to BLACK (color 0) - repeat 16 times as per spec
    for repeat in range(16):
        packets.append(create_memory_preset_packet(0, repeat))
    
    # Set border to BLACK
    packets.append(create_border_preset_packet(0))
    
    # Fill with no-ops to reach total_packets
    for i in range(len(packets), total_packets):
        packets.append(CDGPacket(0x09, 0, bytearray(16)))
    
    # Group words
    print("\nGrouping words into lines...")
    lines = group_words_into_lines(transcript)
    print(f"Created {len(lines)} lines\n")
    
    print("Adding lyrics to CDG:")
    print("-" * 60)
    
    for idx, line in enumerate(lines):
        text = line['text']
        start_time = line['start']
        end_time = line['end']
        start_packet = int(start_time * CDG_PACKETS_PER_SECOND)
        
        print(f"[{start_time:6.2f}s - {end_time:6.2f}s] Line {idx+1:3d}: {text}")
        
        tiles = render_text_to_tiles(text)
        
        # Center the text
        text_width_tiles = len(tiles)
        start_column = (CDG_SCREEN_WIDTH - text_width_tiles) // 2
        center_row = (CDG_SCREEN_HEIGHT - 1) // 2
        
        # Clear center row first with black tiles
        clear_start = max(18, start_packet - 50)
        empty_tile = [0] * 12
        for col in range(CDG_SCREEN_WIDTH):
            packet_idx = clear_start + col
            if packet_idx < len(packets):
                packets[packet_idx] = create_tile_block_packet(0, 0, center_row, col, empty_tile)
        
        # Write text tiles (color0=black, color1=white)
        for i, tile_data in enumerate(tiles):
            col = start_column + i
            if col < CDG_SCREEN_WIDTH:
                packet_idx = start_packet + i
                if packet_idx < len(packets):
                    packets[packet_idx] = create_tile_block_packet(0, 1, center_row, col, tile_data)
    
    print("-" * 60)
    print(f"Total lines added: {len(lines)}\n")
    
    return packets

def write_cdg_file(packets, filename):
    """Write packets to CDG file"""
    with open(filename, 'wb') as f:
        for packet in packets:
            # Command and instruction (mask with 0x3F per spec)
            f.write(struct.pack('BB', packet.command & 0x3F, packet.instruction & 0x3F))
            # Parity Q (2 bytes)
            f.write(b'\x00\x00')
            # Data (16 bytes)
            f.write(packet.data[:16])
            # Parity P (4 bytes)
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

def transcribe_mp3(mp3_path):
    model = whisper.load_model("base")
    result = model.transcribe(mp3_path, word_timestamps=True)
    
    # Get accurate song duration from Whisper
    song_duration = 0
    for segment in result['segments']:
        if segment['end'] > song_duration:
            song_duration = segment['end']
    
    transcript = []
    for segment in result['segments']:
        for word in segment['words']:
            transcript.append({
                'word': word['word'],
                'start': word['start'],
                'end': word['end']
            })
    
    return transcript, song_duration

def main():
    if len(sys.argv) < 3:
        print("Usage:")
        print("  python whisper_to_cdg.py input.mp3 output.cdg [--json transcript.json]")
        print("  python whisper_to_cdg.py --from-json transcript.json output.cdg")
        sys.exit(1)
    
    # Check if loading from JSON
    if sys.argv[1] == '--from-json':
        json_path = sys.argv[2]
        output_cdg = sys.argv[3]
        
        print(f"Loading transcript from JSON: {json_path}")
        transcript, song_duration = load_transcript_json(json_path)
        print(f"Loaded {len(transcript)} words, duration: {song_duration:.2f}s")
        
    else:
        mp3_path = sys.argv[1]
        output_cdg = sys.argv[2]
        
        # Check for optional JSON output
        json_path = None
        if '--json' in sys.argv:
            json_idx = sys.argv.index('--json')
            if json_idx + 1 < len(sys.argv):
                json_path = sys.argv[json_idx + 1]
        
        print(f"Transcribing {mp3_path} with Whisper...")
        transcript, song_duration = transcribe_mp3(mp3_path)
        
        print(f"Song duration: {song_duration:.2f} seconds")
        print(f"Found {len(transcript)} words")
        
        # Save JSON if requested
        if json_path:
            save_transcript_json(transcript, song_duration, json_path)
    
    print(f"Generating CD+G packets...")
    packets = generate_cdg_packets(transcript, song_duration)
    
    print(f"Writing CDG file to {output_cdg}...")
    write_cdg_file(packets, output_cdg)
    
    file_size = len(packets) * CDG_PACKET_SIZE
    expected_duration = len(packets) / CDG_PACKETS_PER_SECOND
    print(f"Done. CDG file size: {file_size} bytes ({len(packets)} packets)")
    print(f"CDG duration: {expected_duration:.2f} seconds")

if __name__ == "__main__":
    main()
