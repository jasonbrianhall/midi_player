#include "visualization.h"

void init_parrot_system(Visualizer *vis) {
    vis->parrot_state.mouth_open = 0.0;
    vis->parrot_state.blink_timer = 0.0;
    vis->parrot_state.eye_closed = FALSE;
}

void update_parrot(Visualizer *vis, double dt) {
    double target_mouth = vis->volume_level * 3.0;
    if (target_mouth > 1.0) target_mouth = 1.0;
    
    double high_freq_energy = 0.0;
    for (int i = VIS_FREQUENCY_BARS/2; i < VIS_FREQUENCY_BARS; i++) {
        high_freq_energy += vis->frequency_bands[i];
    }
    high_freq_energy /= (VIS_FREQUENCY_BARS / 2);
    
    double speed_multiplier = 8.0 + high_freq_energy * 12.0;
    vis->parrot_state.mouth_open += (target_mouth - vis->parrot_state.mouth_open) * speed_multiplier * dt;
    
    if (vis->parrot_state.mouth_open > 0.3) {
        vis->parrot_state.mouth_open += sin(vis->time_offset * 20) * 0.02;
    }
    
    if (vis->parrot_state.mouth_open < 0.0) vis->parrot_state.mouth_open = 0.0;
    if (vis->parrot_state.mouth_open > 1.0) vis->parrot_state.mouth_open = 1.0;
    
    // Eye blinking
    vis->parrot_state.blink_timer += dt;
    if (!vis->parrot_state.eye_closed && vis->parrot_state.blink_timer > 3.0 + (rand() % 2000) / 1000.0) {
        vis->parrot_state.eye_closed = TRUE;
        vis->parrot_state.blink_timer = 0.0;
    } else if (vis->parrot_state.eye_closed && vis->parrot_state.blink_timer > 0.15) {
        vis->parrot_state.eye_closed = FALSE;
        vis->parrot_state.blink_timer = 0.0;
    }
}

