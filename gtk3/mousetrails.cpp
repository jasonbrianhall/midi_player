#include "mousetrails.h"
#include <stdlib.h>
#include <string.h>

MouseTrailSystem* mouse_trail_init(int width, int height) {
    MouseTrailSystem *sys = (MouseTrailSystem*)malloc(sizeof(MouseTrailSystem));
    memset(sys, 0, sizeof(MouseTrailSystem));
    
    sys->width = width;
    sys->height = height;
    sys->mouse_x = width / 2.0;
    sys->mouse_y = height / 2.0;
    sys->last_mouse_x = width / 2.0;
    sys->last_mouse_y = height / 2.0;
    
    // Visualization parameters
    sys->trail_velocity_damping = 0.97;
    sys->trail_gravity = 0.15;
    sys->particle_lifetime = 2.0;
    sys->spawn_rate = 5.0;
    sys->hue_speed = 0.8;
    sys->time_elapsed = 0.0;
    
    sys->active_trail_count = 0;
    
    return sys;
}

void mouse_trail_cleanup(MouseTrailSystem *sys) {
    if (sys) {
        free(sys);
    }
}

void mouse_trail_resize(MouseTrailSystem *sys, int width, int height) {
    sys->width = width;
    sys->height = height;
}

void hsv_to_rgb_mt(double h, double s, double v, double *r, double *g, double *b) {
    while (h < 0) h += 1.0;
    while (h > 1.0) h -= 1.0;
    
    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h * 6.0, 2.0) - 1.0));
    double m = v - c;
    
    if (h < 1.0/6.0) { *r = c; *g = x; *b = 0; }
    else if (h < 2.0/6.0) { *r = x; *g = c; *b = 0; }
    else if (h < 3.0/6.0) { *r = 0; *g = c; *b = x; }
    else if (h < 4.0/6.0) { *r = 0; *g = x; *b = c; }
    else if (h < 5.0/6.0) { *r = x; *g = 0; *b = c; }
    else { *r = c; *g = 0; *b = x; }
    
    *r += m;
    *g += m;
    *b += m;
}

void mouse_trail_spawn_particles(MouseTrailSystem *sys, double x, double y, double vx, double vy) {
    // Find an empty trail slot or reuse the oldest one
    int trail_idx = -1;
    
    if (sys->active_trail_count < MAX_TRAILS) {
        trail_idx = sys->active_trail_count;
        sys->active_trail_count++;
    } else {
        // Find the oldest trail
        double oldest_time = sys->trails[0].creation_time;
        trail_idx = 0;
        for (int i = 1; i < MAX_TRAILS; i++) {
            if (sys->trails[i].creation_time < oldest_time) {
                oldest_time = sys->trails[i].creation_time;
                trail_idx = i;
            }
        }
    }
    
    Trail *trail = &sys->trails[trail_idx];
    trail->creation_time = sys->time_elapsed;
    trail->hue = sys->hue_offset;
    trail->point_count = 0;
    
    // Spawn initial particles in a spiral/circular pattern
    int num_particles = 12;
    for (int i = 0; i < num_particles; i++) {
        if (trail->point_count >= MAX_TRAIL_POINTS) break;
        
        double angle = (2.0 * M_PI * i) / num_particles;
        double speed = 60.0 + (rand() % 40) * 1.0;
        double pvel_x = cos(angle) * speed + vx * 0.3;
        double pvel_y = sin(angle) * speed + vy * 0.3;
        
        TrailPoint *point = &trail->points[trail->point_count];
        point->x = x;
        point->y = y;
        point->vx = pvel_x;
        point->vy = pvel_y;
        point->age = 0.0;
        point->max_age = sys->particle_lifetime + (rand() % 100) * 0.01;
        
        double hue_var = trail->hue + (rand() % 100) * 0.01;
        hsv_to_rgb_mt(hue_var, 0.8, 1.0, &point->r, &point->g, &point->b);
        
        point->size = 4.0 + (rand() % 3) * 1.5;
        trail->point_count++;
    }
}

