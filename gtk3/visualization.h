#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include "sudoku.h"
#include "generatepuzzle.h"


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

#define MAX_DNA_SEGMENTS 80
#define DNA_BASE_PAIRS 4

#define FOURIER_POINTS 64  // Number of frequency bins to visualize
#define FOURIER_HISTORY 128 // History length for trails

typedef struct {
    double real, imag;     // Complex number components
    double magnitude;      // Magnitude of this frequency bin
    double phase;          // Phase angle
    double x, y;           // Current position on circle
    double trail_x[FOURIER_HISTORY];  // Position history for trails
    double trail_y[FOURIER_HISTORY];  // Position history for trails
    int trail_index;       // Current position in trail buffer
    double hue;            // Color hue for this frequency
    double intensity;      // Visual intensity
} FourierBin;

typedef struct {
    double x, y, z;        // 3D position (z for depth simulation)
    double intensity;      // Audio intensity affecting this segment
    double base_type;      // 0-3 for A,T,G,C base pair types
    double connection_strength; // How strongly the base pairs connect
    double twist_offset;   // Individual twist offset for this segment
    gboolean active;       // Is this segment visible
} DNASegment;

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
    VIS_DNA_HELIX,
    VIS_DNA2_HELIX,
    VIS_SUDOKU_SOLVER,
    VIS_FOURIER_TRANSFORM
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

    // DNA 1
    DNAHelix dna_helix;
    double dna_time_offset;
    double dna_amplitude_multiplier;
    double dna_twist_rate;

    // DNA 2
    DNASegment dna_segments[MAX_DNA_SEGMENTS];
    int dna_segment_count;
    double dna_rotation;
    double dna_twist_speed;
    double dna_spine_offset;
    double dna_base_pulse[DNA_BASE_PAIRS]; // Pulse intensity for each base type    

    // Sudoku visualization
    Sudoku* sudoku_solver;
    PuzzleGenerator* puzzle_generator;
    double sudoku_solve_timer;
    double sudoku_beat_threshold;
    int sudoku_solving_speed;  // milliseconds per move
    bool sudoku_is_solving;
    bool sudoku_puzzle_complete;
    int sudoku_current_step;
    double sudoku_last_beat;
    char sudoku_difficulty[32];
    
    // Visual state
    int sudoku_highlight_x, sudoku_highlight_y;
    double sudoku_highlight_intensity;
    int sudoku_last_changed_x, sudoku_last_changed_y;
    double sudoku_change_glow;
    
    // Beat detection for solving steps
    double sudoku_volume_history[10];
    int sudoku_volume_index;
    int sudoku_last_placed_value;     // The number that was just placed
    double sudoku_completion_glow;    // Celebration effect when puzzle completes    

    // Sudoku pre-solved data
    int sudoku_original_puzzle[9][9];    // The starting puzzle  
    int sudoku_complete_solution[9][9];  // The complete solution
    int sudoku_reveal_order[81][2];      // Order to reveal cells (x,y pairs)
    int sudoku_reveal_index;             // Current position in reveal order
    int sudoku_total_empty_cells;        // How many cells need to be filled
    
    // Background puzzle generation
    Sudoku* background_solver;           // Secondary solver for background generation
    PuzzleGenerator* background_generator; // Background puzzle generator
    bool background_puzzle_ready;        // Is background puzzle ready?
    int background_original_puzzle[9][9]; // Background puzzle state
    int background_complete_solution[9][9]; // Background complete solution
    int background_reveal_order[81][2];   // Background reveal order
    int background_total_empty_cells;    // Background empty cell count
    char background_difficulty[32];      // Background puzzle difficulty
    bool generating_background_puzzle;   // Currently generating?
    
    // Beat synchronization
    double last_real_beat;               // Last detected beat timestamp
    double beat_interval;                // Average time between beats
    double beat_sync_timer;              // Timer for beat synchronization
    bool waiting_for_beat_sync;          // Should wait for next beat?

    // Fourier
    FourierBin fourier_bins[FOURIER_POINTS];
    double fourier_time;
    double fourier_rotation_speed;
    double fourier_zoom;
    int fourier_display_mode; // 0=circular, 1=linear, 2=3D perspective
    double fourier_trail_fade;
    gboolean show_fourier_math; // Show frequency labels and values    


} Visualizer;

