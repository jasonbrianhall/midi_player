#include "equalizer.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Equalizer* equalizer_new(int sample_rate) {
    Equalizer *eq = (Equalizer*)malloc(sizeof(Equalizer));
    if (!eq) return NULL;
    
    memset(eq, 0, sizeof(Equalizer));
    eq->sample_rate = sample_rate;
    eq->enabled = true;
    
    // Initialize with neutral settings (0 dB gain)
    eq->bass_gain_db = 0.0;
    eq->mid_gain_db = 0.0;
    eq->treble_gain_db = 0.0;
    
    // Set up frequency bands
    // Bass: 100 Hz, Mid: 1000 Hz, Treble: 8000 Hz
    calculate_biquad_coefficients(&eq->bands[0], 100.0, 0.0, 0.7, sample_rate);
    calculate_biquad_coefficients(&eq->bands[1], 1000.0, 0.0, 0.7, sample_rate);
    calculate_biquad_coefficients(&eq->bands[2], 8000.0, 0.0, 0.7, sample_rate);
    
    return eq;
}

void equalizer_free(Equalizer *eq) {
    if (eq) {
        free(eq);
    }
}

void equalizer_set_enabled(Equalizer *eq, bool enabled) {
    if (eq) {
        eq->enabled = enabled;
    }
}

void equalizer_set_bass(Equalizer *eq, double gain_db) {
    if (!eq) return;
    
    // Clamp gain to reasonable range
    if (gain_db < -12.0) gain_db = -12.0;
    if (gain_db > 12.0) gain_db = 12.0;
    
    eq->bass_gain_db = gain_db;
    calculate_biquad_coefficients(&eq->bands[0], 100.0, gain_db, 0.7, eq->sample_rate);
}

void equalizer_set_mid(Equalizer *eq, double gain_db) {
    if (!eq) return;
    
    // Clamp gain to reasonable range
    if (gain_db < -12.0) gain_db = -12.0;
    if (gain_db > 12.0) gain_db = 12.0;
    
    eq->mid_gain_db = gain_db;
    calculate_biquad_coefficients(&eq->bands[1], 1000.0, gain_db, 0.7, eq->sample_rate);
}

void equalizer_set_treble(Equalizer *eq, double gain_db) {
    if (!eq) return;
    
    // Clamp gain to reasonable range
    if (gain_db < -12.0) gain_db = -12.0;
    if (gain_db > 12.0) gain_db = 12.0;
    
    eq->treble_gain_db = gain_db;
    calculate_biquad_coefficients(&eq->bands[2], 8000.0, gain_db, 0.7, eq->sample_rate);
}

void equalizer_reset(Equalizer *eq) {
    if (!eq) return;
    
    // Reset all filter states
    for (int i = 0; i < EQ_BANDS; i++) {
        memset(eq->bands[i].x, 0, sizeof(eq->bands[i].x));
        memset(eq->bands[i].y, 0, sizeof(eq->bands[i].y));
    }
}

int16_t equalizer_process_sample(Equalizer *eq, int16_t input) {
    if (!eq || !eq->enabled) return input;
    
    double sample = (double)input / 32768.0;  // Convert to float
    double output = sample;
    
    // Apply each band filter
    for (int i = 0; i < EQ_BANDS; i++) {
        output = biquad_filter(&eq->bands[i], output);
    }
    
    // Convert back to int16_t with clipping
    output *= 32768.0;
    if (output > 32767.0) output = 32767.0;
    if (output < -32768.0) output = -32768.0;
    
    return (int16_t)output;
}

void equalizer_process_buffer(Equalizer *eq, int16_t *buffer, size_t length, int channels) {
    if (!eq || !eq->enabled || !buffer) return;
    
    for (size_t i = 0; i < length; i++) {
        buffer[i] = equalizer_process_sample(eq, buffer[i]);
    }
}

static void calculate_biquad_coefficients(EQBand *band, double frequency, double gain_db, double q, int sample_rate) {
    if (!band) return;
    
    double w = 2.0 * M_PI * frequency / sample_rate;
    double cos_w = cos(w);
    double sin_w = sin(w);
    double A = pow(10.0, gain_db / 40.0);  // Square root of linear gain
    double alpha = sin_w / (2.0 * q);
    
    // Peaking EQ coefficients
    double b0 = 1.0 + alpha * A;
    double b1 = -2.0 * cos_w;
    double b2 = 1.0 - alpha * A;
    double a0 = 1.0 + alpha / A;
    double a1 = -2.0 * cos_w;
    double a2 = 1.0 - alpha / A;
    
    // Normalize coefficients
    band->b[0] = b0 / a0;
    band->b[1] = b1 / a0;
    band->b[2] = b2 / a0;
    band->a[0] = 1.0;
    band->a[1] = a1 / a0;
    band->a[2] = a2 / a0;
    
    band->frequency = frequency;
    band->q_factor = q;
    band->gain = db_to_linear(gain_db);
}

static double db_to_linear(double db) {
    return pow(10.0, db / 20.0);
}

static double biquad_filter(EQBand *band, double input) {
    if (!band) return input;
    
    // Biquad filter implementation (Direct Form II)
    double output = band->b[0] * input + band->b[1] * band->x[0] + band->b[2] * band->x[1]
                   - band->a[1] * band->y[0] - band->a[2] * band->y[1];
    
    // Shift delay line
    band->x[1] = band->x[0];
    band->x[0] = input;
    band->y[1] = band->y[0];
    band->y[0] = output;
    
    return output;
}
