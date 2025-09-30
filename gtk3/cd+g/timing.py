import struct
import sys
import os

CDG_PACKET_SIZE = 24
CDG_PACKETS_PER_SECOND = 300

def analyze_cdg(filename):
    """Analyze a CDG file and show timing information"""
    
    if not os.path.exists(filename):
        print(f"ERROR: File not found: {filename}")
        return
    
    file_size = os.path.getsize(filename)
    
    if file_size % CDG_PACKET_SIZE != 0:
        print(f"WARNING: File size ({file_size} bytes) is not a multiple of {CDG_PACKET_SIZE}")
        print(f"  This may indicate a corrupted or non-standard CDG file")
    
    num_packets = file_size // CDG_PACKET_SIZE
    duration_seconds = num_packets / CDG_PACKETS_PER_SECOND
    
    print(f"\n{'='*60}")
    print(f"CDG File Analysis: {filename}")
    print(f"{'='*60}")
    print(f"File size:        {file_size:,} bytes")
    print(f"Packet count:     {num_packets:,} packets")
    print(f"Duration:         {duration_seconds:.2f} seconds ({duration_seconds/60:.2f} minutes)")
    print(f"Packet rate:      {CDG_PACKETS_PER_SECOND} packets/second (standard)")
    
    # Read and analyze packet types
    with open(filename, 'rb') as f:
        packet_types = {}
        instructions = {}
        first_non_empty = None
        last_non_empty = None
        
        for i in range(num_packets):
            data = f.read(CDG_PACKET_SIZE)
            if len(data) < CDG_PACKET_SIZE:
                break
            
            command = data[0] & 0x3F
            instruction = data[1] & 0x3F
            
            # Track packet types
            packet_types[command] = packet_types.get(command, 0) + 1
            
            # Track instructions
            if command == 0x09:
                instructions[instruction] = instructions.get(instruction, 0) + 1
                
                # Track first and last non-empty packets
                if instruction != 0:  # Not a no-op
                    if first_non_empty is None:
                        first_non_empty = i
                    last_non_empty = i
    
    print(f"\n{'='*60}")
    print(f"Packet Statistics")
    print(f"{'='*60}")
    
    print(f"\nCommand types:")
    for cmd, count in sorted(packet_types.items()):
        pct = (count / num_packets) * 100
        print(f"  0x{cmd:02X}: {count:,} packets ({pct:.1f}%)")
    
    if 0x09 in packet_types:
        print(f"\nCD+G Instructions (command 0x09):")
        instruction_names = {
            0: "No-op",
            1: "Memory Preset",
            2: "Border Preset",
            6: "Tile Block",
            30: "Load Color Table (Low)",
            31: "Load Color Table (High)",
        }
        
        for inst, count in sorted(instructions.items()):
            pct = (count / num_packets) * 100
            name = instruction_names.get(inst, "Unknown")
            print(f"  {inst:2d} ({name:25s}): {count:,} packets ({pct:.1f}%)")
    
    if first_non_empty is not None and last_non_empty is not None:
        first_time = first_non_empty / CDG_PACKETS_PER_SECOND
        last_time = last_non_empty / CDG_PACKETS_PER_SECOND
        content_duration = (last_non_empty - first_non_empty) / CDG_PACKETS_PER_SECOND
        
        print(f"\n{'='*60}")
        print(f"Content Timing")
        print(f"{'='*60}")
        print(f"First content:    packet {first_non_empty:,} at {first_time:.2f}s")
        print(f"Last content:     packet {last_non_empty:,} at {last_time:.2f}s")
        print(f"Content duration: {content_duration:.2f}s")
        
        if first_time > 0:
            print(f"Lead-in silence:  {first_time:.2f}s")
        
        if last_time < duration_seconds - 1:
            trailing = duration_seconds - last_time
            print(f"Trailing silence: {trailing:.2f}s")
    
    print(f"{'='*60}\n")

def main():
    if len(sys.argv) < 2:
        print("Usage: python cdg_analyzer.py <file.cdg> [file2.cdg ...]")
        print("\nAnalyzes CDG files and displays timing information")
        sys.exit(1)
    
    for filename in sys.argv[1:]:
        analyze_cdg(filename)

if __name__ == "__main__":
    main()
