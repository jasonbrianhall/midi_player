#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include "visualization.h"

Visualizer* visualizer_new(void) {
    Visualizer *vis = g_malloc0(sizeof(Visualizer));
    
    // Initialize arrays
    vis->audio_samples = g_malloc0(VIS_SAMPLES * sizeof(double));
    vis->frequency_bands = g_malloc0(VIS_FREQUENCY_BARS * sizeof(double));
    vis->peak_data = g_malloc0(VIS_FREQUENCY_BARS * sizeof(double));
    vis->band_values = g_malloc0(VIS_FREQUENCY_BARS * sizeof(double));
    
    // Initialize history
    for (int i = 0; i < VIS_HISTORY_SIZE; i++) {
        vis->history[i] = g_malloc0(VIS_FREQUENCY_BARS * sizeof(double));
    }
    vis->history_index = 0;
    
    // Initialize simple frequency band analysis
    init_frequency_bands(vis);
    
    // Create drawing area
    vis->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(vis->drawing_area, 400, 200);
    
    // Default settings
    vis->type = VIS_WAVEFORM;
    vis->sensitivity = 1.0;
    vis->decay_rate = 0.95;
    vis->enabled = TRUE;
    vis->volume_level = 0.0;
    
    // Default color scheme (dark theme)
    vis->bg_r = 0.1; vis->bg_g = 0.1; vis->bg_b = 0.1;
    vis->fg_r = 0.0; vis->fg_g = 0.8; vis->fg_b = 0.0;
    vis->accent_r = 0.0; vis->accent_g = 1.0; vis->accent_b = 0.5;
    
    vis->rotation = 0.0;
    vis->time_offset = 0.0;
    
    // Connect signals
    g_signal_connect(vis->drawing_area, "draw", G_CALLBACK(on_visualizer_draw), vis);
    g_signal_connect(vis->drawing_area, "configure-event", G_CALLBACK(on_visualizer_configure), vis);
    
    // Start animation timer
    vis->timer_id = g_timeout_add(33, visualizer_timer_callback, vis); // ~30 FPS
    
    return vis;
}

void visualizer_free(Visualizer *vis) {
    if (!vis) return;
    
    if (vis->timer_id > 0) {
        g_source_remove(vis->timer_id);
    }
    
    g_free(vis->audio_samples);
    g_free(vis->frequency_bands);
    g_free(vis->peak_data);
    g_free(vis->band_values);
    
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        if (vis->band_filters[i]) {
            g_free(vis->band_filters[i]);
        }
    }
    
    for (int i = 0; i < VIS_HISTORY_SIZE; i++) {
        g_free(vis->history[i]);
    }
    
    if (vis->surface) {
        cairo_surface_destroy(vis->surface);
    }
    
    g_free(vis);
}

void visualizer_set_type(Visualizer *vis, VisualizationType type) {
    if (vis) {
        vis->type = type;
        gtk_widget_queue_draw(vis->drawing_area);
    }
}

void visualizer_update_audio_data(Visualizer *vis, int16_t *samples, size_t sample_count, int channels) {
    if (!vis || !vis->enabled || !samples || sample_count == 0) return;
    
    // Convert and downsample audio data
    size_t step = sample_count / VIS_SAMPLES;
    if (step == 0) step = 1;
    
    double rms_sum = 0.0;
    
    for (int i = 0; i < VIS_SAMPLES; i++) {
        double sum = 0.0;
        int count = 0;
        
        // Average multiple samples if needed
        for (size_t j = 0; j < step && (i * step + j) < sample_count; j++) {
            if (channels == 1) {
                sum += samples[i * step + j];
            } else {
                // Average stereo channels
                size_t idx = (i * step + j) * 2;
                if (idx + 1 < sample_count * channels) {
                    sum += (samples[idx] + samples[idx + 1]) / 2.0;
                }
            }
            count++;
        }
        
        if (count > 0) {
            vis->audio_samples[i] = (sum / count) / 32768.0 * vis->sensitivity;
            rms_sum += vis->audio_samples[i] * vis->audio_samples[i];
        } else {
            vis->audio_samples[i] = 0.0;
        }
    }
    
    // Calculate overall volume level (RMS)
    vis->volume_level = sqrt(rms_sum / VIS_SAMPLES);
    
    // Process simple frequency analysis
    process_audio_simple(vis);
}