void draw_music_notes(Visualizer *vis, cairo_t *cr, double cx, double cy, double scale) {
    // Draw musical notes that shoot out from the middle of the beak
    // Adjusted position - a bit lower and more to the left
    double mouth_x = cx - 150 * scale;
    double mouth_y = cy - 15 * scale;
    
    for (int i = 0; i < VIS_FREQUENCY_BARS; i += 2) {
        double intensity = vis->frequency_bands[i];
        
        if (intensity > 0.2) {
            // Time-based position
            double time_progress = fmod(vis->time_offset * 60 + i * 12, 400.0);
            
            // Shoot left in a straight line
            double note_x = mouth_x - time_progress * scale;
            double note_y = mouth_y;
            
            // Fade based on distance
            double distance_fade = 1.0 - (time_progress / 400.0);
            
            // Color based on frequency
            double hue = (double)i / VIS_FREQUENCY_BARS;
            double r, g, b;
            hsv_to_rgb(hue * 0.8, 0.8, 1.0, &r, &g, &b);
            
            // Opacity based on audio intensity AND distance
            double alpha = intensity * distance_fade;
            cairo_set_source_rgba(cr, r, g, b, alpha);
            
            double note_scale = scale * 0.7;
            
            // Note head
            cairo_arc(cr, note_x, note_y, 6 * note_scale, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Note stem
            cairo_set_line_width(cr, 2 * note_scale);
            cairo_move_to(cr, note_x + 5 * note_scale, note_y);
            cairo_line_to(cr, note_x + 5 * note_scale, note_y - 20 * note_scale);
            cairo_stroke(cr);
            
            // Note flag
            cairo_move_to(cr, note_x + 5 * note_scale, note_y - 20 * note_scale);
            cairo_curve_to(cr,
                note_x + 15 * note_scale, note_y - 18 * note_scale,
                note_x + 18 * note_scale, note_y - 10 * note_scale,
                note_x + 12 * note_scale, note_y - 5 * note_scale);
            cairo_fill(cr);
        }
    }
}

void draw_audio_bars_around_parrot(Visualizer *vis, cairo_t *cr, double cx, double cy, double scale) {
    double radius = 220 * scale;
    int num_bars = VIS_FREQUENCY_BARS;
    
    for (int i = 0; i < num_bars; i++) {
        double angle = (double)i / num_bars * 2.0 * M_PI - M_PI / 2;
        double bar_height = vis->frequency_bands[i] * 80 * scale;
        
        double x1 = cx + cos(angle) * radius;
        double y1 = cy + sin(angle) * radius;
        double x2 = cx + cos(angle) * (radius + bar_height);
        double y2 = cy + sin(angle) * (radius + bar_height);
        
        double hue = (double)i / num_bars;
        double r, g, b;
        hsv_to_rgb(hue * 0.8, 0.6, 0.8, &r, &g, &b);
        
        cairo_set_source_rgba(cr, r, g, b, 0.5);
        cairo_set_line_width(cr, 4 * scale);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }
}

void draw_parrot(Visualizer *vis, cairo_t *cr) {
    double bird_height = vis->height * 0.9;
    double scale = bird_height / 400.0;
    
    double cx = vis->width / 2.0;
    double cy = vis->height / 2.0 - 30 * scale;
    
    // Tail feathers
    for (int i = 0; i < 8; i++) {
        double y_offset = i * 12 * scale - 50 * scale;
        
        cairo_set_source_rgb(cr, 0.6 + i * 0.05, 0.85 - i * 0.03, 0.2);
        
        cairo_move_to(cr, cx + 80 * scale, cy + y_offset);
        cairo_curve_to(cr,
            cx + 120 * scale, cy + y_offset + 10 * scale,
            cx + 160 * scale, cy + y_offset + 30 * scale,
            cx + 180 * scale, cy + y_offset + 60 * scale);
        cairo_line_to(cr, cx + 175 * scale, cy + y_offset + 65 * scale);
        cairo_curve_to(cr,
            cx + 155 * scale, cy + y_offset + 35 * scale,
            cx + 115 * scale, cy + y_offset + 15 * scale,
            cx + 80 * scale, cy + y_offset + 5 * scale);
        cairo_close_path(cr);
        cairo_fill(cr);
    }
    
    // Main body
    cairo_set_source_rgb(cr, 0.1, 0.75, 0.2);
    cairo_arc(cr, cx + 30 * scale, cy, 90 * scale, 0, 2 * M_PI);
    cairo_fill(cr);
    
    // Wing detail
    cairo_set_source_rgb(cr, 0.05, 0.6, 0.15);
    cairo_move_to(cr, cx + 40 * scale, cy - 40 * scale);
    cairo_curve_to(cr,
        cx + 90 * scale, cy - 20 * scale,
        cx + 100 * scale, cy + 30 * scale,
        cx + 80 * scale, cy + 70 * scale);
    cairo_curve_to(cr,
        cx + 60 * scale, cy + 60 * scale,
        cx + 40 * scale, cy + 30 * scale,
        cx + 40 * scale, cy - 40 * scale);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Wing feather lines
    cairo_set_source_rgba(cr, 0.0, 0.4, 0.1, 0.6);
    cairo_set_line_width(cr, 3 * scale);
    for (int i = 0; i < 6; i++) {
        double start_y = cy - 30 * scale + i * 18 * scale;
        cairo_move_to(cr, cx + 45 * scale, start_y);
        cairo_line_to(cr, cx + 85 * scale, start_y + 10 * scale);
        cairo_stroke(cr);
    }
    
    // Chest accent
    cairo_set_source_rgb(cr, 0.95, 0.15, 0.55);
    cairo_arc(cr, cx - 20 * scale, cy + 20 * scale, 50 * scale, 0, 2 * M_PI);
    cairo_fill(cr);
    
    // Legs and feet
    cairo_set_source_rgb(cr, 1.0, 0.6, 0.0);
    cairo_set_line_width(cr, 6 * scale);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    // Left leg
    cairo_move_to(cr, cx + 10 * scale, cy + 70 * scale);
    cairo_line_to(cr, cx + 5 * scale, cy + 110 * scale);
    cairo_stroke(cr);
    
    // Left foot toes
    cairo_move_to(cr, cx + 5 * scale, cy + 110 * scale);
    cairo_line_to(cr, cx - 5 * scale, cy + 120 * scale);
    cairo_stroke(cr);
    cairo_move_to(cr, cx + 5 * scale, cy + 110 * scale);
    cairo_line_to(cr, cx + 5 * scale, cy + 122 * scale);
    cairo_stroke(cr);
    cairo_move_to(cr, cx + 5 * scale, cy + 110 * scale);
    cairo_line_to(cr, cx + 15 * scale, cy + 120 * scale);
    cairo_stroke(cr);
    
    // Right leg
    cairo_move_to(cr, cx + 35 * scale, cy + 75 * scale);
    cairo_line_to(cr, cx + 35 * scale, cy + 115 * scale);
    cairo_stroke(cr);
    
    // Right foot toes
    cairo_move_to(cr, cx + 35 * scale, cy + 115 * scale);
    cairo_line_to(cr, cx + 25 * scale, cy + 125 * scale);
    cairo_stroke(cr);
    cairo_move_to(cr, cx + 35 * scale, cy + 115 * scale);
    cairo_line_to(cr, cx + 35 * scale, cy + 127 * scale);
    cairo_stroke(cr);
    cairo_move_to(cr, cx + 35 * scale, cy + 115 * scale);
    cairo_line_to(cr, cx + 45 * scale, cy + 125 * scale);
    cairo_stroke(cr);
    
    // Blue head
    cairo_set_source_rgb(cr, 0.0, 0.45, 1.0);
    cairo_arc(cr, cx - 80 * scale, cy - 60 * scale, 70 * scale, 0, 2 * M_PI);
    cairo_fill(cr);
    
    // White eye ring
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_arc(cr, cx - 80 * scale, cy - 70 * scale, 28 * scale, 0, 2 * M_PI);
    cairo_fill(cr);
    
    // Black pupil (or closed eye)
    if (vis->parrot_state.eye_closed) {
        // Draw closed eye as a horizontal line
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_line_width(cr, 4 * scale);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, cx - 95 * scale, cy - 70 * scale);
        cairo_line_to(cr, cx - 65 * scale, cy - 70 * scale);
        cairo_stroke(cr);
    } else {
        // Draw open eye
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_arc(cr, cx - 85 * scale, cy - 70 * scale, 15 * scale, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Eye shine
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.9);
        cairo_arc(cr, cx - 78 * scale, cy - 75 * scale, 6 * scale, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // BEAK
    double mouth_gap = vis->parrot_state.mouth_open * 35 * scale;
    
    // Upper beak
    cairo_set_source_rgb(cr, 1.0, 0.55, 0.0);
    cairo_move_to(cr, cx - 100 * scale, cy - 45 * scale - mouth_gap * 0.3);
    cairo_curve_to(cr,
        cx - 140 * scale, cy - 50 * scale - mouth_gap * 0.2,
        cx - 165 * scale, cy - 35 * scale,
        cx - 170 * scale, cy - 10 * scale);
    cairo_curve_to(cr,
        cx - 165 * scale, cy,
        cx - 145 * scale, cy - 5 * scale,
        cx - 120 * scale, cy - 20 * scale);
    cairo_curve_to(cr,
        cx - 105 * scale, cy - 30 * scale - mouth_gap * 0.2,
        cx - 100 * scale, cy - 40 * scale - mouth_gap * 0.25,
        cx - 100 * scale, cy - 45 * scale - mouth_gap * 0.3);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Upper beak highlight
    cairo_set_source_rgba(cr, 1.0, 0.75, 0.2, 0.6);
    cairo_move_to(cr, cx - 110 * scale, cy - 40 * scale - mouth_gap * 0.3);
    cairo_curve_to(cr,
        cx - 140 * scale, cy - 43 * scale - mouth_gap * 0.2,
        cx - 155 * scale, cy - 30 * scale,
        cx - 158 * scale, cy - 12 * scale);
    cairo_line_to(cr, cx - 150 * scale, cy - 8 * scale);
    cairo_curve_to(cr,
        cx - 135 * scale, cy - 15 * scale,
        cx - 115 * scale, cy - 25 * scale - mouth_gap * 0.2,
        cx - 110 * scale, cy - 40 * scale - mouth_gap * 0.3);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Lower beak
    cairo_set_source_rgb(cr, 0.9, 0.5, 0.0);
    cairo_move_to(cr, cx - 100 * scale, cy - 35 * scale + mouth_gap * 0.7);
    cairo_curve_to(cr,
        cx - 135 * scale, cy - 30 * scale + mouth_gap * 0.8,
        cx - 160 * scale, cy - 15 * scale + mouth_gap * 0.6,
        cx - 168 * scale, cy + 5 * scale + mouth_gap * 0.4);
    cairo_curve_to(cr,
        cx - 163 * scale, cy + 15 * scale + mouth_gap * 0.3,
        cx - 145 * scale, cy + 15 * scale + mouth_gap * 0.3,
        cx - 125 * scale, cy + 5 * scale + mouth_gap * 0.5);
    cairo_curve_to(cr,
        cx - 108 * scale, cy - 10 * scale + mouth_gap * 0.65,
        cx - 102 * scale, cy - 25 * scale + mouth_gap * 0.68,
        cx - 100 * scale, cy - 35 * scale + mouth_gap * 0.7);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Mouth interior
    if (vis->parrot_state.mouth_open > 0.15) {
        cairo_set_source_rgba(cr, 0.1, 0.0, 0.0, vis->parrot_state.mouth_open * 0.85);
        cairo_move_to(cr, cx - 100 * scale, cy - 40 * scale);
        cairo_line_to(cr, cx - 140 * scale, cy - 25 * scale + mouth_gap * 0.3);
        cairo_line_to(cr, cx - 155 * scale, cy - 5 * scale + mouth_gap * 0.5);
        cairo_line_to(cr, cx - 140 * scale, cy + 5 * scale + mouth_gap * 0.5);
        cairo_line_to(cr, cx - 105 * scale, cy - 15 * scale + mouth_gap * 0.4);
        cairo_close_path(cr);
        cairo_fill(cr);
    }
    
    // Tongue
    if (vis->parrot_state.mouth_open > 0.4) {
        cairo_set_source_rgba(cr, 0.9, 0.2, 0.2, vis->parrot_state.mouth_open * 0.7);
        cairo_arc(cr, cx - 130 * scale, cy - 10 * scale + mouth_gap * 0.4, 12 * scale, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    draw_music_notes(vis, cr, cx, cy, scale);
    draw_audio_bars_around_parrot(vis, cr, cx, cy, scale);
}
