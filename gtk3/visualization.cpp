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

static void init_bubble_system(Visualizer *vis) {
    vis->bubble_count = 0;
    vis->pop_effect_count = 0;
    vis->bubble_spawn_timer = 0.0;
    vis->last_peak_level = 0.0;
    
    // Initialize all bubbles as inactive
    for (int i = 0; i < MAX_BUBBLES; i++) {
        vis->bubbles[i].active = FALSE;
    }
    
    // Initialize all pop effects as inactive
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        vis->pop_effects[i].active = FALSE;
    }
}

static void spawn_bubble(Visualizer *vis, double intensity) {
    if (vis->bubble_count >= MAX_BUBBLES) return;
    
    // Find inactive bubble slot
    int slot = -1;
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!vis->bubbles[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return;
    
    Bubble *bubble = &vis->bubbles[slot];
    
    // Random spawn position (avoid edges)
    bubble->x = 50 + (double)rand() / RAND_MAX * (vis->width - 100);
    bubble->y = 50 + (double)rand() / RAND_MAX * (vis->height - 100);
    
    // Size based on audio intensity
    bubble->max_radius = 15 + intensity * 40;
    bubble->radius = 2.0;
    
    // Random gentle movement
    double angle = (double)rand() / RAND_MAX * 2.0 * M_PI;
    double speed = 10 + intensity * 20;
    bubble->velocity_x = cos(angle) * speed;
    bubble->velocity_y = sin(angle) * speed;
    
    bubble->life = 1.0;
    bubble->birth_time = vis->time_offset;
    bubble->intensity = intensity;
    bubble->active = TRUE;
    
    vis->bubble_count++;
}

static void create_pop_effect(Visualizer *vis, Bubble *bubble) {
    // Find inactive pop effect slot
    int slot = -1;
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        if (!vis->pop_effects[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return;
    
    PopEffect *effect = &vis->pop_effects[slot];
    
    effect->x = bubble->x;
    effect->y = bubble->y;
    effect->radius = 0.0;
    effect->max_radius = bubble->max_radius * 1.5;
    effect->life = 1.0;
    effect->intensity = bubble->intensity;
    effect->active = TRUE;
    
    vis->pop_effect_count++;
}

static void update_bubbles(Visualizer *vis, double dt) {
    // Update spawn timer
    vis->bubble_spawn_timer += dt;
    
    // Spawn new bubbles based on audio peaks
    double current_peak = 0.0;
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        if (vis->frequency_bands[i] > current_peak) {
            current_peak = vis->frequency_bands[i];
        }
    }
    
    // Spawn bubble on significant audio events
    if (current_peak > 0.3 && current_peak > vis->last_peak_level * 1.2 && 
        vis->bubble_spawn_timer > 0.1) {
        spawn_bubble(vis, current_peak);
        vis->bubble_spawn_timer = 0.0;
    }
    
    // Also spawn bubbles periodically if there's consistent audio
    if (vis->volume_level > 0.2 && vis->bubble_spawn_timer > 0.5) {
        spawn_bubble(vis, vis->volume_level * 0.7);
        vis->bubble_spawn_timer = 0.0;
    }
    
    vis->last_peak_level = current_peak;
    
    // Update existing bubbles
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!vis->bubbles[i].active) continue;
        
        Bubble *bubble = &vis->bubbles[i];
        
        // Update position
        bubble->x += bubble->velocity_x * dt;
        bubble->y += bubble->velocity_y * dt;
        
        // Bounce off walls
        if (bubble->x <= bubble->radius || bubble->x >= vis->width - bubble->radius) {
            bubble->velocity_x *= -0.8;
            bubble->x = fmax(bubble->radius, fmin(vis->width - bubble->radius, bubble->x));
        }
        if (bubble->y <= bubble->radius || bubble->y >= vis->height - bubble->radius) {
            bubble->velocity_y *= -0.8;
            bubble->y = fmax(bubble->radius, fmin(vis->height - bubble->radius, bubble->y));
        }
        
        // Grow bubble
        if (bubble->radius < bubble->max_radius) {
            bubble->radius += (bubble->max_radius - bubble->radius) * dt * 2.0;
        }
        
        // Apply gravity and drag
        bubble->velocity_y += 50.0 * dt; // Gentle gravity
        bubble->velocity_x *= (1.0 - dt * 0.5); // Air resistance
        bubble->velocity_y *= (1.0 - dt * 0.3);
        
        // Age the bubble
        bubble->life -= dt * 0.3; // Slower aging
        
        // Pop bubble if it's too old or reached max size
        if (bubble->life <= 0.0 || bubble->radius >= bubble->max_radius * 0.95) {
            create_pop_effect(vis, bubble);
            bubble->active = FALSE;
            vis->bubble_count--;
        }
    }
    
    // Update pop effects
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        if (!vis->pop_effects[i].active) continue;
        
        PopEffect *effect = &vis->pop_effects[i];
        
        // Expand ring
        effect->radius += (effect->max_radius - effect->radius) * dt * 8.0;
        
        // Age effect
        effect->life -= dt * 3.0; // Fast fade
        
        // Remove if expired
        if (effect->life <= 0.0) {
            effect->active = FALSE;
            vis->pop_effect_count--;
        }
    }
}

