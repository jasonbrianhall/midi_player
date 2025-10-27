#ifndef MOUSETRAILS_H
#define MOUSETRAILS_H

#include <cairo.h>
#include <math.h>

#define MAX_TRAIL_POINTS 1024
#define MAX_TRAILS 8

typedef struct {
    double x, y;
    double vx, vy;
    double age;
    double max_age;
    double r, g, b;
    double size;
} TrailPoint;

typedef struct {
    TrailPoint points[MAX_TRAIL_POINTS];
    int point_count;
    double hue;
    double creation_time;
} Trail;

typedef struct {
    Trail trails[MAX_TRAILS];
    int active_trail_count;
    
    double mouse_x, mouse_y;
    double last_mouse_x, last_mouse_y;
    double mouse_moved_time;
    
    double time_elapsed;
    int width, height;
    
    // Visualization parameters
    double trail_velocity_damping;
    double trail_gravity;
    double particle_lifetime;
    double spawn_rate;
    
    // Color cycling
    double hue_offset;
    double hue_speed;
} MouseTrailSystem;

// Initialization and cleanup
MouseTrailSystem* mouse_trail_init(int width, int height);
void mouse_trail_cleanup(MouseTrailSystem *sys);

// Event handling
void mouse_trail_on_mouse_move(MouseTrailSystem *sys, double x, double y);
void mouse_trail_on_click(MouseTrailSystem *sys, double x, double y);

// Update and render
void mouse_trail_update(MouseTrailSystem *sys, double dt);
void mouse_trail_draw(MouseTrailSystem *sys, cairo_t *cr);
void mouse_trail_resize(MouseTrailSystem *sys, int width, int height);

// Helper functions
void hsv_to_rgb_mt(double h, double s, double v, double *r, double *g, double *b);
void mouse_trail_spawn_particles(MouseTrailSystem *sys, double x, double y, double vx, double vy);

#endif
