#include "visualization.h"

void init_bouncy_ball_system(Visualizer *vis) {
    vis->bouncy_ball_count = 0;
    vis->bouncy_spawn_timer = 0.0;
    vis->bouncy_beat_threshold = 0.1;
    vis->bouncy_gravity_strength = 400.0;  // Pixels per second squared
    vis->bouncy_size_multiplier = 1.0;
    vis->bouncy_physics_enabled = TRUE;
    
    // Initialize all balls as inactive
    for (int i = 0; i < MAX_BOUNCY_BALLS; i++) {
        vis->bouncy_balls[i].active = FALSE;
        vis->bouncy_balls[i].trail_index = 0;
    }
}

void spawn_bouncy_ball(Visualizer *vis, double intensity, int frequency_band) {
    // Find inactive ball slot
    int slot = -1;
    for (int i = 0; i < MAX_BOUNCY_BALLS; i++) {
        if (!vis->bouncy_balls[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        // Replace oldest ball if no free slots
        slot = 0;
        double oldest_time = vis->bouncy_balls[0].spawn_time;
        for (int i = 1; i < MAX_BOUNCY_BALLS; i++) {
            if (vis->bouncy_balls[i].spawn_time < oldest_time) {
                oldest_time = vis->bouncy_balls[i].spawn_time;
                slot = i;
            }
        }
    }
    
    BouncyBall *ball = &vis->bouncy_balls[slot];
    
    // Initialize ball properties
    ball->x = vis->width * 0.1 + (vis->width * 0.8) * ((double)rand() / RAND_MAX);
    ball->y = vis->height * 0.1;  // Start near top
    
    // Initial velocity based on intensity and frequency
    double speed_factor = 100.0 + intensity * 200.0;
    ball->vx = (((double)rand() / RAND_MAX) - 0.5) * speed_factor;
    ball->vy = ((double)rand() / RAND_MAX) * speed_factor * 0.5;  // Slight downward bias
    
    // Size based on intensity and frequency
    ball->base_radius = 8.0 + intensity * 15.0;
    ball->radius = ball->base_radius;
    
    // Physics properties
    ball->bounce_damping = 0.75 + intensity * 0.2;  // More energetic = less damping
    ball->gravity = vis->bouncy_gravity_strength;
    
    // Visual properties - use random colors with frequency influence
    double base_hue = (double)frequency_band / VIS_FREQUENCY_BARS;  // 0.0 to 1.0
    double random_offset = ((double)rand() / RAND_MAX) * 0.3 - 0.15;  // ±0.15 variation
    ball->hue = fmod(base_hue + random_offset + 1.0, 1.0) * 360.0;  // Convert to 0-360 degrees
    ball->saturation = 0.7 + ((double)rand() / RAND_MAX) * 0.3;  // 0.7 to 1.0
    ball->brightness = 0.6 + intensity * 0.4;
    ball->audio_intensity = intensity;
    
    // Metadata
    ball->frequency_band = frequency_band;
    ball->active = TRUE;
    ball->spawn_time = vis->time_offset;
    ball->last_bounce_time = 0.0;
    ball->energy = intensity;
    
    // Clear trail
    ball->trail_index = 0;
    for (int i = 0; i < 20; i++) {
        ball->trail_x[i] = ball->x;
        ball->trail_y[i] = ball->y;
    }
    
    if (slot >= vis->bouncy_ball_count) {
        vis->bouncy_ball_count = slot + 1;
    }
}

void bouncy_ball_wall_collision(BouncyBall *ball, double width, double height) {
    // Floor collision
    if (ball->y + ball->radius >= height) {
        ball->y = height - ball->radius;
        ball->vy = -ball->vy * ball->bounce_damping;
        ball->last_bounce_time = 0.0;
        
        // Add some random horizontal velocity on floor bounce
        ball->vx += (((double)rand() / RAND_MAX) - 0.5) * 20.0;
    }
    
    // Ceiling collision
    if (ball->y - ball->radius <= 0) {
        ball->y = ball->radius;
        ball->vy = -ball->vy * ball->bounce_damping;
    }
    
    // Left wall collision
    if (ball->x - ball->radius <= 0) {
        ball->x = ball->radius;
        ball->vx = -ball->vx * ball->bounce_damping;
    }
    
    // Right wall collision
    if (ball->x + ball->radius >= width) {
        ball->x = width - ball->radius;
        ball->vx = -ball->vx * ball->bounce_damping;
    }
}

void bouncy_ball_update_trail(BouncyBall *ball) {
    ball->trail_x[ball->trail_index] = ball->x;
    ball->trail_y[ball->trail_index] = ball->y;
    ball->trail_index = (ball->trail_index + 1) % 20;
}

void update_bouncy_balls(Visualizer *vis, double dt) {
    vis->bouncy_spawn_timer += dt;
    
    // Spawn new balls on audio beats
    for (int band = 0; band < VIS_FREQUENCY_BARS; band++) {
        if (vis->frequency_bands[band] > vis->bouncy_beat_threshold && 
            vis->bouncy_spawn_timer > 0.2) {  // Limit spawn rate
            
            spawn_bouncy_ball(vis, vis->frequency_bands[band], band);
            vis->bouncy_spawn_timer = 0.0;
            break;  // Only spawn one ball per frame
        }
    }
    
    // Update existing balls
    for (int i = 0; i < vis->bouncy_ball_count; i++) {
        BouncyBall *ball = &vis->bouncy_balls[i];
        if (!ball->active) continue;
        
        // Update trail
        bouncy_ball_update_trail(ball);
        
        // Apply gravity
        if (vis->bouncy_physics_enabled) {
            ball->vy += ball->gravity * dt;
        }
        
        // Update position
        ball->x += ball->vx * dt;
        ball->y += ball->vy * dt;
        
        // Handle collisions
        bouncy_ball_wall_collision(ball, vis->width, vis->height);
        
        // Update audio responsiveness
        double current_intensity = vis->frequency_bands[ball->frequency_band];
        ball->audio_intensity = fmax(current_intensity, ball->audio_intensity * 0.95);
        
        // Radius pulsing based on audio
        ball->radius = ball->base_radius * (1.0 + ball->audio_intensity * vis->bouncy_size_multiplier);
        
        // Brightness based on speed and audio
        double speed = sqrt(ball->vx * ball->vx + ball->vy * ball->vy);
        ball->brightness = 0.4 + (speed / 300.0) * 0.4 + ball->audio_intensity * 0.3;
        if (ball->brightness > 1.0) ball->brightness = 1.0;
        
        // Energy decay over time
        ball->energy *= 0.998;
        ball->last_bounce_time += dt;
        
        // Remove balls that are too old or have too little energy
        double age = vis->time_offset - ball->spawn_time;
        if (age > 30.0 || ball->energy < 0.01) {
            ball->active = FALSE;
        }
    }
    
    // Compact active balls array
    int write_pos = 0;
    for (int read_pos = 0; read_pos < vis->bouncy_ball_count; read_pos++) {
        if (vis->bouncy_balls[read_pos].active) {
            if (write_pos != read_pos) {
                vis->bouncy_balls[write_pos] = vis->bouncy_balls[read_pos];
            }
            write_pos++;
        }
    }
    vis->bouncy_ball_count = write_pos;
}

void draw_bouncy_balls(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    for (int i = 0; i < vis->bouncy_ball_count; i++) {
        BouncyBall *ball = &vis->bouncy_balls[i];
        if (!ball->active) continue;
        
        // Draw motion trail
        for (int t = 0; t < 20; t++) {
            int trail_idx = (ball->trail_index - t - 1 + 20) % 20;
            double trail_alpha = (20 - t) / 20.0 * 0.3;
            double trail_size = ball->radius * (0.3 + 0.7 * trail_alpha);
            
            double r, g, b;
            hsv_to_rgb(ball->hue, ball->saturation, ball->brightness, &r, &g, &b);
            
            cairo_set_source_rgba(cr, r, g, b, trail_alpha);
            cairo_arc(cr, ball->trail_x[trail_idx], ball->trail_y[trail_idx], 
                     trail_size, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        
        // Draw main ball with gradient
        double r, g, b;
        hsv_to_rgb(ball->hue, ball->saturation, ball->brightness, &r, &g, &b);
        
        // Create radial gradient for 3D effect
        cairo_pattern_t *gradient = cairo_pattern_create_radial(
            ball->x - ball->radius * 0.3, ball->y - ball->radius * 0.3, 0,
            ball->x, ball->y, ball->radius);
        
        cairo_pattern_add_color_stop_rgba(gradient, 0, 
            fmin(r + 0.3, 1.0), fmin(g + 0.3, 1.0), fmin(b + 0.3, 1.0), 0.9);
        cairo_pattern_add_color_stop_rgba(gradient, 1, r, g, b, 0.9);
        
        cairo_set_source(cr, gradient);
        cairo_arc(cr, ball->x, ball->y, ball->radius, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_pattern_destroy(gradient);
        
        // Add highlight for extra 3D effect
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
        cairo_arc(cr, ball->x - ball->radius * 0.3, ball->y - ball->radius * 0.3, 
                 ball->radius * 0.3, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Draw energy indicator ring when ball has high audio response
        if (ball->audio_intensity > 0.3) {
            double ring_radius = ball->radius + 5 + ball->audio_intensity * 10;
            cairo_set_source_rgba(cr, r, g, b, ball->audio_intensity * 0.5);
            cairo_set_line_width(cr, 2.0);
            cairo_arc(cr, ball->x, ball->y, ring_radius, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }
}
