#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#define GOL_GRID_WIDTH 80
#define GOL_GRID_HEIGHT 60
#define GOL_MAX_AGE 100

typedef struct {
    int alive;
    int age;           // How long the cell has been alive
    double birth_time; // When it was born (for animation)
    double hue;        // Color based on frequency band that spawned it
} GOLCell;

typedef struct {
    GOLCell cells[GOL_GRID_HEIGHT][GOL_GRID_WIDTH];
    GOLCell next_cells[GOL_GRID_HEIGHT][GOL_GRID_WIDTH];
    double update_timer;
    double update_interval;  // How often to update (based on tempo)
    double spawn_timer;
    int generation;
    
    // Audio-reactive parameters
    double last_spawn_volume;
    double spawn_threshold;
    int spawn_frequency_band;  // Which band triggered last spawn
    
    // Visual effects
    double pulse_intensity;
    double color_shift;
} GameOfLifeSystem;

#endif
