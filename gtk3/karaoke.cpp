#include "visualization.h"
#include "cdg.h"
#include <cairo.h>

void draw_karaoke(Visualizer *vis, cairo_t *cr) {
    if (!vis->cdg_display || !vis->cdg_display->packets) {
        // Draw "no CDG loaded" message
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        cairo_paint(cr);
        
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 20);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, "No Karaoke File Loaded", &extents);
        cairo_move_to(cr, (vis->width - extents.width) / 2, vis->height / 2);
        cairo_show_text(cr, "No Karaoke File Loaded");
        return;
    }
    
    CDGDisplay *cdg = vis->cdg_display;
    
    // Calculate scaling to fit widget
    double scale_x = vis->width / (double)CDG_WIDTH;
    double scale_y = vis->height / (double)CDG_HEIGHT;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Center the display
    double offset_x = (vis->width - CDG_WIDTH * scale) / 2.0;
    double offset_y = (vis->height - CDG_HEIGHT * scale) / 2.0;
    
    // Draw CDG graphics
    for (int y = 0; y < CDG_HEIGHT; y++) {
        for (int x = 0; x < CDG_WIDTH; x++) {
            uint8_t color_index = cdg->screen[y][x];
            uint32_t rgb = cdg->palette[color_index];
            
            double r = ((rgb >> 16) & 0xFF) / 255.0;
            double g = ((rgb >> 8) & 0xFF) / 255.0;
            double b = (rgb & 0xFF) / 255.0;
            
            cairo_set_source_rgb(cr, r, g, b);
            cairo_rectangle(cr, 
                          offset_x + x * scale, 
                          offset_y + y * scale, 
                          scale, 
                          scale);
            cairo_fill(cr);
        }
    }
    
    // Draw ball trail
    for (int i = 0; i < 20; i++) {
        int idx = (cdg->ball_trail_index + i) % 20;
        double trail_x = cdg->ball_trail_positions[idx][0];
        double trail_y = cdg->ball_trail_positions[idx][1];
        
        double alpha = i / 20.0 * 0.5;
        double radius = cdg->ball_radius * (0.5 + i / 40.0);
        
        cairo_arc(cr, 
                 offset_x + trail_x * scale, 
                 offset_y + trail_y * scale, 
                 radius * scale, 
                 0, 2 * M_PI);
        cairo_set_source_rgba(cr, 1.0, 0.8, 0.2, alpha);
        cairo_fill(cr);
    }
    
    // Draw karaoke ball with glow
    double ball_x = offset_x + cdg->ball_x * scale;
    double ball_y = offset_y + cdg->ball_y * scale;
    double ball_radius = cdg->ball_radius * scale;
    
    // Outer glow
    cairo_pattern_t *glow = cairo_pattern_create_radial(
        ball_x, ball_y, ball_radius * 0.3,
        ball_x, ball_y, ball_radius * 2.0
    );
    cairo_pattern_add_color_stop_rgba(glow, 0, 1.0, 1.0, 0.5, 0.8);
    cairo_pattern_add_color_stop_rgba(glow, 0.5, 1.0, 0.8, 0.0, 0.4);
    cairo_pattern_add_color_stop_rgba(glow, 1.0, 1.0, 0.5, 0.0, 0.0);
    
    cairo_arc(cr, ball_x, ball_y, ball_radius * 2.0, 0, 2 * M_PI);
    cairo_set_source(cr, glow);
    cairo_fill(cr);
    cairo_pattern_destroy(glow);
    
    // Main ball
    cairo_pattern_t *gradient = cairo_pattern_create_radial(
        ball_x - ball_radius * 0.3, ball_y - ball_radius * 0.3, ball_radius * 0.2,
        ball_x, ball_y, ball_radius
    );
    cairo_pattern_add_color_stop_rgb(gradient, 0, 1.0, 1.0, 0.9);
    cairo_pattern_add_color_stop_rgb(gradient, 0.7, 1.0, 0.9, 0.2);
    cairo_pattern_add_color_stop_rgb(gradient, 1.0, 0.9, 0.6, 0.0);
    
    cairo_arc(cr, ball_x, ball_y, ball_radius, 0, 2 * M_PI);
    cairo_set_source(cr, gradient);
    cairo_fill(cr);
    cairo_pattern_destroy(gradient);
    
    // Highlight
    cairo_arc(cr, 
             ball_x - ball_radius * 0.3, 
             ball_y - ball_radius * 0.3, 
             ball_radius * 0.3, 
             0, 2 * M_PI);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
    cairo_fill(cr);
}
