#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>

#define VIS_SAMPLES 512
#define VIS_NUM_BARS 32  // Changed from VIS_BARS to VIS_NUM_BARS
#define VIS_HISTORY_SIZE 64

typedef enum {
    VIS_WAVEFORM,
    VIS_OSCILLOSCOPE,
    VIS_BARS,        // This is now safe since we renamed the macro
    VIS_CIRCLE,
    VIS_VOLUME_METER
} VisualizationType;

typedef struct {
    GtkWidget *drawing_area;
    cairo_surface_t *surface;
    
    // Audio analysis data
    double *audio_samples;
    double *frequency_bands;
    double *peak_data;
    double *history[VIS_HISTORY_SIZE];
    int history_index;
    
    // Simple frequency analysis without FFT
    double *band_filters[VIS_NUM_BARS];  // Use VIS_NUM_BARS
    double *band_values;
    
    // Visualization settings
    VisualizationType type;
    double sensitivity;
    double decay_rate;
    gboolean enabled;
    
    // Color scheme
    double bg_r, bg_g, bg_b;
    double fg_r, fg_g, fg_b;
    double accent_r, accent_g, accent_b;
    
    // Animation
    guint timer_id;
    double rotation;
    double time_offset;
    double volume_level;
    
    // Size
    int width, height;
} Visualizer;

// Function declarations
Visualizer* visualizer_new(void);
void visualizer_free(Visualizer *vis);
void visualizer_set_type(Visualizer *vis, VisualizationType type);
void visualizer_update_audio_data(Visualizer *vis, int16_t *samples, size_t sample_count, int channels);
void visualizer_set_enabled(Visualizer *vis, gboolean enabled);
GtkWidget* create_visualization_controls(Visualizer *vis);

// Internal functions
static gboolean on_visualizer_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_visualizer_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
static gboolean visualizer_timer_callback(gpointer user_data);
static void draw_waveform(Visualizer *vis, cairo_t *cr);
static void draw_oscilloscope(Visualizer *vis, cairo_t *cr);
static void draw_bars(Visualizer *vis, cairo_t *cr);
static void draw_circle(Visualizer *vis, cairo_t *cr);
static void draw_volume_meter(Visualizer *vis, cairo_t *cr);
static void process_audio_simple(Visualizer *vis);
static void init_frequency_bands(Visualizer *vis);

#endif
