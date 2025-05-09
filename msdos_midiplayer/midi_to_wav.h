/*
 * MIDI to WAV Converter Header
 * Provides function for converting MIDI files to WAV format
 */
#ifndef MIDI_TO_WAV_H
#define MIDI_TO_WAV_H

#include <stdbool.h>

/* Function to convert MIDI file to WAV format */
bool convert_midi_to_wav(const char* midi_filename, const char* wav_filename, int volume);

#endif /* MIDI_TO_WAV_H */