// Function declarations
Visualizer* visualizer_new(void);
void visualizer_free(Visualizer *vis);
void visualizer_set_type(Visualizer *vis, VisualizationType type);
void visualizer_update_audio_data(Visualizer *vis, int16_t *samples, size_t sample_count, int channels);
void visualizer_set_enabled(Visualizer *vis, gboolean enabled);
GtkWidget* create_visualization_controls(Visualizer *vis);

// Internal functions
gboolean on_visualizer_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);
gboolean on_visualizer_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
gboolean visualizer_timer_callback(gpointer user_data);
void draw_waveform(Visualizer *vis, cairo_t *cr);
void draw_oscilloscope(Visualizer *vis, cairo_t *cr);
void draw_bars(Visualizer *vis, cairo_t *cr);
void draw_circle(Visualizer *vis, cairo_t *cr);
void draw_volume_meter(Visualizer *vis, cairo_t *cr);
void draw_bubbles(Visualizer *vis, cairo_t *cr);
void process_audio_simple(Visualizer *vis);
void init_frequency_bands(Visualizer *vis);

// Bubble system function declarations
void init_bubble_system(Visualizer *vis);
void spawn_bubble(Visualizer *vis, double intensity);
void create_pop_effect(Visualizer *vis, Bubble *bubble);
void update_bubbles(Visualizer *vis, double dt);


// Matrix
void init_matrix_system(Visualizer *vis);
void spawn_matrix_column(Visualizer *vis, int frequency_band);
void update_matrix(Visualizer *vis, double dt);
void draw_matrix(Visualizer *vis, cairo_t *cr);
const char* get_random_matrix_char(void);

// Fireworks
void init_fireworks_system(Visualizer *vis);
void spawn_firework(Visualizer *vis, double intensity, int frequency_band);
void explode_firework(Visualizer *vis, Firework *firework);
void update_fireworks(Visualizer *vis, double dt);
void draw_fireworks(Visualizer *vis, cairo_t *cr);
void spawn_particle(Visualizer *vis, double x, double y, double vx, double vy, 
                          double r, double g, double b, double life);
double get_hue_for_frequency(int frequency_band);

// DNA
void init_dna_system(Visualizer *vis);
void update_dna_helix(Visualizer *vis, double dt);
void draw_dna_helix(Visualizer *vis, cairo_t *cr);
void init_dna2_system(Visualizer *vis);
void update_dna2_helix(Visualizer *vis, double dt);
void draw_dna2_helix(Visualizer *vis, cairo_t *cr);
void get_base_color(int base_type, double intensity, double *r, double *g, double *b);
void on_visualizer_realize(GtkWidget *widget, gpointer user_data);

// Function declarations (Sudoku)
void init_sudoku_system(Visualizer *vis);
void update_sudoku_solver(Visualizer *vis, double dt);
void draw_sudoku_solver(Visualizer *vis, cairo_t *cr);
void sudoku_generate_new_puzzle(Visualizer *vis);
bool sudoku_detect_beat(Visualizer *vis);
void sudoku_solve_step(Visualizer *vis);
void sudoku_draw_grid(Visualizer *vis, cairo_t *cr);
void sudoku_draw_numbers(Visualizer *vis, cairo_t *cr);
void sudoku_draw_effects(Visualizer *vis, cairo_t *cr);
int sudoku_find_naked_single(Visualizer *vis);
int sudoku_place_single_from_technique(Visualizer *vis, const char* technique);
void sudoku_start_background_generation(Visualizer *vis);
void sudoku_update_background_generation(Visualizer *vis);
bool sudoku_detect_beat_with_tempo(Visualizer *vis);
void sudoku_generate_new_puzzle_from_background(Visualizer *vis);

// Fourier
void update_fourier_mode(Visualizer *vis, double dt);
void draw_fourier_math_overlay(Visualizer *vis, cairo_t *cr, double center_x, double center_y);
void draw_fourier_points(Visualizer *vis, cairo_t *cr);
void draw_fourier_trails(Visualizer *vis, cairo_t *cr);
void draw_fourier_background(Visualizer *vis, cairo_t *cr, double center_x, double center_y);
void draw_fourier_transform(Visualizer *vis, cairo_t *cr);
void update_fourier_transform(Visualizer *vis, double dt);
void init_fourier_system(Visualizer *vis);

void hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b);

#endif
