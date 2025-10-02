#include "gameoflife.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "visualization.h"

void init_game_of_life_system(Visualizer *vis) {
    GameOfLifeSystem *gol = &vis->game_of_life;
    
    // Clear the grid
    memset(gol->cells, 0, sizeof(gol->cells));
    memset(gol->next_cells, 0, sizeof(gol->next_cells));
    
    gol->update_timer = 0.0;
    gol->update_interval = 0.1;  // 10 updates per second initially
    gol->spawn_timer = 0.0;
    gol->generation = 0;
    gol->last_spawn_volume = 0.0;
    gol->spawn_threshold = 0.3;
    gol->spawn_frequency_band = 0;
    gol->pulse_intensity = 0.0;
    gol->color_shift = 0.0;
    
    // Seed with a few classic patterns
    // Glider in the middle
    int cx = GOL_GRID_WIDTH / 2;
    int cy = GOL_GRID_HEIGHT / 2;
    gol->cells[cy][cx+1].alive = 1;
    gol->cells[cy+1][cx+2].alive = 1;
    gol->cells[cy+2][cx].alive = 1;
    gol->cells[cy+2][cx+1].alive = 1;
    gol->cells[cy+2][cx+2].alive = 1;
    
    for (int y = 0; y < GOL_GRID_HEIGHT; y++) {
        for (int x = 0; x < GOL_GRID_WIDTH; x++) {
            if (gol->cells[y][x].alive) {
                gol->cells[y][x].hue = 0.5;  // Start with cyan
                gol->cells[y][x].birth_time = 0.0;
            }
        }
    }
}

int gol_count_neighbors(GameOfLifeSystem *gol, int x, int y) {
    int count = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = (x + dx + GOL_GRID_WIDTH) % GOL_GRID_WIDTH;
            int ny = (y + dy + GOL_GRID_HEIGHT) % GOL_GRID_HEIGHT;
            
            if (gol->cells[ny][nx].alive) {
                count++;
            }
        }
    }
    
    return count;
}

void gol_apply_rules(GameOfLifeSystem *gol, double current_time) {
    // Apply Conway's Game of Life rules
    for (int y = 0; y < GOL_GRID_HEIGHT; y++) {
        for (int x = 0; x < GOL_GRID_WIDTH; x++) {
            int neighbors = gol_count_neighbors(gol, x, y);
            int alive = gol->cells[y][x].alive;
            
            // Apply the rules
            if (alive) {
                // Survival: 2 or 3 neighbors
                if (neighbors == 2 || neighbors == 3) {
                    gol->next_cells[y][x].alive = 1;
                    gol->next_cells[y][x].age = gol->cells[y][x].age + 1;
                    gol->next_cells[y][x].hue = gol->cells[y][x].hue;
                    gol->next_cells[y][x].birth_time = gol->cells[y][x].birth_time;
                } else {
                    // Death by isolation or overcrowding
                    gol->next_cells[y][x].alive = 0;
                    gol->next_cells[y][x].age = 0;
                }
            } else {
                // Birth: exactly 3 neighbors
                if (neighbors == 3) {
                    gol->next_cells[y][x].alive = 1;
                    gol->next_cells[y][x].age = 0;
                    gol->next_cells[y][x].birth_time = current_time;
                    
                    // Average the hue of the three parents
                    double hue_sum = 0.0;
                    int parent_count = 0;
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = (x + dx + GOL_GRID_WIDTH) % GOL_GRID_WIDTH;
                            int ny = (y + dy + GOL_GRID_HEIGHT) % GOL_GRID_HEIGHT;
                            if (gol->cells[ny][nx].alive) {
                                hue_sum += gol->cells[ny][nx].hue;
                                parent_count++;
                            }
                        }
                    }
                    gol->next_cells[y][x].hue = hue_sum / parent_count;
                } else {
                    gol->next_cells[y][x].alive = 0;
                    gol->next_cells[y][x].age = 0;
                }
            }
            
            // Cap age
            if (gol->next_cells[y][x].age > GOL_MAX_AGE) {
                gol->next_cells[y][x].age = GOL_MAX_AGE;
            }
        }
    }
    
    // Copy next generation to current
    memcpy(gol->cells, gol->next_cells, sizeof(gol->cells));
    gol->generation++;
}

void gol_spawn_from_audio(Visualizer *vis, int frequency_band, double intensity) {
    GameOfLifeSystem *gol = &vis->game_of_life;
    
    // Map frequency band to position
    // Lower frequencies on left, higher on right
    int x = (frequency_band * GOL_GRID_WIDTH) / VIS_FREQUENCY_BARS;
    
    // Spawn in a small cluster
    int spawn_size = 2 + (int)(intensity * 3);  // 2-5 cells
    
    // Calculate hue from frequency band (rainbow spectrum)
    double hue = (double)frequency_band / VIS_FREQUENCY_BARS;
    
    for (int i = 0; i < spawn_size; i++) {
        int sx = x + (rand() % 5) - 2;  // Random offset
        int sy = rand() % GOL_GRID_HEIGHT;
        
        // Wrap around
        sx = (sx + GOL_GRID_WIDTH) % GOL_GRID_WIDTH;
        
        gol->cells[sy][sx].alive = 1;
        gol->cells[sy][sx].age = 0;
        gol->cells[sy][sx].hue = hue;
        gol->cells[sy][sx].birth_time = vis->time_offset;
    }
    
    gol->pulse_intensity = intensity;
}

gboolean gol_detect_beat(Visualizer *vis) {
    GameOfLifeSystem *gol = &vis->game_of_life;
    
    // Simple beat detection based on volume spike
    if (vis->volume_level > gol->spawn_threshold && 
        vis->volume_level > gol->last_spawn_volume * 1.5) {
        gol->last_spawn_volume = vis->volume_level;
        return TRUE;
    }
    
    gol->last_spawn_volume = vis->volume_level * 0.95 + gol->last_spawn_volume * 0.05;
    return FALSE;
}

