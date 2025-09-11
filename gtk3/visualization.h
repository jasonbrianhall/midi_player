#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>

#define VIS_SAMPLES 512
#define VIS_FREQUENCY_BARS 32
#define VIS_HISTORY_SIZE 64
#define MAX_BUBBLES 100
#define MAX_POP_EFFECTS 50

#define MAX_MATRIX_COLUMNS 60
#define MAX_CHARS_PER_COLUMN 30

#define MAX_FIREWORKS 20
#define MAX_PARTICLES_PER_FIREWORK 50
#define MAX_TOTAL_PARTICLES 1000

#define DNA_POINTS 200
#define DNA_STRANDS 2

typedef struct {
    double amplitude;      // How far from center line
    double frequency;      // How fast the helix twists
    double phase_offset;   // Phase difference between strands
    double flow_speed;     // How fast the helix flows horizontally
    double strand_colors[DNA_STRANDS][3]; // RGB colors for each strand
} DNAHelix;

typedef struct {
    double x, y;           // Position
    double vx, vy;         // Velocity
    double ax, ay;         // Acceleration (gravity)
    double life;           // Life remaining (1.0 to 0.0)
    double max_life;       // Initial life span
    double size;           // Particle size
    double r, g, b;        // Color
    double brightness;     // Brightness multiplier
    gboolean active;       // Is particle alive
    int trail_length;      // Length of particle trail
    double trail_x[10], trail_y[10]; // Trail positions
} FireworkParticle;

typedef struct {
    double x, y;           // Launch position
    double target_x, target_y; // Explosion point
    double vx, vy;         // Velocity
    double life;           // Life until explosion
    double explosion_size; // Size of explosion
    double hue;            // Base color hue
    int particle_count;    // Number of particles to spawn
    gboolean exploded;     // Has it exploded yet
    gboolean active;       // Is firework active
    int frequency_band;    // Which frequency band triggered it
} Firework;

typedef struct {
    int x;                    // Column x position
    double y;                 // Current y position (can be fractional)
    double speed;             // Fall speed
    int length;               // Length of the trail
    double intensity;         // Brightness multiplier
    const char* chars[MAX_CHARS_PER_COLUMN]; // Array of string pointers instead of chars
    double char_ages[MAX_CHARS_PER_COLUMN]; // Age of each character (for fading)
    gboolean active;          // Is this column active
    int frequency_band;       // Which frequency band controls this column
    bool power_mode;
    int flash_intensity;
    int wave_offset;
    int glitch_timer;
} MatrixColumn;

typedef enum {
    VIS_WAVEFORM,
    VIS_OSCILLOSCOPE,
    VIS_BARS,
    VIS_CIRCLE,
    VIS_VOLUME_METER,
    VIS_BUBBLES,
    VIS_MATRIX,
    VIS_FIREWORKS,
    VIS_DNA_HELIX
} VisualizationType;

// Define bubble and pop effect structs BEFORE Visualizer struct
typedef struct {
    double x, y;           // Position
    double radius;         // Current radius
    double max_radius;     // Maximum radius before popping
    double velocity_x, velocity_y;  // Movement
    double life;           // Life remaining (0.0 - 1.0)
    double birth_time;     // When bubble was created
    double intensity;      // Audio intensity that created it
    gboolean active;       // Is this bubble alive?
} Bubble;

typedef struct {
    double x, y;           // Position where pop occurred
    double radius;         // Expanding ring radius
    double max_radius;     // Final ring radius
    double life;           // Effect life remaining
    double intensity;      // Original bubble intensity
    gboolean active;       // Is effect active?
} PopEffect;

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
    double *band_filters[VIS_FREQUENCY_BARS];  // Use renamed constant
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
    
    // Bubble system data
    Bubble bubbles[MAX_BUBBLES];
    PopEffect pop_effects[MAX_POP_EFFECTS];
    int bubble_count;
    int pop_effect_count;
    double bubble_spawn_timer;
    double last_peak_level;
    
    // Matrix
    MatrixColumn matrix_columns[MAX_MATRIX_COLUMNS];
    int matrix_column_count;
    double matrix_spawn_timer;
    int matrix_char_size;

    Firework fireworks[MAX_FIREWORKS];
    FireworkParticle particles[MAX_TOTAL_PARTICLES];
    int firework_count;
    int particle_count;
    double firework_spawn_timer;
    double last_beat_time;
    double beat_threshold;
    double gravity;

    DNAHelix dna_helix;
    double dna_time_offset;
    double dna_amplitude_multiplier;
    double dna_twist_rate;

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
void draw_bubbles(Visualizer *vis, cairo_t *cr);
static void process_audio_simple(Visualizer *vis);
static void init_frequency_bands(Visualizer *vis);

// Bubble system function declarations
static void init_bubble_system(Visualizer *vis);
static void spawn_bubble(Visualizer *vis, double intensity);
static void create_pop_effect(Visualizer *vis, Bubble *bubble);
static void update_bubbles(Visualizer *vis, double dt);

// Matrix
static void init_matrix_system(Visualizer *vis);
static void spawn_matrix_column(Visualizer *vis, int frequency_band);
static void update_matrix(Visualizer *vis, double dt);
void draw_matrix(Visualizer *vis, cairo_t *cr);
static const char* get_random_matrix_char(void);

// Fireworks
void init_fireworks_system(Visualizer *vis);
static void spawn_firework(Visualizer *vis, double intensity, int frequency_band);
static void explode_firework(Visualizer *vis, Firework *firework);
void update_fireworks(Visualizer *vis, double dt);
void draw_fireworks(Visualizer *vis, cairo_t *cr);
static void spawn_particle(Visualizer *vis, double x, double y, double vx, double vy, 
                          double r, double g, double b, double life);
static double get_hue_for_frequency(int frequency_band);
static void hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b);

void init_dna_system(Visualizer *vis);
void update_dna_helix(Visualizer *vis, double dt);
void draw_dna_helix(Visualizer *vis, cairo_t *cr);

#endif
