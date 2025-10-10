#include "visualization.h"
#include <math.h>

void draw_trippy(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    double bar_width = (double)vis->width / VIS_FREQUENCY_BARS;
    static double time_offset = 0;
    time_offset += 0.05;
    
    // Psychedelic background waves
    for (int layer = 0; layer < 3; layer++) {
        cairo_save(cr);
        for (int x = 0; x < vis->width; x += 2) {
            double wave = sin(x * 0.02 + time_offset + layer) * 30;
            double y_pos = vis->height * 0.5 + wave + layer * 40;
            
            double hue_shift = (x / (double)vis->width + time_offset * 0.1 + layer * 0.3);
            hue_shift = fmod(hue_shift, 1.0);
            
            double r = 0.5 + 0.5 * sin(hue_shift * 6.28);
            double g = 0.5 + 0.5 * sin(hue_shift * 6.28 + 2.09);
            double b = 0.5 + 0.5 * sin(hue_shift * 6.28 + 4.18);
            
            cairo_set_source_rgba(cr, r, g, b, 0.15);
            cairo_rectangle(cr, x, y_pos, 2, 20);
            cairo_fill(cr);
        }
        cairo_restore(cr);
    }
    
    // Draw frequency bars with trippy effects
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        double base_height = vis->frequency_bands[i] * vis->height * 0.9;
        
        // Add wave distortion to each bar
        double wave_x = sin(time_offset * 2 + i * 0.3) * 10;
        double wave_scale = 1.0 + sin(time_offset * 3 + i * 0.5) * 0.2;
        double height = base_height * wave_scale;
        
        double x = i * bar_width + wave_x;
        double y = vis->height - height;
        
        // Cycling rainbow colors
        double hue = fmod((double)i / VIS_FREQUENCY_BARS + time_offset * 0.1, 1.0);
        double r = 0.5 + 0.5 * sin(hue * 6.28);
        double g = 0.5 + 0.5 * sin(hue * 6.28 + 2.09);
        double b = 0.5 + 0.5 * sin(hue * 6.28 + 4.18);
        
        // Create pulsing gradient
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, vis->height, 0, y);
        cairo_pattern_add_color_stop_rgba(gradient, 0, r, g, b, 0.2);
        cairo_pattern_add_color_stop_rgba(gradient, 0.5, r * 1.3, g * 1.3, b * 1.3, 0.7);
        cairo_pattern_add_color_stop_rgba(gradient, 1, r * 1.5, g * 1.5, b * 1.5, 1.0);
        
        cairo_set_source(cr, gradient);
        cairo_rectangle(cr, x + 1, y, bar_width - 2, height);
        cairo_fill(cr);
        cairo_pattern_destroy(gradient);
        
        // Draw glowing outline
        cairo_set_source_rgba(cr, r * 2, g * 2, b * 2, 0.5);
        cairo_set_line_width(cr, 2);
        cairo_rectangle(cr, x + 1, y, bar_width - 2, height);
        cairo_stroke(cr);
        

        
        // Add random sparkles at high frequencies
        if (vis->frequency_bands[i] > 0.7 && (i * 17 + (int)(time_offset * 50)) % 7 == 0) {
            double sparkle_y = y + (rand() % (int)height);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
            cairo_arc(cr, x + bar_width/2, sparkle_y, 2, 0, 6.28);
            cairo_fill(cr);
        }
    }
    
    // Add radial blur effect overlay
    cairo_pattern_t *vignette = cairo_pattern_create_radial(
        vis->width/2, vis->height/2, vis->height * 0.3,
        vis->width/2, vis->height/2, vis->height * 0.8
    );
    cairo_pattern_add_color_stop_rgba(vignette, 0, 0, 0, 0, 0);
    cairo_pattern_add_color_stop_rgba(vignette, 1, 0, 0, 0, 0.3);
    cairo_set_source(cr, vignette);
    cairo_paint(cr);
    cairo_pattern_destroy(vignette);
}
