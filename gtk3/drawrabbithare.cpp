#include "visualization.h"
#include <math.h>

// Static variables for animation state
static double time_offset = 0;
static double turtle_x = 50;
static double hare_x = 50;
static double hare_sleep_timer = 0;
static bool hare_sleeping = false;
static double finish_line_x = 0;
static bool race_finished = false;
static int winner = 0; // 0=none, 1=turtle, 2=hare
static double celebration_time = 0;
static double cloud_x[8] = {0};
static double cloud_y[8] = {0};
static bool clouds_initialized = false;

void update_rabbithare(Visualizer *vis, double dt) {
    const double min_dt = 1.0 / 120.0;
    double speed_factor = dt / 0.033;
    
    if (dt < min_dt) {
        dt = min_dt;
        speed_factor = dt / 0.033;
    }
    
    time_offset += 0.05 * speed_factor;
    
    // Initialize clouds
    if (!clouds_initialized) {
        for (int i = 0; i < 8; i++) {
            cloud_x[i] = (rand() % vis->width);
            cloud_y[i] = 30 + (rand() % 80);
        }
        clouds_initialized = true;
    }
    
    // Update clouds
    for (int i = 0; i < 8; i++) {
        cloud_x[i] += 0.3 * speed_factor;
        if (cloud_x[i] > vis->width + 80) {
            cloud_x[i] = -80;
            cloud_y[i] = 30 + (rand() % 80);
        }
    }
    
    // Set finish line
    finish_line_x = vis->width - 100;
    
    // Calculate speed multiplier based on screen width
    // 400px = 0.2x, 800px = 0.4x, 1600px = 0.8x
    double screen_speed_factor = vis->width / 2000.0;
    if (screen_speed_factor < 0.2) screen_speed_factor = 0.2; // Minimum speed
    if (screen_speed_factor > 0.8) screen_speed_factor = 0.8; // Maximum speed
    
    if (!race_finished) {
        // Calculate average energy from frequency bands
        double avg_energy = 0;
        for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
            avg_energy += vis->frequency_bands[i];
        }
        avg_energy /= VIS_FREQUENCY_BARS;
        
        // Turtle moves steadily, slightly faster with music (slower than before)
        turtle_x += (1.8 + avg_energy * 0.6) * speed_factor * screen_speed_factor;
        
        // Hare behavior based on music energy
        if (hare_sleeping) {
            hare_sleep_timer -= dt;
            
            // Very small chance of waking if turtle passes - turtle makes noises!
            double lead = hare_x - turtle_x;
            double wake_chance = 0;
            
            if (lead < -30) {
                wake_chance = 0.03; // 3% chance if turtle is way ahead
            } else if (lead < -10) {
                wake_chance = 0.02; // 2% chance if turtle is ahead
            } else if (lead < 0) {
                wake_chance = 0.01; // 1% chance if turtle just passed
            }
            
            bool noise_wake = (rand() % 1000) < (wake_chance * 1000);
            
            // Hare is a DEEP sleeper - mostly only wakes up when timer runs out
            if (hare_sleep_timer <= 0 || noise_wake) {
                hare_sleeping = false;
            }
        } else {
            // Hare runs fast when music is energetic (faster than before)
            if (avg_energy > 0.35) {
                hare_x += (6.0 + avg_energy * 3.0) * speed_factor * screen_speed_factor;
            } else {
                // Better minimum speed to stay competitive
                hare_x += 2.5 * speed_factor * screen_speed_factor;
            }
            
            // Calculate how far ahead the hare is
            double lead = hare_x - turtle_x;
            
            // Hare gets more confident and lazy the further ahead it is
            // Uses exponential scaling - barely ahead = very cautious, far ahead = very lazy
            double confidence_factor = 1.0;
            double sleep_time_bonus = 0;
            
            if (lead > 150) {
                confidence_factor = 10.0; // Way ahead - extremely overconfident
                sleep_time_bonus = 6.0;   // Very long naps
            } else if (lead > 100) {
                confidence_factor = 7.0;  // Far ahead - very lazy
                sleep_time_bonus = 5.0;
            } else if (lead > 60) {
                confidence_factor = 5.0;  // Comfortable lead - quite lazy
                sleep_time_bonus = 4.0;
            } else if (lead > 30) {
                confidence_factor = 3.0;  // Moderate lead - somewhat lazy
                sleep_time_bonus = 3.0;
            } else if (lead > 15) {
                confidence_factor = 1.5;  // Small lead - bit cautious
                sleep_time_bonus = 1.5;
            } else if (lead > 5) {
                confidence_factor = 0.5;  // Barely ahead - very cautious
                sleep_time_bonus = 0.5;
            }
            // If lead <= 5, confidence_factor stays at 1.0 (minimal sleep chance)
            
            // Base sleep chance is lower, but scales dramatically with confidence
            double sleep_chance = 3 * confidence_factor;  // Was 20
            
            // Hare only sleeps when ahead and conditions are right
            double energy_threshold = 0.50;
            if (lead > 80) energy_threshold = 0.60; // Can sleep even with more energetic music
            
            if (avg_energy < energy_threshold && lead > 5 && (rand() % 100) < sleep_chance) {
                hare_sleeping = true;
                hare_sleep_timer = 2.0 + sleep_time_bonus + (rand() % 2); // 2-4 base + bonus
            }
        }
        
        // Check for winner
        if (turtle_x >= finish_line_x && winner == 0) {
            winner = 1;
            race_finished = true;
            celebration_time = 3.0;
        } else if (hare_x >= finish_line_x && winner == 0) {
            winner = 2;
            race_finished = true;
            celebration_time = 3.0;
        }
    } else {
        celebration_time -= dt;
        if (celebration_time <= 0) {
            // Reset race
            turtle_x = 50;
            hare_x = 50;
            hare_sleeping = false;
            hare_sleep_timer = 0;
            race_finished = false;
            winner = 0;
            celebration_time = 0;
        }
    }
}

