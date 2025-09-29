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
    
    // Create or update the Cairo surface for CDG graphics
    if (!vis->cdg_surface || 
        cairo_image_surface_get_width(vis->cdg_surface) != CDG_WIDTH ||
        cairo_image_surface_get_height(vis->cdg_surface) != CDG_HEIGHT) {
        
        if (vis->cdg_surface) {
            cairo_surface_destroy(vis->cdg_surface);
        }
        vis->cdg_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, CDG_WIDTH, CDG_HEIGHT);
        vis->cdg_last_packet = -1;
    }
    
    // Always update surface when packets have changed
    if (vis->cdg_last_packet != cdg->current_packet) {
        unsigned char *data = cairo_image_surface_get_data(vis->cdg_surface);
        int stride = cairo_image_surface_get_stride(vis->cdg_surface);
        
        static int debug_count = 0;
        
        // Convert CDG indexed color buffer to RGB surface
        for (int y = 0; y < CDG_HEIGHT; y++) {
            unsigned char *row = data + y * stride;
            for (int x = 0; x < CDG_WIDTH; x++) {
                uint8_t color_index = cdg->screen[y][x];
                uint32_t rgb = cdg->palette[color_index];
                
                // Extract RGB components
                uint8_t r = (rgb >> 16) & 0xFF;
                uint8_t g = (rgb >> 8) & 0xFF;
                uint8_t b = rgb & 0xFF;
                
                // Cairo ARGB32 on little-endian: byte order is B, G, R, A
                int pixel_offset = x * 4;
                row[pixel_offset + 0] = b;   // Blue
                row[pixel_offset + 1] = g;   // Green
                row[pixel_offset + 2] = r;   // Red
                row[pixel_offset + 3] = 255; // Alpha (fully opaque)
            }
        }
        
        cairo_surface_mark_dirty(vis->cdg_surface);
        vis->cdg_last_packet = cdg->current_packet;
    }
    
    // Calculate scaling to fit widget
    double scale_x = vis->width / (double)CDG_WIDTH;
    double scale_y = vis->height / (double)CDG_HEIGHT;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Center the display
    double offset_x = (vis->width - CDG_WIDTH * scale) / 2.0;
    double offset_y = (vis->height - CDG_HEIGHT * scale) / 2.0;
    
    // Draw the CDG surface (scaled)
    cairo_save(cr);
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, vis->cdg_surface, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_paint(cr);
    cairo_restore(cr);
}
