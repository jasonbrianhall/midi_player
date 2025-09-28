#include "visualization.h"

void init_clock_system(Visualizer *vis) {
    vis->swirl_particle_count = 0;
    vis->swirl_spawn_timer = 0.0;
    vis->swirl_beat_threshold = 0.15;
    vis->clock_dot_size = 8.0;
    vis->clock_digit_spacing = 60.0;
    vis->clock_colon_blink_timer = 0.0;
    vis->clock_beat_pulse = 0.0;
    vis->clock_show_seconds = TRUE;
    
    // Initialize all particles as inactive
    for (int i = 0; i < MAX_SWIRL_PARTICLES; i++) {
        vis->swirl_particles[i].active = FALSE;
    }
}

gboolean clock_detect_beat(Visualizer *vis) {
    // Simple beat detection based on volume spike
    double current_volume = vis->volume_level;
    static double last_volume = 0.0;
    static double beat_cooldown = 0.0;
    
    beat_cooldown -= 0.033; // Decrease cooldown
    if (beat_cooldown < 0) beat_cooldown = 0;
    
    gboolean beat = (current_volume > vis->swirl_beat_threshold && 
                     current_volume > last_volume * 1.5 && 
                     beat_cooldown <= 0);
    
    if (beat) {
        beat_cooldown = 0.2; // 200ms cooldown between beats
    }
    
    last_volume = current_volume;
    return beat;
}

