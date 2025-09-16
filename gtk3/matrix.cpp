#ifndef MATRIX_H
#define MATRIX_H

#include "visualization.h"

// Enhanced ASCII character sets for more variety
const char* get_random_matrix_char(void) {
    static const char* matrix_chars[] = {
        // Numbers (weighted more heavily for that digital feel)
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", // Doubled for higher frequency
        // Uppercase letters
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
        "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
        "U", "V", "W", "X", "Y", "Z",
        // Lowercase letters
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
        "k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
        "u", "v", "w", "x", "y", "z",
        // Special symbols and tech characters
        "!", "@", "#", "$", "%", "^", "&", "*", "(", ")",
        "[", "]", "{", "}", "|", "\\", "/", "?", "<", ">",
        "=", "+", "-", "_", "~", "`", ":", ";", ".", ",",
        // Extra digital/matrix-style characters
        "§", "±", "°", "µ", "π", "Σ", "Ω", "∞", "≈", "≠",
        "≤", "≥", "÷", "×", "√", "∫", "∆", "∇", "∂", "∑"
    };
    
    int num_chars = sizeof(matrix_chars) / sizeof(matrix_chars[0]);
    int index = rand() % num_chars;
    return matrix_chars[index];
}

// Get special "power" characters for intense moments
static const char* get_power_matrix_char(void) {
    static const char* power_chars[] = {
        "★", "※", "◆", "◇", "◈", "◉", "◎", "●", "○", "◐",
        "◑", "◒", "◓", "▲", "△", "▼", "▽", "◄", "►", "♦",
        "♠", "♣", "♥", "♪", "♫", "☆", "✦", "✧", "✩", "✪"
    };
    
    int num_chars = sizeof(power_chars) / sizeof(power_chars[0]);
    int index = rand() % num_chars;
    return power_chars[index];
}

void init_matrix_system(Visualizer *vis) {
    vis->matrix_column_count = 0;
    vis->matrix_spawn_timer = 0.0;
    vis->matrix_char_size = 12;
    
    // Initialize all columns as inactive
    for (int i = 0; i < MAX_MATRIX_COLUMNS; i++) {
        vis->matrix_columns[i].active = FALSE;
        vis->matrix_columns[i].x = 0;
        vis->matrix_columns[i].y = 0;
        vis->matrix_columns[i].speed = 0;
        vis->matrix_columns[i].length = 0;
        vis->matrix_columns[i].intensity = 0;
        vis->matrix_columns[i].frequency_band = 0;
    }
}

