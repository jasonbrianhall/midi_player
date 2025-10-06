#include "visualization.h"

void kaleidoscope_hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b) {
    if (s == 0.0) {
        *r = *g = *b = v;
        return;
    }
    
    h = h * 6.0;
    int i = (int)h;
    double f = h - i;
    double p = v * (1.0 - s);
    double q = v * (1.0 - s * f);
    double t = v * (1.0 - s * (1.0 - f));
    
    switch(i % 6) {
        case 0: *r = v; *g = t; *b = p; break;
        case 1: *r = q; *g = v; *b = p; break;
        case 2: *r = p; *g = v; *b = t; break;
        case 3: *r = p; *g = q; *b = v; break;
        case 4: *r = t; *g = p; *b = v; break;
        case 5: *r = v; *g = p; *b = q; break;
    }
}

void init_kaleidoscope_system(Visualizer *vis) {
    vis->kaleidoscope_shape_count = 0;
    vis->kaleidoscope_rotation = 0.0;
    vis->kaleidoscope_rotation_speed = 1.5;
    vis->kaleidoscope_zoom = 1.0;
    vis->kaleidoscope_zoom_target = 1.0;
    vis->kaleidoscope_spawn_timer = 0.0;
    vis->kaleidoscope_mirror_offset = 0.0;
    vis->kaleidoscope_mirror_count = 8;
    vis->kaleidoscope_color_shift = 0.0;
    vis->kaleidoscope_auto_shapes = TRUE;
    
    for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
        vis->kaleidoscope_shapes[i].active = FALSE;
    }
    
    for (int i = 0; i < 12; i++) {
        int slot = i;
        KaleidoscopeShape *shape = &vis->kaleidoscope_shapes[slot];
        
        shape->x = 0.3 + (rand() / (double)RAND_MAX) * 0.4;
        shape->y = 0.3 + (rand() / (double)RAND_MAX) * 0.4;
        shape->vx = ((rand() / (double)RAND_MAX) - 0.5) * 0.08;
        shape->vy = ((rand() / (double)RAND_MAX) - 0.5) * 0.08;
        shape->rotation = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
        shape->rotation_speed = ((rand() / (double)RAND_MAX) - 0.5) * 6.0;
        shape->scale = 0.2;
        shape->scale_speed = 2.5;
        shape->hue = rand() / (double)RAND_MAX;
        shape->saturation = 1.0;
        shape->base_brightness = 1.0;
        shape->brightness = 1.0;
        shape->shape_type = rand() % 7;
        shape->life = 3.0;
        shape->pulse_phase = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
        shape->frequency_band = 0;
        shape->active = TRUE;
    }
    
    vis->kaleidoscope_shape_count = 12;
}

