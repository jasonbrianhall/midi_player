#!/bin/bash

# Advanced MP4 to APNG optimization script
# Includes multiple strategies for file size vs quality trade-offs

set -e

if [ -z "$1" ]; then
    cat << 'EOF'
Usage: ./optimize_to_apng_advanced.sh <input.mp4> [options]

Options:
  -o, --output FILE       Output filename (default: icon.apng)
  -c, --colors NUM        Number of colors: 16, 32, 64, 128, 256 (default: 256)
  -s, --size WxH          Output size (default: 64x64)
  -d, --dither TYPE       Dither method: none, ordered, floyd_steinberg (default: floyd_steinberg)
  -f, --fps FPS           Target FPS (default: auto-detect from input)
  --quality               Quality mode: fast, balanced, best (default: balanced)

Examples:
  ./optimize_to_apng_advanced.sh icon.mp4
  ./optimize_to_apng_advanced.sh icon.mp4 -c 128 -o icon_small.apng
  ./optimize_to_apng_advanced.sh icon.mp4 --quality best -c 256
  ./optimize_to_apng_advanced.sh icon.mp4 -s 128x128 --dither none

EOF
    exit 0
fi

# Defaults
INPUT="$1"
OUTPUT="icon.apng"
COLORS=256
SIZE="64x64"
DITHER="floyd_steinberg"
QUALITY="balanced"
FPS=""

# Parse options
while [[ $# -gt 1 ]]; do
    case "$2" in
        -o|--output) OUTPUT="$3"; shift 2;;
        -c|--colors) COLORS="$3"; shift 2;;
        -s|--size) SIZE="$3"; shift 2;;
        -d|--dither) DITHER="$3"; shift 2;;
        -f|--fps) FPS="$3"; shift 2;;
        --quality) QUALITY="$3"; shift 2;;
        *) echo "Unknown option: $2"; exit 1;;
    esac
done

if [ ! -f "$INPUT" ]; then
    echo "✗ Error: File '$INPUT' not found"
    exit 1
fi

# Validate colors
case $COLORS in
    16|32|64|128|256) ;;
    *) echo "✗ Colors must be 16, 32, 64, 128, or 256"; exit 1;;
esac

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         MP4 to APNG Optimization Script                    ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Input:        $INPUT"
echo "Output:       $OUTPUT"
echo "Size:         $SIZE"
echo "Colors:       $COLORS"
echo "Dither:       $DITHER"
echo "Quality mode: $QUALITY"
echo ""

# Get input duration for progress
DURATION=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$INPUT" 2>/dev/null || echo "unknown")

echo "Input duration: ${DURATION}s"
echo ""
echo "Processing... (this may take a minute or two)"
echo ""

# Build ffmpeg filter based on quality mode
case "$QUALITY" in
    fast)
        # Fast mode: minimal filtering
        SCALE_FILTER="scale=$SIZE:flags=bilinear"
        ;;
    balanced)
        # Balanced: good quality, reasonable speed (default)
        SCALE_FILTER="scale=$SIZE:flags=lanczos"
        ;;
    best)
        # Best: maximum quality, slower processing
        SCALE_FILTER="scale=$SIZE:flags=lanczos,format=pix_fmts=rgba"
        ;;
esac

# Build palette filter
PALETTE_FILTER="palettegen=max_colors=$COLORS"

# Build paletteuse filter
if [ "$DITHER" = "none" ]; then
    PALETTEUSE_FILTER="paletteuse=dither=none"
else
    PALETTEUSE_FILTER="paletteuse=dither=$DITHER"
fi

# Build complete filter
FULL_FILTER="$SCALE_FILTER,split[p][v];[p]$PALETTE_FILTER[pal];[v][pal]$PALETTEUSE_FILTER"

# Run ffmpeg with progress
if [ -z "$FPS" ]; then
    ffmpeg -i "$INPUT" \
        -vf "$FULL_FILTER" \
        -plays 0 \
        -progress pipe:1 \
        "$OUTPUT" 2>&1 | grep -E "(frame=|progress=)"
else
    ffmpeg -i "$INPUT" \
        -vf "fps=$FPS,$FULL_FILTER" \
        -plays 0 \
        -progress pipe:1 \
        "$OUTPUT" 2>&1 | grep -E "(frame=|progress=)"
fi

if [ $? -eq 0 ]; then
    SIZE_BYTES=$(stat -f%z "$OUTPUT" 2>/dev/null || stat -c%s "$OUTPUT")
    SIZE_KB=$((SIZE_BYTES / 1024))
    SIZE_B64=$((SIZE_BYTES * 4 / 3 / 1024))
    
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║                     ✓ SUCCESS!                             ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Output file:  $OUTPUT"
    echo "File size:    $SIZE_KB KB"
    echo "Base64 size:  ~$SIZE_B64 KB (when embedded in C code)"
    echo ""
    
    # Show frame info
    echo "Animation details:"
    ffprobe -v error -select_streams v:0 \
        -show_entries stream=width,height,nb_frames,r_frame_rate \
        -of default=noprint_wrappers=1:nokey=1 "$OUTPUT" 2>/dev/null | \
        awk 'NR==1{printf "  Dimensions: %s\n", $0} 
             NR==2{printf "  Frames: %s\n", $0} 
             NR==3{printf "  Frame rate: %s\n", $0}'
    
    echo ""
    echo "To embed in C header:"
    echo "  base64 -w 0 $OUTPUT | sed 's/^/\"/' | sed 's/$/\";/' > icon_base64.txt"
    echo ""
else
    echo ""
    echo "✗ ffmpeg failed!"
    echo ""
    echo "Troubleshooting:"
    echo "  - Ensure ffmpeg is installed: apt install ffmpeg"
    echo "  - Check that ffmpeg has PNG support: ffmpeg -codecs | grep png"
    exit 1
fi
