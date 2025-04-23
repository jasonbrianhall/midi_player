#!/bin/bash
# Build script for MIDI analyzer and debugger tools

echo "Building MIDI analyzer tools..."

# Compile MIDI analyzer
gcc -o midi_analyzer midi_analyzer.c -Wall -Wextra -std=c99 -lm

if [ $? -eq 0 ]; then
    echo "Successfully built midi_analyzer"
else
    echo "Failed to build midi_analyzer"
    exit 1
fi

# Compile debug version of DBOPL MIDI player
gcc -o midi_debug main.c instruments.c midiplayer.c dbopl_wrapper.cpp dbopl.cpp -Wall -lstdc++ -lSDL2 -lm -DDEBUG_MODE

if [ $? -eq 0 ]; then
    echo "Successfully built debug MIDI player"
else
    echo "Failed to build debug MIDI player"
    exit 1
fi

echo "Build complete. Use ./analyze_midi.sh to analyze problematic MIDI files."

chmod +x analyze_midi.sh

echo "Done!"