void visualizer_set_enabled(Visualizer *vis, gboolean enabled) {
    if (vis) {
        vis->enabled = enabled;
        if (!enabled) {
            // Clear visualization data
            memset(vis->frequency_bands, 0, VIS_FREQUENCY_BARS * sizeof(double));
            memset(vis->peak_data, 0, VIS_FREQUENCY_BARS * sizeof(double));
            vis->volume_level = 0.0;
            gtk_widget_queue_draw(vis->drawing_area);
        }
    }
}

static void init_frequency_bands(Visualizer *vis) {
    // Create simple frequency band filters using moving averages
    // This is a basic approximation without FFT
    for (int band = 0; band < VIS_FREQUENCY_BARS; band++) {
        vis->band_filters[band] = g_malloc0(VIS_SAMPLES * sizeof(double));
        
        // Create simple band-pass filter coefficients
        // Lower bands = longer averaging windows (bass)
        // Higher bands = shorter averaging windows (treble)
        int window_size = VIS_SAMPLES / (band + 2);
        if (window_size < 2) window_size = 2;
        if (window_size > VIS_SAMPLES / 4) window_size = VIS_SAMPLES / 4;
        
        for (int i = 0; i < window_size; i++) {
            vis->band_filters[band][i] = 1.0 / window_size;
        }
    }
}

static void process_audio_simple(Visualizer *vis) {
    // Simple frequency band analysis using filtering
    for (int band = 0; band < VIS_FREQUENCY_BARS; band++) {
        double band_energy = 0.0;
        
        // Split the audio samples into frequency-like bands
        // Each band analyzes a different section of the sample array
        int samples_per_band = VIS_SAMPLES / VIS_FREQUENCY_BARS;
        int start_idx = band * samples_per_band;
        int end_idx = start_idx + samples_per_band;
        if (end_idx > VIS_SAMPLES) end_idx = VIS_SAMPLES;
        
        // Calculate RMS energy for this band
        for (int i = start_idx; i < end_idx; i++) {
            band_energy += vis->audio_samples[i] * vis->audio_samples[i];
        }
        
        if (end_idx > start_idx) {
            band_energy = sqrt(band_energy / (end_idx - start_idx));
        }
        
        // Apply some frequency-based weighting
        // Higher frequencies get boosted slightly for visual balance
        double freq_weight = 1.0 + (double)band / VIS_FREQUENCY_BARS * 0.5;
        band_energy *= freq_weight;
        
        // Apply logarithmic scaling for better visual representation
        band_energy = log(1.0 + band_energy * 10.0) / log(11.0);
        
        // Clamp to reasonable range
        if (band_energy > 1.0) band_energy = 1.0;
        if (band_energy < 0.0) band_energy = 0.0;
        
        // Update frequency bands with decay
        vis->frequency_bands[band] = fmax(band_energy, vis->frequency_bands[band] * vis->decay_rate);
        
        // Update peaks
        if (band_energy > vis->peak_data[band]) {
            vis->peak_data[band] = band_energy;
        } else {
            vis->peak_data[band] *= 0.98; // Slower peak decay
        }
    }
    
    // Store in history for effects
    memcpy(vis->history[vis->history_index], vis->frequency_bands, VIS_FREQUENCY_BARS * sizeof(double));
    vis->history_index = (vis->history_index + 1) % VIS_HISTORY_SIZE;
}

static gboolean on_visualizer_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    Visualizer *vis = (Visualizer*)user_data;
    
    if (!vis->enabled) {
        // Draw disabled state
        cairo_set_source_rgb(cr, vis->bg_r, vis->bg_g, vis->bg_b);
        cairo_paint(cr);
        
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 16);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, "Visualization Disabled", &extents);
        cairo_move_to(cr, (vis->width - extents.width) / 2, (vis->height + extents.height) / 2);
        cairo_show_text(cr, "Visualization Disabled");
        return FALSE;
    }
    
    // Clear background
    cairo_set_source_rgb(cr, vis->bg_r, vis->bg_g, vis->bg_b);
    cairo_paint(cr);
    
    // Draw visualization based on type
    switch (vis->type) {
        case VIS_WAVEFORM:
            draw_waveform(vis, cr);
            break;
        case VIS_OSCILLOSCOPE:
            draw_oscilloscope(vis, cr);
            break;
        case VIS_BARS:
            draw_bars(vis, cr);
            break;
        case VIS_CIRCLE:
            draw_circle(vis, cr);
            break;
        case VIS_VOLUME_METER:
            draw_volume_meter(vis, cr);
            break;
        case VIS_BUBBLES:
            draw_bubbles(vis, cr);
            break;
        case VIS_MATRIX:
            draw_matrix(vis, cr);
            break;
    }
    
    return FALSE;
}

