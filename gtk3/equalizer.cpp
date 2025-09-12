#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "equalizer.h"
#include "audio_player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern AudioPlayer *player;

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

void on_eq_enabled_toggled(GtkToggleButton *button, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    bool enabled = gtk_toggle_button_get_active(button);
    equalizer_set_enabled(player->equalizer, enabled);
    
    // Enable/disable the EQ controls
    gtk_widget_set_sensitive(player->bass_scale, enabled);
    gtk_widget_set_sensitive(player->mid_scale, enabled);
    gtk_widget_set_sensitive(player->treble_scale, enabled);
    gtk_widget_set_sensitive(player->eq_reset_button, enabled);
}

void on_bass_changed(GtkRange *range, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    double value = gtk_range_get_value(range);
    equalizer_set_bass(player->equalizer, value);
}

void on_mid_changed(GtkRange *range, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    double value = gtk_range_get_value(range);
    equalizer_set_mid(player->equalizer, value);
}

void on_treble_changed(GtkRange *range, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    double value = gtk_range_get_value(range);
    equalizer_set_treble(player->equalizer, value);
}

void on_eq_reset_clicked(GtkButton *button, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Reset all sliders to 0
    gtk_range_set_value(GTK_RANGE(player->bass_scale), 0.0);
    gtk_range_set_value(GTK_RANGE(player->mid_scale), 0.0);
    gtk_range_set_value(GTK_RANGE(player->treble_scale), 0.0);
    
    // Reset equalizer
    equalizer_set_bass(player->equalizer, 0.0);
    equalizer_set_mid(player->equalizer, 0.0);
    equalizer_set_treble(player->equalizer, 0.0);
    equalizer_reset(player->equalizer);
}

// Function to create equalizer controls:
GtkWidget* create_equalizer_controls(AudioPlayer *player) {
    // Create frame without label to remove blue header
    player->eq_frame = gtk_frame_new(NULL);

    GtkWidget *eq_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(player->eq_frame), eq_vbox);
    gtk_container_set_border_width(GTK_CONTAINER(eq_vbox), 5);

    // Detect DPI scale factor
    GtkWidget *toplevel = gtk_widget_get_toplevel(player->window);
    int scale_factor = gtk_widget_get_scale_factor(toplevel);

    if (GTK_IS_WINDOW(toplevel) && gtk_widget_get_realized(toplevel)) {
        scale_factor = get_scale_factor(toplevel);
    }

    // DPI-scaled dimensions for vertical sliders
    int slider_width = (int)(150 / scale_factor);
    int slider_height = (int)(150 / scale_factor);

    // EQ controls laid out horizontally
    GtkWidget *eq_controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(eq_vbox), eq_controls_box, TRUE, TRUE, 0);

    // Bass control
    GtkWidget *bass_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *bass_label = gtk_label_new("Bass (100Hz)");
    if (scale_factor<=1.0) {
        player->bass_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -12.0, 12.0, 0.5);
    } else {
        player->bass_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -12.0, 12.0, 0.5);
    }
    
    gtk_range_set_value(GTK_RANGE(player->bass_scale), 0.0);
    gtk_scale_set_draw_value(GTK_SCALE(player->bass_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(player->bass_scale), GTK_POS_BOTTOM);
    gtk_widget_set_size_request(player->bass_scale, slider_width, slider_height);
    gtk_range_set_inverted(GTK_RANGE(player->bass_scale), FALSE);
    g_signal_connect(player->bass_scale, "value-changed", G_CALLBACK(on_bass_changed), player);
    gtk_box_pack_start(GTK_BOX(bass_vbox), bass_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bass_vbox), player->bass_scale, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(eq_controls_box), bass_vbox, TRUE, TRUE, 0);

    // Mid control
    GtkWidget *mid_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *mid_label = gtk_label_new("Mid (1KHz)");
    if (scale_factor<=1.0) {
        player->mid_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -12.0, 12.0, 0.5);
    } else {
        player->mid_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -12.0, 12.0, 0.5);
    }    
    gtk_range_set_value(GTK_RANGE(player->mid_scale), 0.0);
    gtk_scale_set_draw_value(GTK_SCALE(player->mid_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(player->mid_scale), GTK_POS_BOTTOM);
    gtk_widget_set_size_request(player->mid_scale, slider_width, slider_height);
    gtk_range_set_inverted(GTK_RANGE(player->mid_scale), FALSE);
    g_signal_connect(player->mid_scale, "value-changed", G_CALLBACK(on_mid_changed), player);
    gtk_box_pack_start(GTK_BOX(mid_vbox), mid_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mid_vbox), player->mid_scale, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(eq_controls_box), mid_vbox, TRUE, TRUE, 0);

    // Treble control
    GtkWidget *treble_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *treble_label = gtk_label_new("Treble (8KHz)");
    if (scale_factor<=1.0) {
        player->treble_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -12.0, 12.0, 0.5);
    } else {
        player->treble_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -12.0, 12.0, 0.5);
    }
    gtk_range_set_value(GTK_RANGE(player->treble_scale), 0.0);
    gtk_scale_set_draw_value(GTK_SCALE(player->treble_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(player->treble_scale), GTK_POS_BOTTOM);
    gtk_widget_set_size_request(player->treble_scale, slider_width, slider_height);
    gtk_range_set_inverted(GTK_RANGE(player->treble_scale), FALSE);
    g_signal_connect(player->treble_scale, "value-changed", G_CALLBACK(on_treble_changed), player);
    gtk_box_pack_start(GTK_BOX(treble_vbox), treble_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(treble_vbox), player->treble_scale, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(eq_controls_box), treble_vbox, TRUE, TRUE, 0);

    // Reset button
    player->eq_reset_button = gtk_button_new_with_label("Reset");
    g_signal_connect(player->eq_reset_button, "clicked", G_CALLBACK(on_eq_reset_clicked), player);
    gtk_box_pack_start(GTK_BOX(eq_vbox), player->eq_reset_button, FALSE, FALSE, 0);

    return player->eq_frame;
}