void update_game_of_life(Visualizer *vis, double dt) {
    GameOfLifeSystem *gol = &vis->game_of_life;
    
    gol->update_timer += dt;
    gol->spawn_timer += dt;
    gol->color_shift += dt * 0.1;
    
    // Adjust update speed based on volume (faster when louder)
    double tempo_factor = 1.0 + vis->volume_level * 2.0;
    double adjusted_interval = gol->update_interval / tempo_factor;
    
    // Update the game state
    if (gol->update_timer >= adjusted_interval) {
        gol_apply_rules(gol, vis->time_offset);
        gol->update_timer = 0.0;
    }
    
    // Spawn new cells on beats
    if (gol->spawn_timer >= 0.1 && gol_detect_beat(vis)) {
        // Find the frequency band with highest energy
        int max_band = 0;
        double max_energy = 0.0;
        for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
            if (vis->frequency_bands[i] > max_energy) {
                max_energy = vis->frequency_bands[i];
                max_band = i;
            }
        }
        
        gol_spawn_from_audio(vis, max_band, max_energy);
        gol->spawn_timer = 0.0;
    }
    
    // Decay pulse
    gol->pulse_intensity *= 0.95;
}

void draw_game_of_life(Visualizer *vis, cairo_t *cr) {
    GameOfLifeSystem *gol = &vis->game_of_life;
    
    double cell_width = (double)vis->width / GOL_GRID_WIDTH;
    double cell_height = (double)vis->height / GOL_GRID_HEIGHT;
    
    // Draw background with subtle grid
    cairo_set_source_rgb(cr, 0.05, 0.05, 0.1);
    cairo_paint(cr);
    
    // Draw very faint grid lines
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.3);
    cairo_set_line_width(cr, 0.5);
    for (int x = 0; x <= GOL_GRID_WIDTH; x++) {
        cairo_move_to(cr, x * cell_width, 0);
        cairo_line_to(cr, x * cell_width, vis->height);
    }
    for (int y = 0; y <= GOL_GRID_HEIGHT; y++) {
        cairo_move_to(cr, 0, y * cell_height);
        cairo_line_to(cr, vis->width, y * cell_height);
    }
    cairo_stroke(cr);
    
    // Draw cells
    for (int y = 0; y < GOL_GRID_HEIGHT; y++) {
        for (int x = 0; x < GOL_GRID_WIDTH; x++) {
            if (gol->cells[y][x].alive) {
                double cell_x = x * cell_width;
                double cell_y = y * cell_height;
                
                // Age-based brightness (older cells are brighter)
                double age_factor = (double)gol->cells[y][x].age / GOL_MAX_AGE;
                age_factor = sqrt(age_factor);  // Non-linear scaling
                
                // Birth animation (cells grow in)
                double birth_anim = vis->time_offset - gol->cells[y][x].birth_time;
                double scale = 1.0;
                if (birth_anim < 0.2) {
                    scale = birth_anim / 0.2;
                }
                
                // Convert hue to RGB with saturation and value
                double hue = fmod(gol->cells[y][x].hue + gol->color_shift * 0.1, 1.0);
                double r, g, b;
                hsv_to_rgb(hue, 0.8, 0.3 + age_factor * 0.7, &r, &g, &b);
                
                // Add pulse effect on beats
                if (gol->pulse_intensity > 0.1) {
                    double pulse = gol->pulse_intensity * 0.3;
                    r += pulse;
                    g += pulse;
                    b += pulse;
                }
                
                cairo_set_source_rgb(cr, r, g, b);
                
                // Draw cell with slight padding and rounded corners
                double padding = 1.0;
                double radius = cell_width * 0.15 * scale;
                double cx = cell_x + padding;
                double cy = cell_y + padding;
                double cw = cell_width - padding * 2;
                double ch = cell_height - padding * 2;
                
                // Scale from center
                if (scale < 1.0) {
                    double center_x = cx + cw / 2;
                    double center_y = cy + ch / 2;
                    cw *= scale;
                    ch *= scale;
                    cx = center_x - cw / 2;
                    cy = center_y - ch / 2;
                    radius *= scale;
                }
                
                // Rounded rectangle
                cairo_new_sub_path(cr);
                cairo_arc(cr, cx + radius, cy + radius, radius, M_PI, 3 * M_PI / 2);
                cairo_arc(cr, cx + cw - radius, cy + radius, radius, 3 * M_PI / 2, 2 * M_PI);
                cairo_arc(cr, cx + cw - radius, cy + ch - radius, radius, 0, M_PI / 2);
                cairo_arc(cr, cx + radius, cy + ch - radius, radius, M_PI / 2, M_PI);
                cairo_close_path(cr);
                cairo_fill(cr);
                
                // Add glow for recently born cells
                if (birth_anim < 0.5) {
                    double glow_alpha = (0.5 - birth_anim) / 0.5 * 0.5;
                    cairo_set_source_rgba(cr, r, g, b, glow_alpha);
                    cairo_arc(cr, cell_x + cell_width / 2, 
                             cell_y + cell_height / 2, 
                             cell_width * 0.8, 0, 2 * M_PI);
                    cairo_fill(cr);
                }
            }
        }
    }
    
    // Draw generation counter
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    cairo_set_source_rgba(cr, 0.4, 0.8, 1.0, 0.6);
    
    char gen_text[64];
    snprintf(gen_text, sizeof(gen_text), "Generation: %d", gol->generation);
    cairo_move_to(cr, 10, 25);
    cairo_show_text(cr, gen_text);
}