static void draw_bubbles(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Update bubble system
    update_bubbles(vis, 0.033); // ~30 FPS
    
    // Draw pop effects first (behind bubbles)
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        if (!vis->pop_effects[i].active) continue;
        
        PopEffect *effect = &vis->pop_effects[i];
        
        // Multiple expanding rings for dramatic effect
        for (int ring = 0; ring < 3; ring++) {
            double ring_radius = effect->radius - ring * 8;
            if (ring_radius <= 0) continue;
            
            double alpha = effect->life * (0.6 - ring * 0.1);
            if (alpha <= 0) continue;
            
            // Purple gradient for pop effect
            cairo_set_source_rgba(cr, 
                                 0.6 + effect->intensity * 0.4,  // Purple-pink
                                 0.2, 
                                 0.8 + effect->intensity * 0.2, 
                                 alpha);
            
            cairo_set_line_width(cr, 3.0 - ring);
            cairo_arc(cr, effect->x, effect->y, ring_radius, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
        
        // Central flash
        if (effect->life > 0.8) {
            double flash_alpha = (effect->life - 0.8) * 5.0;
            cairo_set_source_rgba(cr, 1.0, 0.8, 1.0, flash_alpha);
            cairo_arc(cr, effect->x, effect->y, effect->intensity * 15, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
    
    // Draw bubbles
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!vis->bubbles[i].active) continue;
        
        Bubble *bubble = &vis->bubbles[i];
        
        // Calculate bubble appearance
        double age_factor = 1.0 - bubble->life;
        double pulse = sin(vis->time_offset * 4.0 + bubble->birth_time) * 0.1 + 1.0;
        double draw_radius = bubble->radius * pulse;
        
        // Audio-reactive color intensity
        double audio_intensity = 0.0;
        for (int j = 0; j < VIS_FREQUENCY_BARS; j++) {
            audio_intensity += vis->frequency_bands[j];
        }
        audio_intensity /= VIS_FREQUENCY_BARS;
        
        // Purple color variations
        double r = 0.4 + bubble->intensity * 0.4 + audio_intensity * 0.2;
        double g = 0.1 + audio_intensity * 0.3;
        double b = 0.7 + bubble->intensity * 0.3;
        double alpha = bubble->life * 0.8;
        
        // Draw bubble with gradient
        cairo_pattern_t *gradient = cairo_pattern_create_radial(
            bubble->x - draw_radius * 0.3, bubble->y - draw_radius * 0.3, 0,
            bubble->x, bubble->y, draw_radius);
        
        cairo_pattern_add_color_stop_rgba(gradient, 0, r + 0.3, g + 0.3, b + 0.2, alpha);
        cairo_pattern_add_color_stop_rgba(gradient, 0.7, r, g, b, alpha * 0.8);
        cairo_pattern_add_color_stop_rgba(gradient, 1.0, r * 0.5, g * 0.5, b * 0.8, alpha * 0.3);
        
        cairo_set_source(cr, gradient);
        cairo_arc(cr, bubble->x, bubble->y, draw_radius, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(gradient);
        
        // Highlight
        cairo_set_source_rgba(cr, 1.0, 0.9, 1.0, alpha * 0.6);
        cairo_arc(cr, bubble->x - draw_radius * 0.4, bubble->y - draw_radius * 0.4, 
                  draw_radius * 0.2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Outer ring for larger bubbles
        if (draw_radius > 25) {
            cairo_set_source_rgba(cr, r, g, b, alpha * 0.4);
            cairo_set_line_width(cr, 2.0);
            cairo_arc(cr, bubble->x, bubble->y, draw_radius + 3, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }
    
    // Draw floating particles for ambiance
    for (int i = 0; i < 20; i++) {
        double x = fmod(vis->time_offset * 10 + i * 37, vis->width);
        double y = fmod(vis->time_offset * 5 + i * 73, vis->height);
        double size = 1 + sin(vis->time_offset + i) * 0.5;
        
        cairo_set_source_rgba(cr, 0.7, 0.3, 0.9, 0.3);
        cairo_arc(cr, x, y, size, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
}