void create_matrix_column_at_position(Visualizer *vis, int x_position) {
    if (vis->matrix_column_count >= MAX_MATRIX_COLUMNS) return;
    
    // Find inactive column slot
    int slot = -1;
    for (int i = 0; i < MAX_MATRIX_COLUMNS; i++) {
        if (!vis->matrix_columns[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return;
    
    MatrixColumn *col = &vis->matrix_columns[slot];
    
    // Set the exact X position provided
    col->x = x_position;
    
    // Make sure we don't go off screen
    if (col->x < 0) col->x = 0;
    if (col->x > vis->width - vis->matrix_char_size) {
        col->x = vis->width - vis->matrix_char_size;
    }
    
    // Start above screen
    col->y = -vis->matrix_char_size * (1 + rand() % 3);
    
    // Random properties
    col->speed = 50 + (rand() % 150);
    col->intensity = 0.4 + (rand() / (double)RAND_MAX) * 0.6;
    col->length = 8 + (rand() % 18);
    if (col->length > MAX_CHARS_PER_COLUMN) col->length = MAX_CHARS_PER_COLUMN;
    
    // Random frequency band for audio reactivity
    col->frequency_band = rand() % VIS_FREQUENCY_BARS;
    
    // Generate characters
    for (int i = 0; i < col->length; i++) {
        col->chars[i] = get_random_matrix_char();
        double position_factor = 1.0 - (double)i / col->length;
        col->char_ages[i] = position_factor * position_factor;
    }
    
    col->active = TRUE;
    vis->matrix_column_count++;
}

void update_matrix(Visualizer *vis, double dt) {
    vis->matrix_spawn_timer += dt;
    
    // Calculate audio energy
    double total_energy = 0.0;
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        total_energy += vis->frequency_bands[i];
    }
    total_energy /= VIS_FREQUENCY_BARS;
    
    // SYSTEMATIC spawning across the screen width
    static int screen_section = 0;
    const int NUM_SECTIONS = 8; // Divide screen into sections for even distribution
    
    if (vis->matrix_spawn_timer > 0.1) { // Spawn every 0.1 seconds
        
        // Always spawn at least 2-4 columns in different sections
        int columns_to_spawn = 2 + (rand() % 3);
        
        for (int spawn = 0; spawn < columns_to_spawn; spawn++) {
            // Cycle through screen sections to ensure even distribution
            screen_section = (screen_section + 1) % NUM_SECTIONS;
            
            // Calculate X position in this section
            int section_width = vis->width / NUM_SECTIONS;
            int section_start = screen_section * section_width;
            int x_pos = section_start + (rand() % section_width);
            
            create_matrix_column_at_position(vis, x_pos);
        }
        
        // Additional random spawning for variety
        for (int random_spawn = 0; random_spawn < 2; random_spawn++) {
            int random_x = rand() % vis->width;
            create_matrix_column_at_position(vis, random_x);
        }
        
        vis->matrix_spawn_timer = 0.0;
    }
    
    // Audio-reactive burst spawning
    if (total_energy > 0.3) {
        int burst_columns = (int)(total_energy * 5);
        for (int burst = 0; burst < burst_columns; burst++) {
            int random_x = rand() % vis->width;
            create_matrix_column_at_position(vis, random_x);
        }
    }
    
    // Update existing columns
    for (int i = 0; i < MAX_MATRIX_COLUMNS; i++) {
        if (!vis->matrix_columns[i].active) continue;
        
        MatrixColumn *col = &vis->matrix_columns[i];
        
        // Update position
        col->y += col->speed * dt;
        
        // Update intensity based on audio
        double current_audio = vis->frequency_bands[col->frequency_band];
        col->intensity = fmax(col->intensity * 0.98, 0.3 + current_audio * 0.7);
        
        // Character morphing
        if (rand() / (double)RAND_MAX < dt * 4.0) {
            int char_to_change = rand() % col->length;
            if (current_audio > 0.6 && char_to_change < 3 && (rand() % 3 == 0)) {
                col->chars[char_to_change] = get_power_matrix_char();
            } else {
                col->chars[char_to_change] = get_random_matrix_char();
            }
        }
        
        // Age characters
        for (int j = 0; j < col->length; j++) {
            double fade_rate = (j < 3) ? 0.4 : 0.8;
            col->char_ages[j] -= dt * fade_rate;
            if (col->char_ages[j] < 0) col->char_ages[j] = 0;
        }
        
        // Remove when off screen
        if (col->y > vis->height + col->length * vis->matrix_char_size) {
            col->active = FALSE;
            vis->matrix_column_count--;
        }
    }
}

void draw_matrix(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Initialize matrix system on first call
    static gboolean matrix_initialized = FALSE;
    if (!matrix_initialized) {
        init_matrix_system(vis);
        matrix_initialized = TRUE;
    }
    
    update_matrix(vis, 0.033); // ~30 FPS
    
    // Subtle background grid
    cairo_set_source_rgba(cr, 0.0, 0.1, 0.0, 0.2);
    cairo_set_line_width(cr, 0.5);
    for (int x = 0; x < vis->width; x += vis->matrix_char_size * 3) {
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, vis->height);
        cairo_stroke(cr);
    }
    
    // Set up font
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, vis->matrix_char_size);
    
    // Draw all active columns
    for (int i = 0; i < MAX_MATRIX_COLUMNS; i++) {
        if (!vis->matrix_columns[i].active) continue;
        
        MatrixColumn *col = &vis->matrix_columns[i];
        
        // Draw each character in the column
        for (int j = 0; j < col->length; j++) {
            double char_y = col->y - j * vis->matrix_char_size;
            
            // Skip if off screen
            if (char_y < -vis->matrix_char_size || char_y > vis->height + vis->matrix_char_size) continue;
            
            double brightness = col->char_ages[j] * col->intensity;
            if (brightness < 0.05) continue;
            
            // Color based on position in column
            if (j == 0 && brightness > 0.7) {
                // Bright head
                cairo_set_source_rgba(cr, 0.9, 1.0, 0.9, brightness);
            } else {
                // Matrix green
                cairo_set_source_rgba(cr, 0, brightness, 0, brightness);
            }
            
            // Draw character
            cairo_move_to(cr, col->x, char_y);
            cairo_show_text(cr, col->chars[j]);
            
            // Glow effect for bright characters
            if (brightness > 0.6) {
                cairo_set_source_rgba(cr, 0, brightness * 0.5, 0, brightness * 0.3);
                cairo_move_to(cr, col->x - 1, char_y);
                cairo_show_text(cr, col->chars[j]);
                cairo_move_to(cr, col->x + 1, char_y);
                cairo_show_text(cr, col->chars[j]);
            }
        }
    }
    
    // Audio-reactive particles
    int particle_count = 20 + (int)(vis->volume_level * 40);
    for (int i = 0; i < particle_count; i++) {
        double x = fmod(vis->time_offset * 30 + i * 47, vis->width);
        double y = fmod(vis->time_offset * 25 + i * 83, vis->height);
        double alpha = (sin(vis->time_offset * 3 + i) + 1) * 0.1 + vis->volume_level * 0.2;
        
        cairo_set_source_rgba(cr, 0, 0.6, 0, alpha);
        cairo_arc(cr, x, y, 1.0 + vis->volume_level * 2.0, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // Screen flash for intense audio
    if (vis->volume_level > 0.4) {
        double flash_alpha = (vis->volume_level - 0.4) * 0.1;
        cairo_set_source_rgba(cr, 0, 1.0, 0.2, flash_alpha);
        cairo_rectangle(cr, 0, 0, vis->width, vis->height);
        cairo_fill(cr);
    }
    
    // Scanning line effect
    static double scan_y = 0;
    scan_y += vis->height * 0.008;
    if (scan_y > vis->height) scan_y = 0;
    
    cairo_set_source_rgba(cr, 0, 0.7, 0.3, 0.15);
    cairo_rectangle(cr, 0, scan_y, vis->width, 2);
    cairo_fill(cr);
}

#endif
