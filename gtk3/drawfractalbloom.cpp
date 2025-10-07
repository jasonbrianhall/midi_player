#include "visualization.h"
#include <math.h>

void draw_waveform_fractal_bloom(Visualizer *vis, cairo_t *cr)  {
    if (vis->width <= 0 || vis->height <= 0) return;

    int num_shards = VIS_FREQUENCY_BARS;
    double center_x = vis->width / 2.0;
    double center_y = vis->height / 2.0;
    double max_radius = fmin(vis->width, vis->height) * 0.45;

    // Update rotation based on average amplitude
    double avg_amp = 0.0;
    for (int i = 0; i < num_shards; i++) {
        avg_amp += vis->frequency_bands[i];
    }
    avg_amp /= num_shards;
    vis->rotation += avg_amp * 0.03; // tweak factor for speed

    // Apply rotation transform
    cairo_translate(cr, center_x, center_y);
    cairo_rotate(cr, vis->rotation);
    cairo_translate(cr, -center_x, -center_y);

    for (int i = 0; i < num_shards; i++) {
        double angle = ((double)i / num_shards) * 2.0 * M_PI;
        double amplitude = vis->frequency_bands[i];
        double radius = amplitude * max_radius;

        // Shard tip
        double tip_x = center_x + radius * cos(angle);
        double tip_y = center_y + radius * sin(angle);

        // Shard base (wider for bloom effect)
        double base_angle_offset = M_PI / num_shards;
        double base_x1 = center_x + (radius * 0.3) * cos(angle - base_angle_offset);
        double base_y1 = center_y + (radius * 0.3) * sin(angle - base_angle_offset);
        double base_x2 = center_x + (radius * 0.3) * cos(angle + base_angle_offset);
        double base_y2 = center_y + (radius * 0.3) * sin(angle + base_angle_offset);

        // Color explosion: hue based on angle, saturation by amplitude
        double hue = (double)i / num_shards;
        double sat = fmin(1.0, amplitude * 2.0);
        double val = 1.0;

        // Convert HSV to RGB
        double r, g, b;
        int h_i = (int)(hue * 6);
        double f = hue * 6 - h_i;
        double p = val * (1 - sat);
        double q = val * (1 - f * sat);
        double t = val * (1 - (1 - f) * sat);
        switch (h_i % 6) {
            case 0: r = val; g = t; b = p; break;
            case 1: r = q; g = val; b = p; break;
            case 2: r = p; g = val; b = t; break;
            case 3: r = p; g = q; b = val; break;
            case 4: r = t; g = p; b = val; break;
            case 5: r = val; g = p; b = q; break;
        }

        cairo_set_source_rgba(cr, r, g, b, 0.8);
        cairo_move_to(cr, center_x, center_y);
        cairo_line_to(cr, base_x1, base_y1);
        cairo_line_to(cr, tip_x, tip_y);
        cairo_line_to(cr, base_x2, base_y2);
        cairo_close_path(cr);
        cairo_fill(cr);
    }
}