static void draw_waveform(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    cairo_set_source_rgba(cr, vis->fg_r, vis->fg_g, vis->fg_b, 0.8);
    cairo_set_line_width(cr, 2.0);
    
    cairo_move_to(cr, 0, vis->height / 2.0);
    
    for (int i = 0; i < VIS_SAMPLES; i++) {
        double x = (double)i * vis->width / (VIS_SAMPLES - 1);
        double y = vis->height / 2.0 + vis->audio_samples[i] * vis->height / 2.5;
        
        // Clamp y to screen bounds
        if (y < 0) y = 0;
        if (y > vis->height) y = vis->height;
        
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    
    cairo_stroke(cr);
    
    // Add a secondary waveform with phase shift for visual interest
    cairo_set_source_rgba(cr, vis->accent_r, vis->accent_g, vis->accent_b, 0.4);
    cairo_set_line_width(cr, 1.0);
    
    cairo_move_to(cr, 0, vis->height / 2.0);
    
    for (int i = 0; i < VIS_SAMPLES; i++) {
        double x = (double)i * vis->width / (VIS_SAMPLES - 1);
        double phase_shifted = i < VIS_SAMPLES - 10 ? vis->audio_samples[i + 10] : 0.0;
        double y = vis->height / 2.0 + phase_shifted * vis->height / 3.0;
        
        // Clamp y to screen bounds
        if (y < 0) y = 0;
        if (y > vis->height) y = vis->height;
        
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    
    cairo_stroke(cr);
}

static void draw_oscilloscope(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Draw grid
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
    cairo_set_line_width(cr, 1.0);
    
    // Horizontal lines
    for (int i = 1; i < 4; i++) {
        double y = vis->height * i / 4.0;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, vis->width, y);
        cairo_stroke(cr);
    }
    
    // Vertical lines
    for (int i = 1; i < 8; i++) {
        double x = vis->width * i / 8.0;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, vis->height);
        cairo_stroke(cr);
    }
    
    // Draw waveform
    cairo_set_source_rgba(cr, vis->accent_r, vis->accent_g, vis->accent_b, 1.0);
    cairo_set_line_width(cr, 2.0);
    
    cairo_move_to(cr, 0, vis->height / 2.0);
    
    for (int i = 0; i < VIS_SAMPLES; i++) {
        double x = (double)i * vis->width / (VIS_SAMPLES - 1);
        double y = vis->height / 2.0 + vis->audio_samples[i] * vis->height / 2.5;
        
        // Clamp y to screen bounds
        if (y < 0) y = 0;
        if (y > vis->height) y = vis->height;
        
        cairo_line_to(cr, x, y);
    }
    
    cairo_stroke(cr);
}

static void draw_bars(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    double bar_width = (double)vis->width / VIS_FREQUENCY_BARS;
    
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        double height = vis->frequency_bands[i] * vis->height * 0.9;
        double x = i * bar_width;
        double y = vis->height - height;
        
        // Color gradient based on frequency band
        double hue = (double)i / VIS_FREQUENCY_BARS;
        double r = vis->fg_r + hue * (vis->accent_r - vis->fg_r);
        double g = vis->fg_g + hue * (vis->accent_g - vis->fg_g);
        double b = vis->fg_b + hue * (vis->accent_b - vis->fg_b);
        
        // Create gradient
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, vis->height, 0, y);
        cairo_pattern_add_color_stop_rgba(gradient, 0, r, g, b, 0.3);
        cairo_pattern_add_color_stop_rgba(gradient, 1, r, g, b, 1.0);
        
        cairo_set_source(cr, gradient);
        cairo_rectangle(cr, x + 1, y, bar_width - 2, height);
        cairo_fill(cr);
        
        cairo_pattern_destroy(gradient);
        
        // Draw peak
        if (vis->peak_data[i] > 0.01) {
            double peak_y = vis->height - (vis->peak_data[i] * vis->height * 0.9);
            cairo_set_source_rgba(cr, vis->accent_r, vis->accent_g, vis->accent_b, 1.0);
            cairo_rectangle(cr, x + 1, peak_y - 2, bar_width - 2, 2);
            cairo_fill(cr);
        }
    }
}