void spawn_kaleidoscope_shape(Visualizer *vis, double intensity, int frequency_band) {
    int slot = -1;
    for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
        if (!vis->kaleidoscope_shapes[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        slot = rand() % MAX_KALEIDOSCOPE_SHAPES;
    }
    
    KaleidoscopeShape *shape = &vis->kaleidoscope_shapes[slot];
    
    shape->x = 0.3 + (rand() / (double)RAND_MAX) * 0.4;
    shape->y = 0.3 + (rand() / (double)RAND_MAX) * 0.4;
    shape->vx = ((rand() / (double)RAND_MAX) - 0.5) * 0.08;
    shape->vy = ((rand() / (double)RAND_MAX) - 0.5) * 0.08;
    shape->rotation = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
    shape->rotation_speed = ((rand() / (double)RAND_MAX) - 0.5) * 6.0;
    shape->scale = 0.15 + intensity * 0.3;
    shape->scale_speed = 2.5 + intensity * 2.5;
    shape->hue = rand() / (double)RAND_MAX;
    shape->saturation = 1.0;
    shape->base_brightness = 1.0;
    shape->brightness = 1.0;
    shape->shape_type = rand() % 7;
    shape->life = 3.0;
    shape->pulse_phase = (rand() / (double)RAND_MAX) * 2.0 * M_PI;
    shape->frequency_band = 0;
    shape->active = TRUE;
    
    vis->kaleidoscope_shape_count++;
}

void update_kaleidoscope(Visualizer *vis, double dt) {
    vis->kaleidoscope_rotation += 2.5 * dt;
    if (vis->kaleidoscope_rotation > 2.0 * M_PI) {
        vis->kaleidoscope_rotation -= 2.0 * M_PI;
    }
    
    vis->kaleidoscope_zoom = 0.8 + vis->volume_level * 0.5;
    
    vis->kaleidoscope_spawn_timer += dt;
    if (vis->kaleidoscope_spawn_timer > 0.15) {
        spawn_kaleidoscope_shape(vis, 0.6, 0);
        vis->kaleidoscope_spawn_timer = 0.0;
    }
    
    int active = 0;
    for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
        KaleidoscopeShape *s = &vis->kaleidoscope_shapes[i];
        
        if (!s->active) continue;
        active++;
        
        s->x += s->vx * dt;
        s->y += s->vy * dt;
        
        if (s->x < 0.0) s->x = 1.0;
        if (s->x > 1.0) s->x = 0.0;
        if (s->y < 0.0) s->y = 1.0;
        if (s->y > 1.0) s->y = 0.0;
        
        s->rotation += s->rotation_speed * dt;
        s->pulse_phase += s->scale_speed * dt;
        
        double pulse = sin(s->pulse_phase) * 0.5 + 1.0;
        s->scale = 0.2 * pulse;
        
        s->brightness = 0.8 + sin(vis->time_offset * 8.0 + i) * 0.2;
        
        s->life -= dt * 0.25;
        if (s->life <= 0.0) {
            s->active = FALSE;
            active--;
        }
    }
    
    vis->kaleidoscope_shape_count = active;
}

