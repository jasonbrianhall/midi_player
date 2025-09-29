#include "visualization.h"

void draw_oscilloscope(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Draw grid
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
    cairo_set_line_width(cr, 1.0);
    
    // Horizontal lines
    for (int i = 1; i < 4; i++) {
        double y = vis->height * i / 4.0;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, vis->width, y);
        cairo_stroke(cr);
    }
    
    // Vertical lines
    for (int i = 1; i < 8; i++) {
        double x = vis->width * i / 8.0;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, vis->height);
        cairo_stroke(cr);
    }
    
    // Draw waveform
    cairo_set_source_rgba(cr, vis->accent_r, vis->accent_g, vis->accent_b, 1.0);
    cairo_set_line_width(cr, 2.0);
    
    cairo_move_to(cr, 0, vis->height / 2.0);
    
    for (int i = 0; i < VIS_SAMPLES; i++) {
        double x = (double)i * vis->width / (VIS_SAMPLES - 1);
        double y = vis->height / 2.0 + vis->audio_samples[i] * vis->height / 2.5;
        
        // Clamp y to screen bounds
        if (y < 0) y = 0;
        if (y > vis->height) y = vis->height;
        
        cairo_line_to(cr, x, y);
    }
    
    cairo_stroke(cr);
}