void draw_rabbithare(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Sky gradient background
    cairo_pattern_t *sky = cairo_pattern_create_linear(0, 0, 0, vis->height);
    cairo_pattern_add_color_stop_rgb(sky, 0, 0.4, 0.7, 1.0);
    cairo_pattern_add_color_stop_rgb(sky, 0.7, 0.6, 0.85, 1.0);
    cairo_pattern_add_color_stop_rgb(sky, 1.0, 0.5, 0.8, 0.6);
    cairo_set_source(cr, sky);
    cairo_paint(cr);
    cairo_pattern_destroy(sky);
    
    // Draw clouds
    for (int i = 0; i < 8; i++) {
        cairo_set_source_rgba(cr, 1, 1, 1, 0.7);
        cairo_arc(cr, cloud_x[i], cloud_y[i], 20, 0, 6.28);
        cairo_fill(cr);
        cairo_arc(cr, cloud_x[i] + 15, cloud_y[i] - 5, 15, 0, 6.28);
        cairo_fill(cr);
        cairo_arc(cr, cloud_x[i] + 25, cloud_y[i], 18, 0, 6.28);
        cairo_fill(cr);
    }
    
    // Ground
    double ground_y = vis->height * 0.7;
    cairo_set_source_rgb(cr, 0.4, 0.7, 0.3);
    cairo_rectangle(cr, 0, ground_y, vis->width, vis->height - ground_y);
    cairo_fill(cr);
    
    // Grass texture
    cairo_set_source_rgb(cr, 0.3, 0.6, 0.25);
    for (int gx = 0; gx < vis->width; gx += 10) {
        for (int i = 0; i < 3; i++) {
            double grass_x = gx + (rand() % 8);
            double grass_y = ground_y + (rand() % 30);
            cairo_move_to(cr, grass_x, grass_y);
            cairo_line_to(cr, grass_x, grass_y - 5 - (rand() % 5));
            cairo_set_line_width(cr, 1);
            cairo_stroke(cr);
        }
    }
    
    // Race track
    cairo_set_source_rgb(cr, 0.6, 0.5, 0.4);
    cairo_rectangle(cr, 0, ground_y - 60, vis->width, 80);
    cairo_fill(cr);
    
    // Track lines
    cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 0, ground_y - 20);
    cairo_line_to(cr, vis->width, ground_y - 20);
    cairo_stroke(cr);
    
    // Dashed center line
    cairo_set_dash(cr, (double[]){10, 10}, 2, 0);
    cairo_move_to(cr, 0, ground_y - 40);
    cairo_line_to(cr, vis->width, ground_y - 40);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
    
    // Start line
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 40, ground_y - 60, 5, 80);
    cairo_fill(cr);
    
    // Finish line (checkered)
    for (int i = 0; i < 8; i++) {
        if (i % 2 == 0) {
            cairo_set_source_rgb(cr, 0, 0, 0);
        } else {
            cairo_set_source_rgb(cr, 1, 1, 1);
        }
        cairo_rectangle(cr, finish_line_x, ground_y - 60 + i * 10, 10, 10);
        cairo_fill(cr);
    }
    
    // Draw frequency bars behind racers
    double bar_width = (double)vis->width / VIS_FREQUENCY_BARS;
    // Scale bar height based on screen height - taller screens get taller bars
    double max_bar_height = vis->height * 0.35; // Use 35% of screen height
    if (max_bar_height < 60) max_bar_height = 60; // Minimum height
    
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        double height = vis->frequency_bands[i] * max_bar_height;
        double x = i * bar_width;
        double y = ground_y - 60 - height;
        
        double hue = (double)i / VIS_FREQUENCY_BARS;
        double r = 0.5 + 0.5 * sin(hue * 6.28);
        double g = 0.5 + 0.5 * sin(hue * 6.28 + 2.09);
        double b = 0.5 + 0.5 * sin(hue * 6.28 + 4.18);
        
        cairo_set_source_rgba(cr, r, g, b, 0.3);
        cairo_rectangle(cr, x, y, bar_width - 1, height);
        cairo_fill(cr);
    }
    
    // Draw Turtle
    double turtle_y = ground_y - 45;
    cairo_save(cr);
    cairo_translate(cr, turtle_x, turtle_y);
    
    // Shell
    cairo_set_source_rgb(cr, 0.4, 0.6, 0.3);
    cairo_arc(cr, 0, 0, 18, 0, 6.28);
    cairo_fill(cr);
    
    // Shell pattern
    cairo_set_source_rgb(cr, 0.3, 0.5, 0.25);
    cairo_arc(cr, 0, 0, 12, 0, 6.28);
    cairo_fill(cr);
    for (int i = 0; i < 6; i++) {
        double angle = i * 1.047;
        cairo_arc(cr, cos(angle) * 8, sin(angle) * 8, 4, 0, 6.28);
        cairo_fill(cr);
    }
    
    // Head (moving back and forth)
    double head_bob = sin(time_offset * 3) * 2;
    cairo_set_source_rgb(cr, 0.5, 0.7, 0.4);
    cairo_arc(cr, 20 + head_bob, -5, 8, 0, 6.28);
    cairo_fill(cr);
    
    // Eye
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_arc(cr, 23 + head_bob, -7, 2, 0, 6.28);
    cairo_fill(cr);
    
    // Legs
    cairo_set_source_rgb(cr, 0.5, 0.7, 0.4);
    cairo_set_line_width(cr, 4);
    double leg_swing = sin(time_offset * 4) * 3;
    cairo_move_to(cr, -8, 12);
    cairo_line_to(cr, -12, 18 + leg_swing);
    cairo_stroke(cr);
    cairo_move_to(cr, 8, 12);
    cairo_line_to(cr, 12, 18 - leg_swing);
    cairo_stroke(cr);
    
    cairo_restore(cr);
    
    // Draw Hare
    double hare_y = ground_y - 50;
    cairo_save(cr);
    cairo_translate(cr, hare_x, hare_y);
    
    if (hare_sleeping) {
        // Sleeping hare
        cairo_set_source_rgb(cr, 0.7, 0.6, 0.5);
        
        // Body (curled up)
        cairo_arc(cr, 0, 5, 20, 0, 6.28);
        cairo_fill(cr);
        
        // Head
        cairo_arc(cr, 15, 0, 12, 0, 6.28);
        cairo_fill(cr);
        
        // Ears (floppy)
        cairo_arc(cr, 20, -8, 8, 0, 6.28);
        cairo_fill(cr);
        cairo_arc(cr, 28, -6, 7, 0, 6.28);
        cairo_fill(cr);
        
        // Closed eyes
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, 18, -2);
        cairo_line_to(cr, 22, -2);
        cairo_stroke(cr);
        
        // ZZZ
        cairo_set_source_rgba(cr, 0.3, 0.3, 0.5, 0.7);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 20);
        double z_bob = sin(time_offset * 2) * 5;
        cairo_move_to(cr, 30, -15 + z_bob);
        cairo_show_text(cr, "Z");
        cairo_move_to(cr, 40, -25 + z_bob * 0.7);
        cairo_show_text(cr, "Z");
        cairo_move_to(cr, 50, -35 + z_bob * 0.5);
        cairo_show_text(cr, "Z");
    } else {
        // Running hare
        cairo_set_source_rgb(cr, 0.7, 0.6, 0.5);
        
        // Body (stretched)
        cairo_save(cr);
        cairo_scale(cr, 1.3, 0.8);
        cairo_arc(cr, 0, 0, 15, 0, 6.28);
        cairo_fill(cr);
        cairo_restore(cr);
        
        // Head
        cairo_arc(cr, 18, -8, 10, 0, 6.28);
        cairo_fill(cr);
        
        // Ears (upright)
        cairo_arc(cr, 20, -18, 6, 0, 6.28);
        cairo_fill(cr);
        cairo_arc(cr, 28, -20, 5, 0, 6.28);
        cairo_fill(cr);
        
        // Eye
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_arc(cr, 22, -10, 2, 0, 6.28);
        cairo_fill(cr);
        
        // Running legs
        cairo_set_source_rgb(cr, 0.7, 0.6, 0.5);
        cairo_set_line_width(cr, 3);
        double run_cycle = sin(time_offset * 8) * 8;
        cairo_move_to(cr, -10, 8);
        cairo_line_to(cr, -15, 18 + run_cycle);
        cairo_stroke(cr);
        cairo_move_to(cr, 5, 8);
        cairo_line_to(cr, 8, 18 - run_cycle);
        cairo_stroke(cr);
        
        // Speed lines
        cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 0.4);
        cairo_set_line_width(cr, 2);
        for (int i = 0; i < 3; i++) {
            cairo_move_to(cr, -25 - i * 8, -5 + i * 3);
            cairo_line_to(cr, -35 - i * 8, -5 + i * 3);
            cairo_stroke(cr);
        }
    }
    
    cairo_restore(cr);
    
    // Draw winner announcement
    if (race_finished && celebration_time > 0) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
        cairo_rectangle(cr, vis->width/2 - 150, vis->height/2 - 50, 300, 100);
        cairo_fill(cr);
        
        cairo_set_source_rgb(cr, 1, 1, 0.2);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 36);
        
        if (winner == 1) {
            cairo_move_to(cr, vis->width/2 - 120, vis->height/2);
            cairo_show_text(cr, "TURTLE WINS!");
        } else {
            cairo_move_to(cr, vis->width/2 - 100, vis->height/2);
            cairo_show_text(cr, "HARE WINS!");
        }
        
        cairo_set_font_size(cr, 18);
        cairo_move_to(cr, vis->width/2 - 80, vis->height/2 + 30);
        cairo_show_text(cr, "Race restarting...");
    }
}