void draw_kaleidoscope_shape(cairo_t *cr, KaleidoscopeShape *shape, double scale_factor) {
    double r, g, b;
    kaleidoscope_hsv_to_rgb(shape->hue, shape->saturation, shape->brightness, &r, &g, &b);
    
    cairo_set_source_rgba(cr, r, g, b, shape->life / 3.0);
    cairo_rotate(cr, shape->rotation);
    cairo_scale(cr, shape->scale * scale_factor, shape->scale * scale_factor);
    
    switch (shape->shape_type) {
        case 0:
            cairo_arc(cr, 0, 0, 1.5, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
            
        case 1:
            cairo_move_to(cr, 0, -2);
            cairo_line_to(cr, 1.7, 1);
            cairo_line_to(cr, -1.7, 1);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 2:
            cairo_rectangle(cr, -1.5, -1.5, 3.0, 3.0);
            cairo_fill(cr);
            break;
            
        case 3:
            cairo_move_to(cr, 0, -2);
            for (int i = 1; i < 10; i++) {
                double angle = i * M_PI / 5.0;
                double rad = (i % 2 == 0) ? 2.0 : 0.8;
                cairo_line_to(cr, sin(angle) * rad, -cos(angle) * rad);
            }
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 4:
            cairo_move_to(cr, 1.5, 0);
            for (int i = 1; i < 6; i++) {
                double angle = i * M_PI / 3.0;
                cairo_line_to(cr, cos(angle) * 1.5, sin(angle) * 1.5);
            }
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 5:
            cairo_move_to(cr, 0, -2);
            cairo_line_to(cr, 1.4, 0);
            cairo_line_to(cr, 0, 2);
            cairo_line_to(cr, -1.4, 0);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
            
        case 6:
            cairo_rectangle(cr, -0.4, -2, 0.8, 4);
            cairo_fill(cr);
            cairo_rectangle(cr, -2, -0.4, 4, 0.8);
            cairo_fill(cr);
            break;
    }
}

void draw_kaleidoscope(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    double cx = vis->width / 2.0;
    double cy = vis->height / 2.0;
    double rad = fmin(cx, cy) * 0.9;
    
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    
    cairo_save(cr);
    cairo_arc(cr, cx, cy, rad, 0, 2 * M_PI);
    cairo_clip(cr);
    
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, vis->kaleidoscope_rotation);
    cairo_scale(cr, vis->kaleidoscope_zoom, vis->kaleidoscope_zoom);
    
    int mirrors = 12;
    double angle = 2.0 * M_PI / mirrors;
    
    for (int m = 0; m < mirrors; m++) {
        cairo_save(cr);
        cairo_rotate(cr, m * angle);
        
        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, rad * 2, 0);
        cairo_line_to(cr, rad * 2 * cos(angle), rad * 2 * sin(angle));
        cairo_close_path(cr);
        cairo_clip(cr);
        
        for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
            KaleidoscopeShape *s = &vis->kaleidoscope_shapes[i];
            if (!s->active) continue;
            
            cairo_save(cr);
            cairo_translate(cr, (s->x - 0.5) * rad * 1.5, (s->y - 0.5) * rad * 1.5);
            draw_kaleidoscope_shape(cr, s, 50.0);
            cairo_restore(cr);
        }
        
        if (m % 2 == 1) {
            cairo_save(cr);
            cairo_scale(cr, 1.0, -1.0);
            
            for (int i = 0; i < MAX_KALEIDOSCOPE_SHAPES; i++) {
                KaleidoscopeShape *s = &vis->kaleidoscope_shapes[i];
                if (!s->active) continue;
                
                cairo_save(cr);
                cairo_translate(cr, (s->x - 0.5) * rad * 1.5, (s->y - 0.5) * rad * 1.5);
                draw_kaleidoscope_shape(cr, s, 50.0);
                cairo_restore(cr);
            }
            cairo_restore(cr);
        }
        
        cairo_restore(cr);
    }
    
    cairo_restore(cr);
    
    // Draw dynamic audio-reactive outline like radial wave
    int num_points = VIS_FREQUENCY_BARS * 4;
    
    // Main waveform ring with variable line width
    for (int i = 0; i < num_points; i++) {
        double outline_angle = (double)i / num_points * 2.0 * M_PI + vis->kaleidoscope_rotation * 0.5;
        double next_angle = (double)(i + 1) / num_points * 2.0 * M_PI + vis->kaleidoscope_rotation * 0.5;
        
        // Map to frequency bands
        int band = ((i % num_points) * VIS_FREQUENCY_BARS) / num_points;
        if (band >= VIS_FREQUENCY_BARS) band = VIS_FREQUENCY_BARS - 1;
        
        double band_fraction = ((double)(i % num_points) * VIS_FREQUENCY_BARS) / num_points - band;
        int next_band = (band + 1) % VIS_FREQUENCY_BARS;
        
        double bulge = vis->frequency_bands[band] * (1.0 - band_fraction) + 
                       vis->frequency_bands[next_band] * band_fraction;
        double next_bulge = vis->frequency_bands[next_band];
        
        // Add ripple effect
        double ripple = sin(vis->time_offset * 2.0 + outline_angle * 3.0) * 0.08;
        double next_ripple = sin(vis->time_offset * 2.0 + next_angle * 3.0) * 0.08;
        
        double radius = rad + bulge * 40.0 + ripple * 8.0;
        double next_radius = rad + next_bulge * 40.0 + next_ripple * 8.0;
        
        double x = cx + cos(outline_angle) * radius;
        double y = cy + sin(outline_angle) * radius;
        double next_x = cx + cos(next_angle) * next_radius;
        double next_y = cy + sin(next_angle) * next_radius;
        
        double line_width = 1.5 + bulge * 4.0;
        double intensity = 0.7 + bulge * 0.3;
        
        cairo_set_line_width(cr, line_width);
        cairo_set_source_rgba(cr, 
            vis->accent_r * intensity, 
            vis->accent_g * intensity, 
            vis->accent_b * intensity, 
            0.85 + bulge * 0.15);
        
        cairo_move_to(cr, x, y);
        cairo_line_to(cr, next_x, next_y);
        cairo_stroke(cr);
    }
    
    // Inner glow layer
    cairo_new_path(cr);
    for (int i = 0; i <= num_points; i++) {
        double outline_angle = (double)i / num_points * 2.0 * M_PI + vis->kaleidoscope_rotation * 0.5;
        
        int band = ((i % num_points) * VIS_FREQUENCY_BARS) / num_points;
        if (band >= VIS_FREQUENCY_BARS) band = VIS_FREQUENCY_BARS - 1;
        
        double band_fraction = ((double)(i % num_points) * VIS_FREQUENCY_BARS) / num_points - band;
        int next_band = (band + 1) % VIS_FREQUENCY_BARS;
        
        double bulge = vis->frequency_bands[band] * (1.0 - band_fraction) + 
                       vis->frequency_bands[next_band] * band_fraction;
        double ripple = sin(vis->time_offset * 2.0 + outline_angle * 3.0) * 0.08;
        
        double radius = rad + bulge * 40.0 + ripple * 8.0;
        double x = cx + cos(outline_angle) * radius;
        double y = cy + sin(outline_angle) * radius;
        
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_close_path(cr);
    cairo_set_line_width(cr, 3.0);
    cairo_set_source_rgba(cr, vis->fg_r, vis->fg_g, vis->fg_b, 0.4);
    cairo_stroke(cr);
    
    // Secondary phase-shifted waveform for depth
    for (int i = 0; i < num_points; i++) {
        double outline_angle = (double)i / num_points * 2.0 * M_PI + vis->kaleidoscope_rotation * 0.5;
        double next_angle = (double)(i + 1) / num_points * 2.0 * M_PI + vis->kaleidoscope_rotation * 0.5;
        
        int band = ((i % num_points) * VIS_FREQUENCY_BARS) / num_points;
        if (band >= VIS_FREQUENCY_BARS) band = VIS_FREQUENCY_BARS - 1;
        
        double band_fraction = ((double)(i % num_points) * VIS_FREQUENCY_BARS) / num_points - band;
        int next_band = (band + 1) % VIS_FREQUENCY_BARS;
        
        // Phase shift for secondary wave
        int shifted_band = (band + VIS_FREQUENCY_BARS / 8) % VIS_FREQUENCY_BARS;
        int shifted_next = (next_band + VIS_FREQUENCY_BARS / 8) % VIS_FREQUENCY_BARS;
        
        double bulge = vis->frequency_bands[shifted_band] * (1.0 - band_fraction) + 
                       vis->frequency_bands[shifted_next] * band_fraction;
        
        double ripple = sin(vis->time_offset * 2.0 + outline_angle * 3.0 + M_PI / 4.0) * 0.06;
        double next_ripple = sin(vis->time_offset * 2.0 + next_angle * 3.0 + M_PI / 4.0) * 0.06;
        
        double radius = rad * 0.95 + bulge * 30.0 + ripple * 6.0;
        double next_radius = rad * 0.95 + bulge * 30.0 + next_ripple * 6.0;
        
        double x = cx + cos(outline_angle) * radius;
        double y = cy + sin(outline_angle) * radius;
        double next_x = cx + cos(next_angle) * next_radius;
        double next_y = cy + sin(next_angle) * next_radius;
        
        double line_width = 1.0 + bulge * 2.5;
        
        cairo_set_line_width(cr, line_width);
        cairo_set_source_rgba(cr, 
            vis->fg_r * 0.7, 
            vis->fg_g * 0.7, 
            vis->fg_b * 0.7, 
            0.3 + bulge * 0.2);
        
        cairo_move_to(cr, x, y);
        cairo_line_to(cr, next_x, next_y);
        cairo_stroke(cr);
    }
}
