#ifndef BUBBLES_H
#define BUBBLES_H

#include "visualization.h"

void init_bubble_system(Visualizer *vis) {
    vis->bubble_count = 0;
    vis->pop_effect_count = 0;
    vis->bubble_spawn_timer = 0.0;
    vis->last_peak_level = 0.0;
    
    // Initialize all bubbles as inactive
    for (int i = 0; i < MAX_BUBBLES; i++) {
        vis->bubbles[i].active = FALSE;
    }
    
    // Initialize all pop effects as inactive
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        vis->pop_effects[i].active = FALSE;
    }
}

void spawn_bubble(Visualizer *vis, double intensity) {
    if (vis->bubble_count >= MAX_BUBBLES) return;
    
    // Find inactive bubble slot
    int slot = -1;
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!vis->bubbles[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return;
    
    Bubble *bubble = &vis->bubbles[slot];
    
    // Random spawn position (avoid edges)
    bubble->x = 50 + (double)rand() / RAND_MAX * (vis->width - 100);
    bubble->y = 50 + (double)rand() / RAND_MAX * (vis->height - 100);
    
    // Size based on audio intensity
    bubble->max_radius = 15 + intensity * 40;
    bubble->radius = 2.0;
    
    // Random gentle movement
    double angle = (double)rand() / RAND_MAX * 2.0 * M_PI;
    double speed = 10 + intensity * 20;
    bubble->velocity_x = cos(angle) * speed;
    bubble->velocity_y = sin(angle) * speed;
    
    bubble->life = 1.0;
    bubble->birth_time = vis->time_offset;
    bubble->intensity = intensity;
    bubble->active = TRUE;
    
    vis->bubble_count++;
}

void create_pop_effect(Visualizer *vis, Bubble *bubble) {
    // Find inactive pop effect slot
    int slot = -1;
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        if (!vis->pop_effects[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return;
    
    PopEffect *effect = &vis->pop_effects[slot];
    
    effect->x = bubble->x;
    effect->y = bubble->y;
    effect->radius = 0.0;
    effect->max_radius = bubble->max_radius * 1.5;
    effect->life = 1.0;
    effect->intensity = bubble->intensity;
    effect->active = TRUE;
    
    vis->pop_effect_count++;
}

void update_bubbles(Visualizer *vis, double dt) {
    // Update spawn timer
    vis->bubble_spawn_timer += dt;
    
    // Spawn new bubbles based on audio peaks
    double current_peak = 0.0;
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        if (vis->frequency_bands[i] > current_peak) {
            current_peak = vis->frequency_bands[i];
        }
    }
    
    // Spawn bubble on significant audio events
    if (current_peak > 0.3 && current_peak > vis->last_peak_level * 1.2 && 
        vis->bubble_spawn_timer > 0.1) {
        spawn_bubble(vis, current_peak);
        vis->bubble_spawn_timer = 0.0;
    }
    
    // Also spawn bubbles periodically if there's consistent audio
    if (vis->volume_level > 0.2 && vis->bubble_spawn_timer > 0.5) {
        spawn_bubble(vis, vis->volume_level * 0.7);
        vis->bubble_spawn_timer = 0.0;
    }
    
    vis->last_peak_level = current_peak;
    
    // Update existing bubbles
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!vis->bubbles[i].active) continue;
        
        Bubble *bubble = &vis->bubbles[i];
        
        // Update position
        bubble->x += bubble->velocity_x * dt;
        bubble->y += bubble->velocity_y * dt;
        
        // Bounce off walls
        if (bubble->x <= bubble->radius || bubble->x >= vis->width - bubble->radius) {
            bubble->velocity_x *= -0.8;
            bubble->x = fmax(bubble->radius, fmin(vis->width - bubble->radius, bubble->x));
        }
        if (bubble->y <= bubble->radius || bubble->y >= vis->height - bubble->radius) {
            bubble->velocity_y *= -0.8;
            bubble->y = fmax(bubble->radius, fmin(vis->height - bubble->radius, bubble->y));
        }
        
        // Grow bubble
        if (bubble->radius < bubble->max_radius) {
            bubble->radius += (bubble->max_radius - bubble->radius) * dt * 2.0;
        }
        
        // Apply gravity and drag
        bubble->velocity_y += 50.0 * dt; // Gentle gravity
        bubble->velocity_x *= (1.0 - dt * 0.5); // Air resistance
        bubble->velocity_y *= (1.0 - dt * 0.3);
        
        // Age the bubble
        bubble->life -= dt * 0.3; // Slower aging
        
        // Pop bubble if it's too old or reached max size
        if (bubble->life <= 0.0 || bubble->radius >= bubble->max_radius * 0.95) {
            create_pop_effect(vis, bubble);
            bubble->active = FALSE;
            vis->bubble_count--;
        }
    }
    
    // Update pop effects
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        if (!vis->pop_effects[i].active) continue;
        
        PopEffect *effect = &vis->pop_effects[i];
        
        // Expand ring
        effect->radius += (effect->max_radius - effect->radius) * dt * 8.0;
        
        // Age effect
        effect->life -= dt * 3.0; // Fast fade
        
        // Remove if expired
        if (effect->life <= 0.0) {
            effect->active = FALSE;
            vis->pop_effect_count--;
        }
    }
}

