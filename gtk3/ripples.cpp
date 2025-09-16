#include "visualization.h"

void init_ripple_system(Visualizer *vis) {
    vis->ripple_count = 0;
    vis->ripple_spawn_timer = 0.0;
    vis->last_ripple_volume = 0.0;
    vis->ripple_beat_threshold = 0.1;
    
    // Initialize all ripples as inactive
    for (int i = 0; i < MAX_RIPPLES; i++) {
        vis->ripples[i].active = FALSE;
    }
}

void spawn_ripple(Visualizer *vis, double x, double y, double intensity, int frequency_band) {
    // Find an inactive ripple slot
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!vis->ripples[i].active) {
            Ripple *ripple = &vis->ripples[i];
            
            ripple->center_x = x;
            ripple->center_y = y;
            ripple->radius = 5.0;
            
            // Scale max radius based on screen size and intensity
            double screen_diagonal = sqrt(vis->width * vis->width + vis->height * vis->height);
            ripple->max_radius = screen_diagonal * 0.6 * (0.3 + intensity); // Bigger ripples
            
            // Slower speed for better visibility
            ripple->speed = 30.0 + intensity * 100.0 + frequency_band * 3.0;
            
            ripple->intensity = intensity;
            ripple->life = 1.0;
            
            // Color based on frequency band
            ripple->hue = (double)frequency_band / VIS_FREQUENCY_BARS;

            // MUCH thicker rings for visibility
            ripple->thickness = 4.0 + intensity * 12.0;
            
            ripple->frequency_band = frequency_band;
            ripple->active = TRUE;
            
            break;
        }
    }
}

void update_ripples(Visualizer *vis, double dt) {
    // Spawn new ripples based on audio
    vis->ripple_spawn_timer += dt;
    
    // Check for volume spikes (beats)
    double volume_diff = vis->volume_level - vis->last_ripple_volume;
    vis->last_ripple_volume = vis->volume_level;
    
    // Spawn ripples on volume spikes or timer
    gboolean should_spawn = FALSE;
    int spawn_frequency_band = 0;
    double spawn_intensity = vis->volume_level;
    
    // Method 1: Volume spike detection
    if (volume_diff > vis->ripple_beat_threshold && vis->volume_level > 0.1) {
        should_spawn = TRUE;
        spawn_intensity = volume_diff * 2.0;
        
        // Find the strongest frequency band
        double max_band = 0.0;
        for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
            if (vis->frequency_bands[i] > max_band) {
                max_band = vis->frequency_bands[i];
                spawn_frequency_band = i;
            }
        }
    }
    
    // Method 2: Timer-based spawning for consistent ripples
    if (vis->ripple_spawn_timer > 0.3 && vis->volume_level > 0.05) {
        should_spawn = TRUE;
        vis->ripple_spawn_timer = 0.0;
        spawn_intensity = vis->volume_level;
        spawn_frequency_band = (int)(vis->time_offset * 3) % VIS_FREQUENCY_BARS;
    }
    
    if (should_spawn) {
        // Spawn ripple at center, or offset based on stereo/frequency data
        double center_x = vis->width / 2.0;
        double center_y = vis->height / 2.0;
        
        // Add some variation to spawn position based on frequency
        double offset_x = sin(vis->time_offset + spawn_frequency_band * 0.5) * vis->width * 0.1;
        double offset_y = cos(vis->time_offset + spawn_frequency_band * 0.3) * vis->height * 0.1;
        
        spawn_ripple(vis, center_x + offset_x, center_y + offset_y, 
                    spawn_intensity, spawn_frequency_band);
        
        vis->ripple_spawn_timer = 0.0;
    }
    
    // Update existing ripples
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (vis->ripples[i].active) {
            Ripple *ripple = &vis->ripples[i];
            
            // Expand radius
            ripple->radius += ripple->speed * dt;
            
            // Calculate life based on radius
            if (ripple->radius >= ripple->max_radius) {
                ripple->life -= dt * 2.0; // Fade out when max radius reached
            } else {
                // Life decreases slowly during expansion
                ripple->life -= dt * 0.5;
            }
            
            // Remove dead ripples
            if (ripple->life <= 0.0) {
                ripple->active = FALSE;
            }
        }
    }
}

void draw_ripples(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Draw all active ripples
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (vis->ripples[i].active) {
            Ripple *ripple = &vis->ripples[i];
            
            // Convert hue to RGB with MUCH brighter colors
            double r, g, b;
            // Use fixed high brightness (0.9) instead of low intensity
            hsv_to_rgb(ripple->hue, 0.8, 0.9, &r, &g, &b);
            
            // Boost alpha significantly - use life directly, not scaled down
            double alpha = ripple->life;
            if (alpha > 0.1) {
                
                // Draw main ripple ring - MUCH more visible
                cairo_set_source_rgba(cr, r, g, b, alpha);
                cairo_set_line_width(cr, ripple->thickness);
                cairo_arc(cr, ripple->center_x, ripple->center_y, ripple->radius, 0, 2 * M_PI);
                cairo_stroke(cr);
                
                // Draw inner glow effect - brighter
                cairo_set_source_rgba(cr, r, g, b, alpha * 0.6);
                cairo_set_line_width(cr, ripple->thickness * 0.5);
                cairo_arc(cr, ripple->center_x, ripple->center_y, 
                         ripple->radius - ripple->thickness * 0.5, 0, 2 * M_PI);
                cairo_stroke(cr);
                
                // Draw outer fade - more visible
                cairo_set_source_rgba(cr, r, g, b, alpha * 0.4);
                cairo_set_line_width(cr, ripple->thickness * 2.0);
                cairo_arc(cr, ripple->center_x, ripple->center_y, 
                         ripple->radius + ripple->thickness, 0, 2 * M_PI);
                cairo_stroke(cr);
                
                // Add center dot when ripple starts (for debugging)
                if (ripple->radius < 20.0) {
                    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
                    cairo_arc(cr, ripple->center_x, ripple->center_y, 3.0, 0, 2 * M_PI);
                    cairo_fill(cr);
                }
            }
        }
    }
}
