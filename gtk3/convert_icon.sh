#!/bin/bash

# Script to convert icon.png to icon.h with base64 encoding
# Usage: ./convert_icon.sh

INPUT_FILE="icon.png"
OUTPUT_FILE="icon.h"
ICON_SIZE="64x64"

# Check if ImageMagick is installed
if ! command -v convert &> /dev/null; then
    echo "ImageMagick is required but not installed."
    echo "Install it with: sudo apt-get install imagemagick"
    exit 1
fi

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: $INPUT_FILE not found!"
    exit 1
fi

echo "Converting $INPUT_FILE to $OUTPUT_FILE..."

# Resize image to 64x64 and convert to base64
convert "$INPUT_FILE" -resize $ICON_SIZE "$INPUT_FILE.tmp"
BASE64_DATA=$(base64 -w 0 "$INPUT_FILE.tmp")
rm "$INPUT_FILE.tmp"

# Get original image info
ORIGINAL_SIZE=$(identify -format "%wx%h" "$INPUT_FILE")
echo "Original size: $ORIGINAL_SIZE"
echo "Resized to: $ICON_SIZE"

# Create header file
cat > "$OUTPUT_FILE" << EOF
#ifndef ICON_H
#define ICON_H

// Auto-generated icon header file
// Original file: $INPUT_FILE
// Original size: $ORIGINAL_SIZE
// Resized to: $ICON_SIZE
// Generated on: $(date)

#include <glib.h>
#include <gtk/gtk.h>

// Base64 encoded PNG data
static const char icon_base64_data[] = 
"$BASE64_DATA";

// Function to decode base64 and create GdkPixbuf
GdkPixbuf* load_icon_from_base64(void);
void set_window_icon_from_base64(GtkWindow *window);

#endif // ICON_H
EOF

echo "Header file created: $OUTPUT_FILE"
echo "Base64 data length: ${#BASE64_DATA} characters"