static void draw_circle(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    double center_x = vis->width / 2.0;
    double center_y = vis->height / 2.0;
    double radius = fmin(center_x, center_y) * 0.8;
    
    // Draw circle background
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.5);
    cairo_set_line_width(cr, 2.0);
    cairo_arc(cr, center_x, center_y, radius * 0.3, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Draw frequency bars in circle
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        double angle = (double)i / VIS_FREQUENCY_BARS * 2.0 * M_PI + vis->rotation;
        double magnitude = vis->frequency_bands[i];
        
        double inner_radius = radius * 0.3;
        double outer_radius = inner_radius + magnitude * radius * 0.7;
        
        double inner_x = center_x + cos(angle) * inner_radius;
        double inner_y = center_y + sin(angle) * inner_radius;
        double outer_x = center_x + cos(angle) * outer_radius;
        double outer_y = center_y + sin(angle) * outer_radius;
        
        // Color based on magnitude and position
        double intensity = magnitude;
        double hue = (double)i / VIS_FREQUENCY_BARS;
        
        cairo_set_source_rgba(cr, 
                             vis->fg_r + intensity * hue * (vis->accent_r - vis->fg_r),
                             vis->fg_g + intensity * (vis->accent_g - vis->fg_g),
                             vis->fg_b + intensity * (1.0 - hue) * (vis->accent_b - vis->fg_b),
                             0.8);
        
        cairo_set_line_width(cr, 4.0);
        cairo_move_to(cr, inner_x, inner_y);
        cairo_line_to(cr, outer_x, outer_y);
        cairo_stroke(cr);
    }
    
    // Draw center volume indicator
    double vol_radius = vis->volume_level * radius * 0.2;
    if (vol_radius > 2.0) {
        cairo_set_source_rgba(cr, vis->accent_r, vis->accent_g, vis->accent_b, 0.7);
        cairo_arc(cr, center_x, center_y, vol_radius, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

static void draw_volume_meter(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Draw VU meter style visualization
    double meter_width = vis->width * 0.8;
    double meter_height = vis->height * 0.6;
    double meter_x = (vis->width - meter_width) / 2;
    double meter_y = (vis->height - meter_height) / 2;
    
    // Background
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.8);
    cairo_rectangle(cr, meter_x, meter_y, meter_width, meter_height);
    cairo_fill(cr);
    
    // Draw level bars
    int num_bars = 20;
    double bar_width = meter_width / num_bars;
    double level = vis->volume_level;
    
    for (int i = 0; i < num_bars; i++) {
        double bar_level = (double)i / num_bars;
        double x = meter_x + i * bar_width;
        
        if (level > bar_level) {
            // Color coding: green -> yellow -> red
            double r, g, b;
            if (bar_level < 0.7) {
                r = 0.0; g = 1.0; b = 0.0; // Green
            } else if (bar_level < 0.9) {
                r = 1.0; g = 1.0; b = 0.0; // Yellow
            } else {
                r = 1.0; g = 0.0; b = 0.0; // Red
            }
            
            cairo_set_source_rgba(cr, r, g, b, 0.9);
        } else {
            cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
        }
        
        cairo_rectangle(cr, x + 1, meter_y + 5, bar_width - 2, meter_height - 10);
        cairo_fill(cr);
    }
    
    // Draw peak indicator
    if (level > 0.01) {
        double peak_x = meter_x + level * meter_width;
        cairo_set_source_rgba(cr, vis->accent_r, vis->accent_g, vis->accent_b, 1.0);
        cairo_rectangle(cr, peak_x - 2, meter_y, 4, meter_height);
        cairo_fill(cr);
    }
    
    // Draw frequency bars below
    double freq_y = meter_y + meter_height + 20;
    double freq_height = vis->height - freq_y - 10;
    if (freq_height > 0) {
        double freq_bar_width = meter_width / VIS_FREQUENCY_BARS;
        
        for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
            double height = vis->frequency_bands[i] * freq_height;
            double x = meter_x + i * freq_bar_width;
            double y = freq_y + freq_height - height;
            
            double hue = (double)i / VIS_FREQUENCY_BARS;
            cairo_set_source_rgba(cr, 
                                 vis->fg_r + hue * (vis->accent_r - vis->fg_r),
                                 vis->fg_g + hue * (vis->accent_g - vis->fg_g),
                                 vis->fg_b + hue * (vis->accent_b - vis->fg_b),
                                 0.7);
            
            cairo_rectangle(cr, x + 1, y, freq_bar_width - 2, height);
            cairo_fill(cr);
        }
    }
}