void draw_bubbles(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Update bubble system
    update_bubbles(vis, 0.033); // ~30 FPS
    
    // Draw pop effects first (behind bubbles)
    for (int i = 0; i < MAX_POP_EFFECTS; i++) {
        if (!vis->pop_effects[i].active) continue;
        
        PopEffect *effect = &vis->pop_effects[i];
        
        // Multiple expanding rings for dramatic effect
        for (int ring = 0; ring < 3; ring++) {
            double ring_radius = effect->radius - ring * 8;
            if (ring_radius <= 0) continue;
            
            double alpha = effect->life * (0.6 - ring * 0.1);
            if (alpha <= 0) continue;
            
            // Purple gradient for pop effect
            cairo_set_source_rgba(cr, 
                                 0.6 + effect->intensity * 0.4,  // Purple-pink
                                 0.2, 
                                 0.8 + effect->intensity * 0.2, 
                                 alpha);
            
            cairo_set_line_width(cr, 3.0 - ring);
            cairo_arc(cr, effect->x, effect->y, ring_radius, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
        
        // Central flash
        if (effect->life > 0.8) {
            double flash_alpha = (effect->life - 0.8) * 5.0;
            cairo_set_source_rgba(cr, 1.0, 0.8, 1.0, flash_alpha);
            cairo_arc(cr, effect->x, effect->y, effect->intensity * 15, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
    
    // Draw bubbles
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!vis->bubbles[i].active) continue;
        
        Bubble *bubble = &vis->bubbles[i];
        
        // Calculate bubble appearance
        double age_factor = 1.0 - bubble->life;
        double pulse = sin(vis->time_offset * 4.0 + bubble->birth_time) * 0.1 + 1.0;
        double draw_radius = bubble->radius * pulse;
        
        // Audio-reactive color intensity
        double audio_intensity = 0.0;
        for (int j = 0; j < VIS_FREQUENCY_BARS; j++) {
            audio_intensity += vis->frequency_bands[j];
        }
        audio_intensity /= VIS_FREQUENCY_BARS;
        
        // Purple color variations
        double r = 0.4 + bubble->intensity * 0.4 + audio_intensity * 0.2;
        double g = 0.1 + audio_intensity * 0.3;
        double b = 0.7 + bubble->intensity * 0.3;
        double alpha = bubble->life * 0.8;
        
        // Draw bubble with gradient
        cairo_pattern_t *gradient = cairo_pattern_create_radial(
            bubble->x - draw_radius * 0.3, bubble->y - draw_radius * 0.3, 0,
            bubble->x, bubble->y, draw_radius);
        
        cairo_pattern_add_color_stop_rgba(gradient, 0, r + 0.3, g + 0.3, b + 0.2, alpha);
        cairo_pattern_add_color_stop_rgba(gradient, 0.7, r, g, b, alpha * 0.8);
        cairo_pattern_add_color_stop_rgba(gradient, 1.0, r * 0.5, g * 0.5, b * 0.8, alpha * 0.3);
        
        cairo_set_source(cr, gradient);
        cairo_arc(cr, bubble->x, bubble->y, draw_radius, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(gradient);
        
        // Highlight
        cairo_set_source_rgba(cr, 1.0, 0.9, 1.0, alpha * 0.6);
        cairo_arc(cr, bubble->x - draw_radius * 0.4, bubble->y - draw_radius * 0.4, 
                  draw_radius * 0.2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Outer ring for larger bubbles
        if (draw_radius > 25) {
            cairo_set_source_rgba(cr, r, g, b, alpha * 0.4);
            cairo_set_line_width(cr, 2.0);
            cairo_arc(cr, bubble->x, bubble->y, draw_radius + 3, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }
    
    // Draw floating particles for ambiance
    for (int i = 0; i < 20; i++) {
        double x = fmod(vis->time_offset * 10 + i * 37, vis->width);
        double y = fmod(vis->time_offset * 5 + i * 73, vis->height);
        double size = 1 + sin(vis->time_offset + i) * 0.5;
        
        cairo_set_source_rgba(cr, 0.7, 0.3, 0.9, 0.3);
        cairo_arc(cr, x, y, size, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
}

#endif
