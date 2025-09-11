#ifndef MATRIX_H
#define MATRIX_H

#include "visualization.h"

void init_fireworks_system(Visualizer *vis) {
    vis->firework_count = 0;
    vis->particle_count = 0;
    vis->firework_spawn_timer = 0.0;
    vis->last_beat_time = 0.0;
    vis->beat_threshold = 0.25; // Lowered for better responsiveness
    vis->gravity = 150.0; // Reduced gravity for more graceful particles
    
    // Initialize all fireworks as inactive
    for (int i = 0; i < MAX_FIREWORKS; i++) {
        vis->fireworks[i].active = FALSE;
        vis->fireworks[i].exploded = FALSE;
    }
    
    // Initialize all particles as inactive
    for (int i = 0; i < MAX_TOTAL_PARTICLES; i++) {
        vis->particles[i].active = FALSE;
    }
}

// Convert HSV to RGB color space
static void hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b) {
    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    
    if (h >= 0 && h < 60) {
        *r = c; *g = x; *b = 0;
    } else if (h >= 60 && h < 120) {
        *r = x; *g = c; *b = 0;
    } else if (h >= 120 && h < 180) {
        *r = 0; *g = c; *b = x;
    } else if (h >= 180 && h < 240) {
        *r = 0; *g = x; *b = c;
    } else if (h >= 240 && h < 300) {
        *r = x; *g = 0; *b = c;
    } else {
        *r = c; *g = 0; *b = x;
    }
    
    *r += m; *g += m; *b += m;
}

// Get color hue based on frequency band
static double get_hue_for_frequency(int frequency_band) {
    // Map frequency bands to visually distinct colors
    double normalized = (double)frequency_band / (VIS_FREQUENCY_BARS - 1);
    
    if (normalized < 0.33) {
        // Bass: Red to Orange (0° to 30°)
        return normalized * 3.0 * 30.0;
    } else if (normalized < 0.66) {
        // Mid: Green to Cyan (120° to 180°)
        return 120.0 + (normalized - 0.33) * 3.0 * 60.0;
    } else {
        // Treble: Blue to Purple (240° to 300°)
        return 240.0 + (normalized - 0.66) * 3.0 * 60.0;
    }
}