void mouse_trail_on_mouse_move(MouseTrailSystem *sys, double x, double y) {
    sys->last_mouse_x = sys->mouse_x;
    sys->last_mouse_y = sys->mouse_y;
    sys->mouse_x = x;
    sys->mouse_y = y;
    sys->mouse_moved_time = sys->time_elapsed;
    
    // Calculate velocity from mouse movement
    double vx = (x - sys->last_mouse_x);
    double vy = (y - sys->last_mouse_y);
    
    // Spawn particles along the path
    double distance = sqrt(vx * vx + vy * vy);
    if (distance > 2.0) {
        int num_spawn = (int)(distance / 10.0) + 1;
        for (int i = 0; i < num_spawn; i++) {
            double t = num_spawn > 1 ? (double)i / num_spawn : 0.5;
            double spawn_x = sys->last_mouse_x + vx * t;
            double spawn_y = sys->last_mouse_y + vy * t;
            mouse_trail_spawn_particles(sys, spawn_x, spawn_y, vx, vy);
        }
    }
}

void mouse_trail_on_click(MouseTrailSystem *sys, double x, double y) {
    // Spawn a larger burst of particles on click
    for (int i = 0; i < 3; i++) {
        mouse_trail_spawn_particles(sys, x, y, 0, 0);
    }
}

void mouse_trail_update(MouseTrailSystem *sys, double dt) {
    sys->time_elapsed += dt;
    sys->hue_offset += sys->hue_speed * dt * 0.2;
    while (sys->hue_offset > 1.0) sys->hue_offset -= 1.0;
    
    // Update all trails
    for (int t = 0; t < sys->active_trail_count; t++) {
        Trail *trail = &sys->trails[t];
        
        // Update particles
        for (int p = 0; p < trail->point_count; p++) {
            TrailPoint *point = &trail->points[p];
            
            // Apply physics
            point->vy += sys->trail_gravity;
            point->vx *= sys->trail_velocity_damping;
            point->vy *= sys->trail_velocity_damping;
            
            point->x += point->vx * dt;
            point->y += point->vy * dt;
            point->age += dt;
            
            // Wrap around screen edges
            if (point->x < -50) point->x = sys->width + 50;
            if (point->x > sys->width + 50) point->x = -50;
            if (point->y < -50) point->y = sys->height + 50;
            if (point->y > sys->height + 50) point->y = -50;
        }
        
        // Remove dead particles
        int new_count = 0;
        for (int p = 0; p < trail->point_count; p++) {
            if (trail->points[p].age < trail->points[p].max_age) {
                trail->points[new_count] = trail->points[p];
                new_count++;
            }
        }
        trail->point_count = new_count;
    }
    
    // Remove empty trails
    int new_trail_count = 0;
    for (int t = 0; t < sys->active_trail_count; t++) {
        if (sys->trails[t].point_count > 0) {
            sys->trails[new_trail_count] = sys->trails[t];
            new_trail_count++;
        }
    }
    sys->active_trail_count = new_trail_count;
}

void mouse_trail_draw(MouseTrailSystem *sys, cairo_t *cr) {
    // Draw background
    cairo_set_source_rgb(cr, 0.05, 0.05, 0.08);
    cairo_paint(cr);
    
    // Draw all particles
    for (int t = 0; t < sys->active_trail_count; t++) {
        Trail *trail = &sys->trails[t];
        
        for (int p = 0; p < trail->point_count; p++) {
            TrailPoint *point = &trail->points[p];
            
            // Calculate alpha based on age
            double alpha = 1.0 - (point->age / point->max_age);
            alpha = alpha * alpha * 0.9; // Quadratic falloff for smoother fade
            
            // Draw glow effect
            cairo_set_source_rgba(cr, point->r, point->g, point->b, alpha * 0.3);
            cairo_arc(cr, point->x, point->y, point->size * 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Draw core
            cairo_set_source_rgba(cr, point->r, point->g, point->b, alpha);
            cairo_arc(cr, point->x, point->y, point->size * 0.7, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
    
    // Draw cursor position indicator
    double cursor_glow = 0.6 + 0.4 * sin(sys->time_elapsed * 3.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, cursor_glow * 0.5);
    cairo_arc(cr, sys->mouse_x, sys->mouse_y, 12.0, 0, 2 * M_PI);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, cursor_glow);
    cairo_arc(cr, sys->mouse_x, sys->mouse_y, 8.0, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_set_line_width(cr, 2.0);
}