static gboolean on_visualizer_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) {
    Visualizer *vis = (Visualizer*)user_data;
    
    vis->width = gtk_widget_get_allocated_width(widget);
    vis->height = gtk_widget_get_allocated_height(widget);
    
    if (vis->surface) {
        cairo_surface_destroy(vis->surface);
    }
    
    vis->surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget),
                                                    CAIRO_CONTENT_COLOR,
                                                    vis->width, vis->height);
    
    return TRUE;
}

static gboolean visualizer_timer_callback(gpointer user_data) {
    Visualizer *vis = (Visualizer*)user_data;
    
    if (vis->enabled) {
        vis->rotation += 0.02;
        vis->time_offset += 0.1;
        
        if (vis->rotation > 2.0 * M_PI) vis->rotation -= 2.0 * M_PI;
        
        gtk_widget_queue_draw(vis->drawing_area);
    }
    
    return TRUE; // Continue timer
}

// Callback functions for controls
static void on_vis_type_changed(GtkComboBox *combo, gpointer user_data) {
    Visualizer *vis = (Visualizer*)user_data;
    int active = gtk_combo_box_get_active(combo);
    visualizer_set_type(vis, (VisualizationType)active);
}

static void on_vis_enabled_toggled(GtkToggleButton *button, gpointer user_data) {
    Visualizer *vis = (Visualizer*)user_data;
    gboolean enabled = gtk_toggle_button_get_active(button);
    visualizer_set_enabled(vis, enabled);
}

static void on_sensitivity_changed(GtkRange *range, gpointer user_data) {
    Visualizer *vis = (Visualizer*)user_data;
    vis->sensitivity = gtk_range_get_value(range);
}

GtkWidget* create_visualization_controls(Visualizer *vis) {
    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    // Enable/disable checkbox
    GtkWidget *enable_check = gtk_check_button_new_with_label("Enable");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_check), vis->enabled);
    g_signal_connect(enable_check, "toggled", G_CALLBACK(on_vis_enabled_toggled), vis);
    gtk_box_pack_start(GTK_BOX(controls_box), enable_check, FALSE, FALSE, 0);
    
    // Type selection
    GtkWidget *type_label = gtk_label_new("Type:");
    gtk_box_pack_start(GTK_BOX(controls_box), type_label, FALSE, FALSE, 0);
    
    GtkWidget *type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Waveform");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Oscilloscope");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Bars");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Circle");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Volume Meter");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Bubbles");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Matrix Rain");
    gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), vis->type);
    g_signal_connect(type_combo, "changed", G_CALLBACK(on_vis_type_changed), vis);
    gtk_box_pack_start(GTK_BOX(controls_box), type_combo, FALSE, FALSE, 0);
    
    // Sensitivity slider
    GtkWidget *sens_label = gtk_label_new("Sensitivity:");
    gtk_box_pack_start(GTK_BOX(controls_box), sens_label, FALSE, FALSE, 0);
    
    GtkWidget *sens_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 5.0, 0.1);
    gtk_range_set_value(GTK_RANGE(sens_scale), vis->sensitivity);
    gtk_widget_set_size_request(sens_scale, 100, -1);
    g_signal_connect(sens_scale, "value-changed", G_CALLBACK(on_sensitivity_changed), vis);
    gtk_box_pack_start(GTK_BOX(controls_box), sens_scale, FALSE, FALSE, 0);
    
    return controls_box;
}