// Spawn a new firework
static void spawn_firework(Visualizer *vis, double intensity, int frequency_band) {
    // Find an inactive firework slot
    int slot = -1;
    for (int i = 0; i < MAX_FIREWORKS; i++) {
        if (!vis->fireworks[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return; // No available slots
    
    Firework *fw = &vis->fireworks[slot];
    
    // Launch from bottom of screen with some variation
    fw->x = vis->width * (0.15 + g_random_double() * 0.7);
    fw->y = vis->height + 10; // Start slightly below screen
    
    // Target height based on intensity - higher intensity = higher explosion
    double target_height_factor = 0.3 + (intensity * 0.5); // 0.3 to 0.8 of screen height
    fw->target_x = vis->width * (0.1 + g_random_double() * 0.8);
    fw->target_y = vis->height * (0.1 + target_height_factor * 0.7);
    
    // Calculate velocity to reach target (accounting for gravity)
    double time_to_target = 1.2 + g_random_double() * 0.8; // 1.2-2.0 seconds
    fw->vx = (fw->target_x - fw->x) / time_to_target;
    fw->vy = (fw->target_y - fw->y) / time_to_target - 0.5 * vis->gravity * time_to_target;
    
    fw->life = time_to_target;
    fw->explosion_size = 40.0 + intensity * 120.0; // Size based on audio intensity
    fw->hue = get_hue_for_frequency(frequency_band);
    fw->particle_count = 15 + (int)(intensity * 40); // 15-55 particles
    fw->frequency_band = frequency_band;
    fw->exploded = FALSE;
    fw->active = TRUE;
    
    vis->firework_count++;
}

// Spawn a particle
static void spawn_particle(Visualizer *vis, double x, double y, double vx, double vy, 
                          double r, double g, double b, double life) {
    // Find an inactive particle slot
    int slot = -1;
    for (int i = 0; i < MAX_TOTAL_PARTICLES; i++) {
        if (!vis->particles[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return; // No available slots
    
    FireworkParticle *p = &vis->particles[slot];
    
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->ax = 0.0;
    p->ay = vis->gravity; // Gravity pulls down
    p->life = life;
    p->max_life = life;
    p->size = 2.0 + g_random_double() * 3.0;
    p->r = r;
    p->g = g;
    p->b = b;
    p->brightness = 1.0;
    p->active = TRUE;
    p->trail_length = 0;
    
    vis->particle_count++;
}

// Explode a firework into particles
static void explode_firework(Visualizer *vis, Firework *firework) {
    if (firework->exploded) return;
    
    double hue = firework->hue;
    
    // Create explosion particles
    for (int i = 0; i < firework->particle_count; i++) {
        // Random direction
        double angle = g_random_double() * 2.0 * M_PI;
        double speed = 50.0 + g_random_double() * 150.0;
        
        double vx = cos(angle) * speed;
        double vy = sin(angle) * speed;
        
        // Vary the color slightly
        double color_variance = 30.0; // Degrees of hue variance
        double particle_hue = hue + (g_random_double() - 0.5) * color_variance;
        if (particle_hue < 0) particle_hue += 360.0;
        if (particle_hue >= 360) particle_hue -= 360.0;
        
        double r, g, b;
        hsv_to_rgb(particle_hue, 0.8 + g_random_double() * 0.2, 1.0, &r, &g, &b);
        
        double life = 1.0 + g_random_double() * 2.0; // 1-3 seconds
        
        spawn_particle(vis, firework->target_x, firework->target_y, vx, vy, r, g, b, life);
    }
    
    // Special effects for high intensity explosions
    if (firework->explosion_size > 100.0) {
        // Create a ring of particles
        int ring_particles = 16;
        for (int i = 0; i < ring_particles; i++) {
            double angle = (double)i / ring_particles * 2.0 * M_PI;
            double speed = 100.0;
            
            double vx = cos(angle) * speed;
            double vy = sin(angle) * speed;
            
            double r, g, b;
            hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
            
            spawn_particle(vis, firework->target_x, firework->target_y, vx, vy, r, g, b, 2.5);
        }
    }
    
    firework->exploded = TRUE;
}

// Update fireworks system
void update_fireworks(Visualizer *vis, double dt) {
    // Update existing fireworks
    for (int i = 0; i < MAX_FIREWORKS; i++) {
        Firework *fw = &vis->fireworks[i];
        if (!fw->active) continue;
        
        if (!fw->exploded) {
            // Update position
            fw->x += fw->vx * dt;
            fw->y += fw->vy * dt;
            
            // Update life
            fw->life -= dt;
            
            // Check if it's time to explode
            if (fw->life <= 0.0 || 
                (fw->vy < 0 && fw->y <= fw->target_y)) { // Fixed: vy should be negative when going up
                explode_firework(vis, fw);
            }
        } else {
            // Firework has exploded, wait a bit then deactivate
            fw->life -= dt;
            if (fw->life <= -1.0) {
                fw->active = FALSE;
                vis->firework_count--;
            }
        }
    }
    
    // Update particles
    for (int i = 0; i < MAX_TOTAL_PARTICLES; i++) {
        FireworkParticle *p = &vis->particles[i];
        if (!p->active) continue;
        
        // Update position with physics
        p->vx += p->ax * dt;
        p->vy += p->ay * dt;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        
        // Update trail
        if (p->trail_length < 10) {
            p->trail_length++;
        }
        for (int j = p->trail_length - 1; j > 0; j--) {
            p->trail_x[j] = p->trail_x[j-1];
            p->trail_y[j] = p->trail_y[j-1];
        }
        p->trail_x[0] = p->x;
        p->trail_y[0] = p->y;
        
        // Update life and brightness
        p->life -= dt;
        p->brightness = p->life / p->max_life;
        
        // Air resistance
        p->vx *= 0.995; // Slightly less air resistance for better visuals
        p->vy *= 0.995;
        
        // Remove dead particles
        if (p->life <= 0.0 || p->y > vis->height + 100) {
            p->active = FALSE;
            vis->particle_count--;
        }
    }
    
    // IMPROVED MUSIC-RESPONSIVE FIREWORKS SPAWNING
    vis->firework_spawn_timer += dt;
    
    // Calculate overall energy and detect sudden increases
    double total_energy = 0.0;
    double bass_energy = 0.0;
    double mid_energy = 0.0;
    double treble_energy = 0.0;
    
    // Split frequency bands into bass, mid, treble
    int bass_bands = VIS_FREQUENCY_BARS / 3;
    int mid_bands = VIS_FREQUENCY_BARS / 3;
    int treble_bands = VIS_FREQUENCY_BARS - bass_bands - mid_bands;
    
    for (int i = 0; i < bass_bands; i++) {
        bass_energy += vis->frequency_bands[i];
    }
    for (int i = bass_bands; i < bass_bands + mid_bands; i++) {
        mid_energy += vis->frequency_bands[i];
    }
    for (int i = bass_bands + mid_bands; i < VIS_FREQUENCY_BARS; i++) {
        treble_energy += vis->frequency_bands[i];
    }
    
    bass_energy /= bass_bands;
    mid_energy /= mid_bands;
    treble_energy /= treble_bands;
    total_energy = (bass_energy + mid_energy + treble_energy) / 3.0;
    
    // Detect beats by comparing current energy to recent average
    static double energy_history[10] = {0};
    static int history_index = 0;
    
    // Calculate average energy over recent frames
    double avg_energy = 0.0;
    for (int i = 0; i < 10; i++) {
        avg_energy += energy_history[i];
    }
    avg_energy /= 10.0;
    
    // Store current energy in history
    energy_history[history_index] = total_energy;
    history_index = (history_index + 1) % 10;
    
    // Beat detection: current energy is significantly higher than recent average
    double beat_multiplier = total_energy / (avg_energy + 0.01); // Avoid division by zero
    
    // Spawn fireworks based on different conditions:
    
    // 1. Strong bass beats (kick drums)
    if (bass_energy > 0.3 && beat_multiplier > 2.0 && 
        vis->firework_spawn_timer > 0.3 && 
        vis->firework_count < MAX_FIREWORKS - 1) {
        
        spawn_firework(vis, bass_energy, 0); // Use low frequency band for color
        vis->firework_spawn_timer = 0.0;
    }
    
    // 2. Mid-range peaks (snares, vocals)
    else if (mid_energy > 0.4 && beat_multiplier > 1.8 && 
             vis->firework_spawn_timer > 0.4 && 
             vis->firework_count < MAX_FIREWORKS - 1) {
        
        spawn_firework(vis, mid_energy, VIS_FREQUENCY_BARS / 2);
        vis->firework_spawn_timer = 0.0;
    }
    
    // 3. Treble peaks (cymbals, hi-hats)
    else if (treble_energy > 0.35 && beat_multiplier > 1.6 && 
             vis->firework_spawn_timer > 0.6 && 
             vis->firework_count < MAX_FIREWORKS - 1) {
        
        spawn_firework(vis, treble_energy, VIS_FREQUENCY_BARS - 1);
        vis->firework_spawn_timer = 0.0;
    }
    
    // 4. Overall volume spikes (chorus, drops)
    else if (total_energy > 0.5 && beat_multiplier > 2.5 && 
             vis->firework_spawn_timer > 0.2 && 
             vis->firework_count < MAX_FIREWORKS - 2) {
        
        // Spawn multiple fireworks for big moments!
        spawn_firework(vis, total_energy, bass_bands);
        spawn_firework(vis, total_energy * 0.8, bass_bands + mid_bands / 2);
        vis->firework_spawn_timer = 0.0;
    }
    
    // 5. Continuous firing for very energetic music
    else if (total_energy > 0.6 && vis->firework_spawn_timer > 1.0 && 
             vis->firework_count < MAX_FIREWORKS - 3) {
        
        // Find the most active frequency band
        int most_active_band = 0;
        double max_energy = 0.0;
        for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
            if (vis->frequency_bands[i] > max_energy) {
                max_energy = vis->frequency_bands[i];
                most_active_band = i;
            }
        }
        
        if (max_energy > 0.4) {
            spawn_firework(vis, max_energy, most_active_band);
            vis->firework_spawn_timer = 0.0;
        }
    }
}


// Draw the fireworks visualization
void draw_fireworks(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Draw a gradient background (night sky)
    cairo_pattern_t *bg_gradient = cairo_pattern_create_linear(0, 0, 0, vis->height);
    cairo_pattern_add_color_stop_rgba(bg_gradient, 0, 0.05, 0.05, 0.2, 1.0); // Dark blue at top
    cairo_pattern_add_color_stop_rgba(bg_gradient, 1, 0.02, 0.02, 0.05, 1.0); // Almost black at bottom
    
    cairo_set_source(cr, bg_gradient);
    cairo_paint(cr);
    cairo_pattern_destroy(bg_gradient);
    
    // Draw particles with trails
    for (int i = 0; i < MAX_TOTAL_PARTICLES; i++) {
        FireworkParticle *p = &vis->particles[i];
        if (!p->active) continue;
        
        // Draw particle trail
        if (p->trail_length > 1) {
            cairo_set_line_width(cr, 1.0);
            for (int j = 1; j < p->trail_length; j++) {
                double trail_alpha = p->brightness * (1.0 - (double)j / p->trail_length) * 0.5;
                cairo_set_source_rgba(cr, p->r, p->g, p->b, trail_alpha);
                cairo_move_to(cr, p->trail_x[j-1], p->trail_y[j-1]);
                cairo_line_to(cr, p->trail_x[j], p->trail_y[j]);
                cairo_stroke(cr);
            }
        }
        
        // Draw the particle itself
        double alpha = p->brightness;
        cairo_set_source_rgba(cr, p->r, p->g, p->b, alpha);
        
        // Create a glowing effect
        cairo_pattern_t *glow = cairo_pattern_create_radial(p->x, p->y, 0, p->x, p->y, p->size * 2);
        cairo_pattern_add_color_stop_rgba(glow, 0, p->r, p->g, p->b, alpha);
        cairo_pattern_add_color_stop_rgba(glow, 1, p->r, p->g, p->b, 0);
        
        cairo_set_source(cr, glow);
        cairo_arc(cr, p->x, p->y, p->size * 2, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(glow);
        
        // Bright center
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 0.8);
        cairo_arc(cr, p->x, p->y, p->size * 0.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // Draw launching fireworks
    for (int i = 0; i < MAX_FIREWORKS; i++) {
        Firework *fw = &vis->fireworks[i];
        if (!fw->active || fw->exploded) continue;
        
        // Draw the rising firework
        double r, g, b;
        hsv_to_rgb(fw->hue, 0.8, 1.0, &r, &g, &b);
        
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_arc(cr, fw->x, fw->y, 3.0, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Draw a small trail
        cairo_set_source_rgba(cr, r, g, b, 0.4);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, fw->x, fw->y);
        cairo_line_to(cr, fw->x - fw->vx * 0.1, fw->y - fw->vy * 0.1);
        cairo_stroke(cr);
    }
}

#endif
