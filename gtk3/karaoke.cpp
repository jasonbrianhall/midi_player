#include "visualization.h"
#include "cdg.h"
#include <cairo.h>
#include <string.h>

// Apply multi-pass smoothing for better anti-aliasing
static void apply_smooth_filter(unsigned char *data, int width, int height, int stride) {
    unsigned char *temp = (unsigned char*)malloc(height * stride);
    
    // First pass: horizontal blur
    memcpy(temp, data, height * stride);
    for (int y = 0; y < height; y++) {
        for (int x = 1; x < width - 1; x++) {
            int offset = y * stride + x * 4;
            for (int c = 0; c < 3; c++) {
                int left = temp[offset - 4 + c];
                int center = temp[offset + c];
                int right = temp[offset + 4 + c];
                data[offset + c] = (left + center * 2 + right) / 4;
            }
        }
    }
    
    // Second pass: vertical blur
    memcpy(temp, data, height * stride);
    for (int y = 1; y < height - 1; y++) {
        for (int x = 0; x < width; x++) {
            int offset = y * stride + x * 4;
            for (int c = 0; c < 3; c++) {
                int top = temp[offset - stride + c];
                int center = temp[offset + c];
                int bottom = temp[offset + stride + c];
                data[offset + c] = (top + center * 2 + bottom) / 4;
            }
        }
    }
    
    free(temp);
}

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
    
    // Create or update the Cairo surface for CDG graphics (2x size for better quality)
    int render_width = CDG_WIDTH * 2;
    int render_height = CDG_HEIGHT * 2;
    
    if (!vis->cdg_surface || 
        cairo_image_surface_get_width(vis->cdg_surface) != render_width ||
        cairo_image_surface_get_height(vis->cdg_surface) != render_height) {
        
        if (vis->cdg_surface) {
            cairo_surface_destroy(vis->cdg_surface);
        }
        vis->cdg_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, render_width, render_height);
        vis->cdg_last_packet = -1;
    }
    
    // Always update surface when packets have changed
    if (vis->cdg_last_packet != cdg->current_packet) {
        unsigned char *data = cairo_image_surface_get_data(vis->cdg_surface);
        int stride = cairo_image_surface_get_stride(vis->cdg_surface);
        
        // Convert CDG indexed color buffer to RGB surface with 2x upscaling
        for (int y = 0; y < CDG_HEIGHT; y++) {
            for (int x = 0; x < CDG_WIDTH; x++) {
                uint8_t color_index = cdg->screen[y][x];
                uint32_t rgb = cdg->palette[color_index];
                
                // Extract RGB components
                uint8_t r = (rgb >> 16) & 0xFF;
                uint8_t g = (rgb >> 8) & 0xFF;
                uint8_t b = rgb & 0xFF;
                
                // Write to 2x2 block for upscaling
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        int dest_y = y * 2 + dy;
                        int dest_x = x * 2 + dx;
                        unsigned char *row = data + dest_y * stride;
                        int pixel_offset = dest_x * 4;
                        
                        row[pixel_offset + 0] = b;   // Blue
                        row[pixel_offset + 1] = g;   // Green
                        row[pixel_offset + 2] = r;   // Red
                        row[pixel_offset + 3] = 255; // Alpha
                    }
                }
            }
        }
        
        // Apply smoothing filter for anti-aliasing
        apply_smooth_filter(data, render_width, render_height, stride);
        
        cairo_surface_mark_dirty(vis->cdg_surface);
        vis->cdg_last_packet = cdg->current_packet;
    }
    
    // Draw black background
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    
    // Calculate scaling to fit widget
    double scale_x = vis->width / (double)render_width;
    double scale_y = vis->height / (double)render_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Center the display
    double offset_x = (vis->width - render_width * scale) / 2.0;
    double offset_y = (vis->height - render_height * scale) / 2.0;
    
    // Draw the CDG surface with high-quality filtering
    cairo_save(cr);
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, vis->cdg_surface, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BEST);
    cairo_paint(cr);
    cairo_restore(cr);
}
