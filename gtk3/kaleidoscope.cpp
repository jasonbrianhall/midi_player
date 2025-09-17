#include "visualization.h"

void init_kaleidoscope_system(Visualizer *vis) {
    vis->kaleidoscope_shape_count = 0;
    vis->kaleidoscope_rotation = 0.0;
    vis->kaleidoscope_rotation_speed = 1.5;  // Start faster
    vis->kaleidoscope_zoom = 1.0;
    vis->kaleidoscope_zoom_target = 1.0;
    vis->kaleidoscope_spawn_timer = 0.0;
    vis->kaleidoscope_mirror_offset = 0.0;
    vis->kaleidoscope_mirror_count = 8;  // Start with more mirrors
    vis->kaleidoscope_color_shift = 0.0;
    vis->kaleidoscope_auto_shapes = TRUE;
    
    // Initialize all shapes as inactive
    for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
        vis->kaleidoscope_shapes[i].active = FALSE;
    }
    
    // Force spawn many initial shapes for immediate visual impact
    for (int i = 0; i < 10; i++) {
        spawn_kaleidoscope_shape(vis, 0.5 + (i * 0.1), i % VIS_FREQUENCY_BARS);
    }
}

void spawn_kaleidoscope_shape(Visualizer *vis, double intensity, int frequency_band) {
    // Find an inactive shape slot
    int slot = -1;
    for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
        if (!vis->kaleidoscope_shapes[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        // Force replace oldest shape (wrap around)
        slot = vis->kaleidoscope_shape_count % MAX_KALEIDOSCOPE_SHAPES;
    }
    
    KaleidoscopeShape *shape = &vis->kaleidoscope_shapes[slot];
    
    // Random positioning within triangle bounds
    shape->x = 0.2 + (rand() / (double)RAND_MAX) * 0.6;
    shape->y = 0.2 + (rand() / (double)RAND_MAX) * 0.6;
    
    // More aggressive movement
    shape->vx = ((rand() / (double)RAND_MAX) - 0.5) * 0.1;
    shape->vy = ((rand() / (double)RAND_MAX) - 0.5) * 0.1;
    
    // Wild rotation
    shape->rotation = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
    shape->rotation_speed = ((rand() / (double)RAND_MAX) - 0.5) * 8.0;
    
    // Much larger, more visible shapes
    shape->scale = 0.1 + intensity * 0.4;
    shape->scale_speed = 2.0 + intensity * 3.0;
    
    // Vivid colors
    shape->hue = (double)frequency_band / VIS_FREQUENCY_BARS;
    shape->saturation = 0.9 + intensity * 0.1;
    shape->base_brightness = 0.8 + intensity * 0.2;
    shape->brightness = shape->base_brightness;
    
    // Random shape types
    shape->shape_type = rand() % 7;
    
    // Much longer life
    shape->life = 2.0 + intensity * 2.0;
    shape->pulse_phase = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
    shape->frequency_band = frequency_band;
    shape->active = TRUE;
    
    vis->kaleidoscope_shape_count++;
}

void update_kaleidoscope(Visualizer *vis, double dt) {
    // Always spinning fast
    vis->kaleidoscope_rotation_speed = 2.0 + vis->volume_level * 4.0;
    vis->kaleidoscope_rotation += vis->kaleidoscope_rotation_speed * dt;
    if (vis->kaleidoscope_rotation > 2.0 * M_PI) {
        vis->kaleidoscope_rotation -= 2.0 * M_PI;
    }
    
    // Dramatic zoom
    vis->kaleidoscope_zoom_target = 0.5 + vis->volume_level * 1.5;
    vis->kaleidoscope_zoom += (vis->kaleidoscope_zoom_target - vis->kaleidoscope_zoom) * dt * 5.0;
    
    // Fast color cycling
    vis->kaleidoscope_color_shift += dt * 0.8;
    if (vis->kaleidoscope_color_shift > 1.0) vis->kaleidoscope_color_shift -= 1.0;
    
    // Dynamic mirror effects
    vis->kaleidoscope_mirror_offset = sin(vis->time_offset * 2.0) * 0.3;
    
    // Force constant spawning
    vis->kaleidoscope_spawn_timer += dt;
    if (vis->kaleidoscope_spawn_timer > 0.1) {  // Spawn every 100ms
        int random_band = rand() % VIS_FREQUENCY_BARS;
        double intensity = 0.3 + (rand() / (double)RAND_MAX) * 0.7;
        spawn_kaleidoscope_shape(vis, intensity, random_band);
        vis->kaleidoscope_spawn_timer = 0.0;
    }
    
    // Update all shapes
    int active_count = 0;
    for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
        KaleidoscopeShape *shape = &vis->kaleidoscope_shapes[i];
        
        if (!shape->active) continue;
        active_count++;
        
        // Wild movement
        shape->x += shape->vx * dt;
        shape->y += shape->vy * dt;
        
        // Wrap around instead of bounce
        if (shape->x < 0.0) shape->x = 1.0;
        if (shape->x > 1.0) shape->x = 0.0;
        if (shape->y < 0.0) shape->y = 1.0;
        if (shape->y > 1.0) shape->y = 0.0;
        
        // Crazy rotation
        shape->rotation += shape->rotation_speed * dt;
        
        // Extreme pulsing
        shape->pulse_phase += shape->scale_speed * dt;
        double pulse = sin(shape->pulse_phase) * 0.8 + 1.2;
        shape->scale = (0.05 + 0.2) * pulse;
        
        // Strobe brightness
        shape->brightness = shape->base_brightness + sin(vis->time_offset * 10.0 + i) * 0.5;
        if (shape->brightness < 0.1) shape->brightness = 0.1;
        
        // Slower life decay
        shape->life -= dt * 0.2;
        if (shape->life <= 0.0) {
            shape->active = FALSE;
            active_count--;
        }
    }
    
    vis->kaleidoscope_shape_count = active_count;
}