void spawn_swirl_particle(Visualizer *vis, double intensity, int frequency_band) {
    // Find inactive particle slot
    int slot = -1;
    for (int i = 0; i < MAX_SWIRL_PARTICLES; i++) {
        if (!vis->swirl_particles[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        // Replace oldest particle
        slot = 0;
    }
    
    SwirlParticle *particle = &vis->swirl_particles[slot];
    
    // Start near the clock center with some randomness
    double spawn_radius = 50.0 + ((double)rand() / RAND_MAX) * 30.0;
    double spawn_angle = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
    
    particle->x = vis->clock_center_x + cos(spawn_angle) * spawn_radius;
    particle->y = vis->clock_center_y + sin(spawn_angle) * spawn_radius;
    particle->angle = spawn_angle;
    particle->radius = spawn_radius;
    
    // Velocity based on frequency band and intensity
    particle->angular_velocity = (1.0 + intensity * 2.0) * (frequency_band % 2 == 0 ? 1.0 : -1.0);
    particle->radial_velocity = intensity * 50.0 * (((double)rand() / RAND_MAX) > 0.5 ? 1.0 : -1.0);
    
    // Visual properties
    particle->hue = (double)frequency_band / VIS_FREQUENCY_BARS * 360.0;
    particle->size = 3.0 + intensity * 5.0;
    particle->intensity = intensity;
    particle->life = 1.0;
    particle->frequency_band = frequency_band;
    particle->active = TRUE;
    
    if (slot >= vis->swirl_particle_count) {
        vis->swirl_particle_count = slot + 1;
    }
}

void update_clock_swirls(Visualizer *vis, double dt) {
    vis->swirl_spawn_timer += dt;
    vis->clock_colon_blink_timer += dt;
    
    // Update beat pulse effect
    if (vis->clock_beat_pulse > 0) {
        vis->clock_beat_pulse -= dt * 3.0; // Fast decay
        if (vis->clock_beat_pulse < 0) vis->clock_beat_pulse = 0;
    }
    
    // Detect beats and spawn particles
    if (clock_detect_beat(vis)) {
        vis->clock_beat_pulse = 1.0;
        
        // Spawn multiple particles on beat
        for (int i = 0; i < 3; i++) {
            int band = rand() % VIS_FREQUENCY_BARS;
            spawn_swirl_particle(vis, vis->frequency_bands[band], band);
        }
    }
    
    // Also spawn particles based on frequency activity
    for (int band = 0; band < VIS_FREQUENCY_BARS; band += 4) {
        if (vis->frequency_bands[band] > vis->swirl_beat_threshold && 
            vis->swirl_spawn_timer > 0.1) {
            spawn_swirl_particle(vis, vis->frequency_bands[band], band);
            vis->swirl_spawn_timer = 0.0;
        }
    }
    
    // Update existing particles
    for (int i = 0; i < vis->swirl_particle_count; i++) {
        SwirlParticle *particle = &vis->swirl_particles[i];
        if (!particle->active) continue;
        
        // Update position
        particle->angle += particle->angular_velocity * dt;
        particle->radius += particle->radial_velocity * dt;
        
        // Update world coordinates
        particle->x = vis->clock_center_x + cos(particle->angle) * particle->radius;
        particle->y = vis->clock_center_y + sin(particle->angle) * particle->radius;
        
        // Apply some randomness to movement
        particle->angular_velocity *= 0.99;
        particle->radial_velocity *= 0.98;
        
        // Decay life
        particle->life -= dt * 0.5; // 2 second lifespan
        
        // Remove particles that are too old or too far
        if (particle->life <= 0 || particle->radius > 300.0 || particle->radius < 5.0) {
            particle->active = FALSE;
        }
    }
    
    // Compact particle array
    int write_pos = 0;
    for (int read_pos = 0; read_pos < vis->swirl_particle_count; read_pos++) {
        if (vis->swirl_particles[read_pos].active) {
            if (write_pos != read_pos) {
                vis->swirl_particles[write_pos] = vis->swirl_particles[read_pos];
            }
            write_pos++;
        }
    }
    vis->swirl_particle_count = write_pos;
}

void draw_digit_matrix(cairo_t *cr, int digit, double x, double y, double dot_size, 
                      double r, double g, double b, double intensity) {
    if (digit < 0 || digit > 9) return;
    
    for (int row = 0; row < DIGIT_HEIGHT; row++) {
        for (int col = 0; col < DIGIT_WIDTH; col++) {
            if (digit_patterns[digit][row][col]) {
                double dot_x = x + col * (dot_size + 2);
                double dot_y = y + row * (dot_size + 2);
                
                // Add some glow effect based on audio intensity
                double glow_size = dot_size * (1.0 + intensity * 0.5);
                
                // Glow
                cairo_set_source_rgba(cr, r, g, b, intensity * 0.3);
                cairo_arc(cr, dot_x, dot_y, glow_size, 0, 2 * M_PI);
                cairo_fill(cr);
                
                // Main dot
                cairo_set_source_rgba(cr, r, g, b, 0.9);
                cairo_arc(cr, dot_x, dot_y, dot_size * 0.7, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
    }
}

void draw_clock_swirls(Visualizer *vis, cairo_t *cr) {
    for (int i = 0; i < vis->swirl_particle_count; i++) {
        SwirlParticle *particle = &vis->swirl_particles[i];
        if (!particle->active) continue;
        
        double r, g, b;
        hsv_to_rgb(particle->hue, 0.8, 0.9, &r, &g, &b);
        
        double alpha = particle->life * 0.8;
        double size = particle->size * particle->life;
        
        // Draw particle with glow
        cairo_set_source_rgba(cr, r, g, b, alpha * 0.3);
        cairo_arc(cr, particle->x, particle->y, size * 2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_set_source_rgba(cr, r, g, b, alpha);
        cairo_arc(cr, particle->x, particle->y, size, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Add bright center
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 0.8);
        cairo_arc(cr, particle->x, particle->y, size * 0.3, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_clock_visualization(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Update center position
    vis->clock_center_x = vis->width / 2.0;
    vis->clock_center_y = vis->height / 2.0;
    
    // Get current time
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    
    // Calculate audio intensity for glow effects
    double audio_intensity = vis->volume_level + vis->clock_beat_pulse * 0.5;
    if (audio_intensity > 1.0) audio_intensity = 1.0;
    
    // Calculate positions for time display
    double total_width = vis->clock_show_seconds ? 
        (8 * (DIGIT_WIDTH * (vis->clock_dot_size + 2))) + (2 * 20) : // HH:MM:SS
        (5 * (DIGIT_WIDTH * (vis->clock_dot_size + 2))) + (1 * 20);  // HH:MM
    
    double start_x = vis->clock_center_x - total_width / 2.0;
    double start_y = vis->clock_center_y - (DIGIT_HEIGHT * (vis->clock_dot_size + 2)) / 2.0;
    
    double current_x = start_x;
    
    // Color scheme - change based on audio
    double base_hue = fmod(vis->time_offset * 10.0, 360.0);
    double r, g, b;
    hsv_to_rgb(base_hue, 0.7, 0.8 + audio_intensity * 0.2, &r, &g, &b);
    
    // Draw hours
    int hours = local_time->tm_hour;
    draw_digit_matrix(cr, hours / 10, current_x, start_y, vis->clock_dot_size, 
                     r, g, b, audio_intensity);
    current_x += DIGIT_WIDTH * (vis->clock_dot_size + 2) + 5;
    
    draw_digit_matrix(cr, hours % 10, current_x, start_y, vis->clock_dot_size, 
                     r, g, b, audio_intensity);
    current_x += DIGIT_WIDTH * (vis->clock_dot_size + 2) + 15;
    
    // Draw colon (blinking based on audio or seconds)
    gboolean show_colon = (fmod(vis->clock_colon_blink_timer, 1.0) < 0.5) || 
                         (audio_intensity > 0.3);
    if (show_colon) {
        double colon_x = current_x;
        double colon_y = start_y + (vis->clock_dot_size + 2);
        
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_arc(cr, colon_x, colon_y, vis->clock_dot_size * 0.5, 0, 2 * M_PI);
        cairo_fill(cr);
        
        colon_y += (vis->clock_dot_size + 2) * 2;
        cairo_arc(cr, colon_x, colon_y, vis->clock_dot_size * 0.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    current_x += 20;
    
    // Draw minutes
    int minutes = local_time->tm_min;
    draw_digit_matrix(cr, minutes / 10, current_x, start_y, vis->clock_dot_size, 
                     r, g, b, audio_intensity);
    current_x += DIGIT_WIDTH * (vis->clock_dot_size + 2) + 5;
    
    draw_digit_matrix(cr, minutes % 10, current_x, start_y, vis->clock_dot_size, 
                     r, g, b, audio_intensity);
    current_x += DIGIT_WIDTH * (vis->clock_dot_size + 2) + 15;
    
    // Draw seconds if enabled
    if (vis->clock_show_seconds) {
        // Second colon
        if (show_colon) {
            double colon_x = current_x;
            double colon_y = start_y + (vis->clock_dot_size + 2);
            
            cairo_set_source_rgba(cr, r, g, b, 0.9);
            cairo_arc(cr, colon_x, colon_y, vis->clock_dot_size * 0.5, 0, 2 * M_PI);
            cairo_fill(cr);
            
            colon_y += (vis->clock_dot_size + 2) * 2;
            cairo_arc(cr, colon_x, colon_y, vis->clock_dot_size * 0.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        current_x += 20;
        
        int seconds = local_time->tm_sec;
        draw_digit_matrix(cr, seconds / 10, current_x, start_y, vis->clock_dot_size, 
                         r, g, b, audio_intensity);
        current_x += DIGIT_WIDTH * (vis->clock_dot_size + 2) + 5;
        
        draw_digit_matrix(cr, seconds % 10, current_x, start_y, vis->clock_dot_size, 
                         r, g, b, audio_intensity);
    }
    
    // Draw the swirling particles
    draw_clock_swirls(vis, cr);
    
    // Draw beat pulse ring around clock
    if (vis->clock_beat_pulse > 0) {
        double ring_radius = 150 + vis->clock_beat_pulse * 50;
        cairo_set_source_rgba(cr, r, g, b, vis->clock_beat_pulse * 0.5);
        cairo_set_line_width(cr, 3.0);
        cairo_arc(cr, vis->clock_center_x, vis->clock_center_y, ring_radius, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
}
