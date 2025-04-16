#!/bin/bash
set -e

# Build the midiplayer application
cd /src
echo "Building midiplayer for MSDOS..."
# Add preprocessor define for MSDOS to handle any platform-specific code
g++ midiplayer.cpp -o midiplayer.exe -DMSDOS

echo "Build complete!"