void draw_kaleidoscope_shape(cairo_t *cr, KaleidoscopeShape *shape, double scale_factor) {
    cairo_save(cr);
    
    // Convert hue to RGB
    double r, g, b;
    hsv_to_rgb(shape->hue, shape->saturation, shape->brightness * shape->life, &r, &g, &b);
    
    // Bright, solid colors
    cairo_set_source_rgba(cr, r, g, b, shape->life);
    
    // Apply transformations
    cairo_rotate(cr, shape->rotation);
    cairo_scale(cr, shape->scale * scale_factor, shape->scale * scale_factor);
    
    // Draw big, bold shapes
    switch (shape->shape_type) {
        case 0: // Large circle
            cairo_arc(cr, 0, 0, 1.5, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
            
        case 1: // Large triangle
            cairo_move_to(cr, 0, -2);
            cairo_line_to(cr, 1.7, 1);
            cairo_line_to(cr, -1.7, 1);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 2: // Large square
            cairo_rectangle(cr, -1.5, -1.5, 3.0, 3.0);
            cairo_fill(cr);
            break;
            
        case 3: // Large star
            {
                double outer_radius = 2.0;
                double inner_radius = 0.8;
                cairo_move_to(cr, 0, -outer_radius);
                for (int i = 1; i < 10; i++) {
                    double angle = i * M_PI / 5.0;
                    double radius = (i % 2 == 0) ? outer_radius : inner_radius;
                    cairo_line_to(cr, sin(angle) * radius, -cos(angle) * radius);
                }
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            break;
            
        case 4: // Large hexagon
            cairo_move_to(cr, 1.5, 0);
            for (int i = 1; i < 6; i++) {
                double angle = i * M_PI / 3.0;
                cairo_line_to(cr, cos(angle) * 1.5, sin(angle) * 1.5);
            }
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 5: // Large diamond
            cairo_move_to(cr, 0, -2);
            cairo_line_to(cr, 1.4, 0);
            cairo_line_to(cr, 0, 2);
            cairo_line_to(cr, -1.4, 0);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 6: // Cross/plus
            cairo_rectangle(cr, -0.4, -2, 0.8, 4);
            cairo_fill(cr);
            cairo_rectangle(cr, -2, -0.4, 4, 0.8);
            cairo_fill(cr);
            break;
    }
    
    cairo_restore(cr);
}

void draw_kaleidoscope(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    double center_x = vis->width / 2.0;
    double center_y = vis->height / 2.0;
    double radius = fmin(center_x, center_y) * 0.9;
    
    cairo_save(cr);
    
    // Black background
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    
    // Create circular clipping mask
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_clip(cr);
    
    // Apply global transformations
    cairo_translate(cr, center_x, center_y);
    cairo_rotate(cr, vis->kaleidoscope_rotation);
    cairo_scale(cr, vis->kaleidoscope_zoom, vis->kaleidoscope_zoom);
    
    // Draw with many mirrors for complex patterns
    int mirror_count = 12;
    double segment_angle = 2.0 * M_PI / mirror_count;
    
    // Draw each mirror segment
    for (int segment = 0; segment < mirror_count; segment++) {
        cairo_save(cr);
        
        // Rotate to this segment
        cairo_rotate(cr, segment * segment_angle);
        
        // Create triangular clipping region
        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, radius * 2, 0);
        cairo_line_to(cr, radius * 2 * cos(segment_angle), radius * 2 * sin(segment_angle));
        cairo_close_path(cr);
        cairo_clip(cr);
        
        // Draw all shapes in this segment
        for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
            KaleidoscopeShape *shape = &vis->kaleidoscope_shapes[i];
            if (!shape->active) continue;
            
            cairo_save(cr);
            
            // Position shape
            double shape_x = (shape->x - 0.5) * radius * 1.5;
            double shape_y = (shape->y - 0.5) * radius * 1.5;
            cairo_translate(cr, shape_x, shape_y);
            
            // Color shift per segment
            KaleidoscopeShape temp_shape = *shape;
            temp_shape.hue += vis->kaleidoscope_color_shift + (segment * 0.08);
            if (temp_shape.hue > 1.0) temp_shape.hue -= 1.0;
            
            draw_kaleidoscope_shape(cr, &temp_shape, 50.0);
            
            cairo_restore(cr);
        }
        
        // Mirror every other segment
        if (segment % 2 == 1) {
            cairo_save(cr);
            cairo_scale(cr, 1.0, -1.0);
            
            for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
                KaleidoscopeShape *shape = &vis->kaleidoscope_shapes[i];
                if (!shape->active) continue;
                
                cairo_save(cr);
                
                double shape_x = (shape->x - 0.5) * radius * 1.5;
                double shape_y = (shape->y - 0.5) * radius * 1.5;
                cairo_translate(cr, shape_x, shape_y);
                
                KaleidoscopeShape temp_shape = *shape;
                temp_shape.hue += vis->kaleidoscope_color_shift + (segment * 0.08) + 0.5;
                if (temp_shape.hue > 1.0) temp_shape.hue -= 1.0;
                temp_shape.brightness *= 0.8;
                
                draw_kaleidoscope_shape(cr, &temp_shape, 50.0);
                
                cairo_restore(cr);
            }
            cairo_restore(cr);
        }
        
        cairo_restore(cr);
    }
    
    cairo_restore(cr);
    
    // Bright outer ring
    cairo_set_source_rgba(cr, 0.0, 1.0, 0.5, 0.8);
    cairo_set_line_width(cr, 5.0 + vis->volume_level * 10.0);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
}
