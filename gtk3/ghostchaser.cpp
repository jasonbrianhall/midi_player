#include "visualization.h"

void init_ghost_chaser_system(Visualizer *vis) {
    // Initialize ghost colors (different from classic to avoid IP issues)
    // Red ghost
    vis->ghost_chaser_ghost_colors[0][0] = 1.0; vis->ghost_chaser_ghost_colors[0][1] = 0.0; vis->ghost_chaser_ghost_colors[0][2] = 0.0;
    // Green ghost 
    vis->ghost_chaser_ghost_colors[1][0] = 0.0; vis->ghost_chaser_ghost_colors[1][1] = 1.0; vis->ghost_chaser_ghost_colors[1][2] = 0.0;
    // Blue ghost
    vis->ghost_chaser_ghost_colors[2][0] = 0.0; vis->ghost_chaser_ghost_colors[2][1] = 0.0; vis->ghost_chaser_ghost_colors[2][2] = 1.0;
    // Purple ghost
    vis->ghost_chaser_ghost_colors[3][0] = 0.8; vis->ghost_chaser_ghost_colors[3][1] = 0.0; vis->ghost_chaser_ghost_colors[3][2] = 0.8;
    
    // Initialize player (now called "chaser")
    vis->ghost_chaser_player.grid_x = 12;
    vis->ghost_chaser_player.grid_y = 11;
    vis->ghost_chaser_player.x = vis->ghost_chaser_player.grid_x;
    vis->ghost_chaser_player.y = vis->ghost_chaser_player.grid_y;
    vis->ghost_chaser_player.direction = CHASER_RIGHT;
    vis->ghost_chaser_player.next_direction = CHASER_RIGHT;
    vis->ghost_chaser_player.mouth_angle = 0.0;
    vis->ghost_chaser_player.size_multiplier = 1.0;
    vis->ghost_chaser_player.moving = TRUE;
    vis->ghost_chaser_player.speed = 3.0;
    vis->ghost_chaser_player.beat_pulse = 0.0;
    
    // Initialize ghosts
    vis->ghost_chaser_ghost_count = 4;
    int ghost_positions[4][2] = {{12, 7}, {11, 7}, {13, 7}, {12, 8}};
    
    for (int i = 0; i < vis->ghost_chaser_ghost_count; i++) {
        ChaserGhost *ghost = &vis->ghost_chaser_ghosts[i];
        ghost->grid_x = ghost_positions[i][0];
        ghost->grid_y = ghost_positions[i][1];
        ghost->x = ghost->grid_x;
        ghost->y = ghost->grid_y;
        ghost->direction = (ChaserDirection)(rand() % 4);
        ghost->color_index = i % GHOST_CHASER_GHOST_COLORS;
        ghost->hue = 0.0;
        ghost->target_hue = 0.0;
        ghost->size_multiplier = 1.0;
        ghost->speed = 2.0;
        ghost->scared_timer = 0.0;
        ghost->scared = FALSE;
        ghost->visible = TRUE;
        ghost->blink_timer = 0.0;
        ghost->frequency_band = (i * VIS_FREQUENCY_BARS) / vis->ghost_chaser_ghost_count;
        ghost->audio_intensity = 0.0;
    }
    
    vis->ghost_chaser_beat_timer = 0.0;
    vis->ghost_chaser_power_pellet_timer = 0.0;
    vis->ghost_chaser_power_mode = FALSE;
    vis->ghost_chaser_move_timer = 0.0;
    
    ghost_chaser_init_maze(vis);
    ghost_chaser_calculate_layout(vis);
}

void ghost_chaser_calculate_layout(Visualizer *vis) {
    double padding = 20.0;
    double available_width = vis->width - (2 * padding);
    double available_height = vis->height - (2 * padding);
    
    double cell_width = available_width / GHOST_CHASER_MAZE_WIDTH;
    double cell_height = available_height / GHOST_CHASER_MAZE_HEIGHT;
    
    vis->ghost_chaser_cell_size = fmin(cell_width, cell_height);
    
    double total_maze_width = vis->ghost_chaser_cell_size * GHOST_CHASER_MAZE_WIDTH;
    double total_maze_height = vis->ghost_chaser_cell_size * GHOST_CHASER_MAZE_HEIGHT;
    
    vis->ghost_chaser_offset_x = (vis->width - total_maze_width) / 2.0;
    vis->ghost_chaser_offset_y = (vis->height - total_maze_height) / 2.0;
}

void ghost_chaser_init_maze(Visualizer *vis) {
    memcpy(vis->ghost_chaser_maze, ghost_chaser_maze_template, sizeof(ghost_chaser_maze_template));
    
    vis->ghost_chaser_pellet_count = 0;
    for (int y = 0; y < GHOST_CHASER_MAZE_HEIGHT; y++) {
        for (int x = 0; x < GHOST_CHASER_MAZE_WIDTH; x++) {
            if (vis->ghost_chaser_maze[y][x] == CHASER_PELLET || vis->ghost_chaser_maze[y][x] == CHASER_POWER_PELLET) {
                ChaserPellet *pellet = &vis->ghost_chaser_pellets[vis->ghost_chaser_pellet_count];
                pellet->grid_x = x;
                pellet->grid_y = y;
                pellet->active = TRUE;
                pellet->pulse_phase = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
                pellet->size_multiplier = 1.0;
                pellet->is_power_pellet = (vis->ghost_chaser_maze[y][x] == CHASER_POWER_PELLET);
                pellet->hue = pellet->is_power_pellet ? 60.0 : 45.0;
                pellet->frequency_band = (vis->ghost_chaser_pellet_count * VIS_FREQUENCY_BARS) / MAX_GHOST_CHASER_PELLETS;
                vis->ghost_chaser_pellet_count++;
            }
        }
    }
}

gboolean ghost_chaser_can_move(Visualizer *vis, int grid_x, int grid_y) {
    if (grid_x < 0 || grid_x >= GHOST_CHASER_MAZE_WIDTH || 
        grid_y < 0 || grid_y >= GHOST_CHASER_MAZE_HEIGHT) {
        return FALSE;
    }
    return vis->ghost_chaser_maze[grid_y][grid_x] != CHASER_WALL;
}

void ghost_chaser_consume_pellet(Visualizer *vis, int grid_x, int grid_y) {
    for (int i = 0; i < vis->ghost_chaser_pellet_count; i++) {
        ChaserPellet *pellet = &vis->ghost_chaser_pellets[i];
        if (pellet->active && pellet->grid_x == grid_x && pellet->grid_y == grid_y) {
            pellet->active = FALSE;
            
            if (pellet->is_power_pellet) {
                vis->ghost_chaser_power_mode = TRUE;
                vis->ghost_chaser_power_pellet_timer = 5.0;
                
                for (int j = 0; j < vis->ghost_chaser_ghost_count; j++) {
                    vis->ghost_chaser_ghosts[j].scared = TRUE;
                    vis->ghost_chaser_ghosts[j].scared_timer = 5.0;
                }
            }
            break;
        }
    }
}

gboolean ghost_chaser_detect_beat(Visualizer *vis) {
    static double last_volume = 0.0;
    static double beat_cooldown = 0.0;
    
    beat_cooldown -= 0.033;
    if (beat_cooldown < 0) beat_cooldown = 0;
    
    gboolean beat = (vis->volume_level > 0.15 && 
                     vis->volume_level > last_volume * 1.2 && 
                     beat_cooldown <= 0);
    
    if (beat) {
        beat_cooldown = 0.2;
    }
    
    last_volume = vis->volume_level;
    return beat;
}

void ghost_chaser_update_pellets(Visualizer *vis, double dt) {
    for (int i = 0; i < vis->ghost_chaser_pellet_count; i++) {
        ChaserPellet *pellet = &vis->ghost_chaser_pellets[i];
        if (!pellet->active) continue;
        
        pellet->pulse_phase += dt * 4.0;
        if (pellet->pulse_phase > 2.0 * M_PI) pellet->pulse_phase -= 2.0 * M_PI;
        
        double audio_factor = vis->frequency_bands[pellet->frequency_band];
        pellet->size_multiplier = 1.0 + audio_factor * 0.5;
        
        if (pellet->is_power_pellet) {
            pellet->size_multiplier *= 1.0 + 0.3 * sin(pellet->pulse_phase);
        }
    }
}

void update_ghost_chaser_visualization(Visualizer *vis, double dt) {
    ghost_chaser_calculate_layout(vis);
    ghost_chaser_update_player(vis, dt);
    ghost_chaser_update_ghosts(vis, dt);
    ghost_chaser_update_pellets(vis, dt);
    vis->ghost_chaser_beat_timer += dt;
}

void draw_ghost_chaser_maze(Visualizer *vis, cairo_t *cr) {
    double wall_glow = vis->volume_level * 0.5;
    cairo_set_source_rgba(cr, 0.0 + wall_glow, 0.0 + wall_glow, 1.0, 0.8 + wall_glow * 0.2);
    
    for (int y = 0; y < GHOST_CHASER_MAZE_HEIGHT; y++) {
        for (int x = 0; x < GHOST_CHASER_MAZE_WIDTH; x++) {
            if (vis->ghost_chaser_maze[y][x] == CHASER_WALL) {
                double wall_x = vis->ghost_chaser_offset_x + x * vis->ghost_chaser_cell_size;
                double wall_y = vis->ghost_chaser_offset_y + y * vis->ghost_chaser_cell_size;
                
                cairo_rectangle(cr, wall_x, wall_y, vis->ghost_chaser_cell_size, vis->ghost_chaser_cell_size);
                cairo_fill(cr);
            }
        }
    }
}

void draw_ghost_chaser_pellets(Visualizer *vis, cairo_t *cr) {
    for (int i = 0; i < vis->ghost_chaser_pellet_count; i++) {
        ChaserPellet *pellet = &vis->ghost_chaser_pellets[i];
        if (!pellet->active) continue;
        
        double pellet_x = vis->ghost_chaser_offset_x + pellet->grid_x * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
        double pellet_y = vis->ghost_chaser_offset_y + pellet->grid_y * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
        
        double r = 1.0, g = 1.0, b = 0.0;
        
        double size = (pellet->is_power_pellet ? vis->ghost_chaser_cell_size * 0.25 : vis->ghost_chaser_cell_size * 0.08) 
                      * pellet->size_multiplier;
        
        cairo_set_source_rgba(cr, r, g, b, 0.3);
        cairo_arc(cr, pellet_x, pellet_y, size * 2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_arc(cr, pellet_x, pellet_y, size, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_ghost_chaser_player(Visualizer *vis, cairo_t *cr) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    
    double player_x = vis->ghost_chaser_offset_x + player->x * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
    double player_y = vis->ghost_chaser_offset_y + player->y * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
    
    double size = vis->ghost_chaser_cell_size * 0.35 * player->size_multiplier;
    
    // CLASSIC PAC-MAN YELLOW COLOR
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.95);
    
    // Calculate mouth angle based on direction and animation
    double mouth_start = 0.0;
    switch (player->direction) {
        case CHASER_RIGHT: mouth_start = 0.0; break;
        case CHASER_DOWN: mouth_start = M_PI / 2; break;
        case CHASER_LEFT: mouth_start = M_PI; break;
        case CHASER_UP: mouth_start = 3 * M_PI / 2; break;
    }
    
    // Animate mouth opening/closing - make it more pronounced
    double mouth_opening = 0.4 + 0.4 * sin(player->mouth_angle);
    double start_angle = mouth_start - mouth_opening;
    double end_angle = mouth_start + mouth_opening;
    
    // Draw the classic Pac-Man shape with mouth
    cairo_new_path(cr);
    cairo_arc(cr, player_x, player_y, size, end_angle, start_angle);
    cairo_line_to(cr, player_x, player_y);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Add beat pulse glow effect
    if (player->beat_pulse > 0) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, player->beat_pulse * 0.5);
        cairo_arc(cr, player_x, player_y, size * (1.2 + player->beat_pulse * 0.3), 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_ghost_chaser_ghosts(Visualizer *vis, cairo_t *cr) {
    for (int i = 0; i < vis->ghost_chaser_ghost_count; i++) {
        ChaserGhost *ghost = &vis->ghost_chaser_ghosts[i];
        if (!ghost->visible) continue;
        
        double ghost_x = vis->ghost_chaser_offset_x + ghost->x * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
        double ghost_y = vis->ghost_chaser_offset_y + ghost->y * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
        
        double size = vis->ghost_chaser_cell_size * 0.32 * ghost->size_multiplier;
        
        double r, g, b;
        if (ghost->scared) {
            r = 0.2; g = 0.2; b = 0.8;
        } else {
            r = vis->ghost_chaser_ghost_colors[ghost->color_index][0];
            g = vis->ghost_chaser_ghost_colors[ghost->color_index][1];
            b = vis->ghost_chaser_ghost_colors[ghost->color_index][2];
            
            double intensity = ghost->audio_intensity;
            r = fmin(1.0, r + intensity * 0.4);
            g = fmin(1.0, g + intensity * 0.4);
            b = fmin(1.0, b + intensity * 0.4);
        }
        
        // Ghost body - rounded top
        cairo_new_path(cr);
        cairo_arc(cr, ghost_x, ghost_y - size * 0.15, size * 0.85, 0, M_PI);
        
        // Wavy bottom
        double wave_points = 8;
        double wave_time = vis->ghost_chaser_beat_timer + i * 0.5;
        
        for (int w = 0; w <= wave_points; w++) {
            double wave_x = ghost_x - size * 0.85 + (2.0 * size * 0.85 * w / wave_points);
            double wave_amplitude = size * 0.15 * (1.0 + ghost->audio_intensity * 0.5);
            double wave_y = ghost_y + size * 0.7 + wave_amplitude * sin(w * M_PI * 0.8 + wave_time * 3.0);
            cairo_line_to(cr, wave_x, wave_y);
        }
        
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_fill(cr);
        
        // Ghost eyes
        double eye_size = size * 0.15;
        double eye_y = ghost_y - size * 0.25;
        
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_arc(cr, ghost_x - size * 0.25, eye_y, eye_size, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, ghost_x + size * 0.25, eye_y, eye_size, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.9);
        cairo_arc(cr, ghost_x - size * 0.25, eye_y, eye_size * 0.6, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, ghost_x + size * 0.25, eye_y, eye_size * 0.6, 0, 2 * M_PI);
        cairo_fill(cr);
        
        if (ghost->audio_intensity > 0.2) {
            cairo_set_source_rgba(cr, r, g, b, ghost->audio_intensity * 0.4);
            cairo_arc(cr, ghost_x, ghost_y, size * 1.4, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
}

void draw_ghost_chaser_visualization(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    double bg_pulse = 0.05 + vis->volume_level * 0.1;
    cairo_set_source_rgba(cr, bg_pulse, bg_pulse, bg_pulse * 2, 1.0);
    cairo_paint(cr);
    
    draw_ghost_chaser_maze(vis, cr);
    draw_ghost_chaser_pellets(vis, cr);
    draw_ghost_chaser_ghosts(vis, cr);
    draw_ghost_chaser_player(vis, cr);
    
    if (vis->ghost_chaser_power_mode) {
        double power_flash = 0.15 + 0.1 * sin(vis->ghost_chaser_power_pellet_timer * 8.0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, power_flash);
        cairo_paint(cr);
        
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 24);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.9);
        
        char timer_text[32];
        snprintf(timer_text, sizeof(timer_text), "POWER MODE: %.1f", vis->ghost_chaser_power_pellet_timer);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, timer_text, &extents);
        cairo_move_to(cr, (vis->width - extents.width) / 2, 35);
        cairo_show_text(cr, timer_text);
    }
    
    if (vis->ghost_chaser_player.beat_pulse > 0) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, vis->ghost_chaser_player.beat_pulse * 0.08);
        cairo_paint(cr);
    }
}

// Improved direction choosing for player (toward pellets)
ChaserDirection ghost_chaser_choose_player_direction(Visualizer *vis) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    
    // Find nearest pellet
    int target_x, target_y;
    ghost_chaser_find_nearest_pellet(vis, player->grid_x, player->grid_y, &target_x, &target_y);
    
    // Get all valid directions
    ChaserDirection possible_directions[4];
    int possible_count = 0;
    
    for (int dir = 0; dir < 4; dir++) {
        int next_x = player->grid_x;
        int next_y = player->grid_y;
        
        switch (dir) {
            case CHASER_UP: next_y--; break;
            case CHASER_DOWN: next_y++; break;
            case CHASER_LEFT: next_x--; break;
            case CHASER_RIGHT: next_x++; break;
        }
        
        if (ghost_chaser_can_move(vis, next_x, next_y)) {
            possible_directions[possible_count++] = (ChaserDirection)dir;
        }
    }
    
    if (possible_count == 0) {
        return player->direction; // Keep current direction if stuck
    }
    
    // Find direction that gets us closer to target pellet
    ChaserDirection best_direction = possible_directions[0];
    double best_distance = 1000.0;
    
    for (int i = 0; i < possible_count; i++) {
        int test_x = player->grid_x;
        int test_y = player->grid_y;
        
        switch (possible_directions[i]) {
            case CHASER_UP: test_y--; break;
            case CHASER_DOWN: test_y++; break;
            case CHASER_LEFT: test_x--; break;
            case CHASER_RIGHT: test_x++; break;
        }
        
        double dx = test_x - target_x;
        double dy = test_y - target_y;
        double distance = sqrt(dx * dx + dy * dy);
        
        if (distance < best_distance) {
            best_distance = distance;
            best_direction = possible_directions[i];
        }
    }
    
    return best_direction;
}

// Improved ghost AI with better wall avoidance
ChaserDirection ghost_chaser_choose_smart_direction_v2(Visualizer *vis, ChaserGhost *ghost, int ghost_index) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    
    // Get all valid directions (no walls)
    ChaserDirection possible_directions[4];
    int possible_count = 0;
    
    for (int dir = 0; dir < 4; dir++) {
        int next_x = ghost->grid_x;
        int next_y = ghost->grid_y;
        
        switch (dir) {
            case CHASER_UP: next_y--; break;
            case CHASER_DOWN: next_y++; break;
            case CHASER_LEFT: next_x--; break;
            case CHASER_RIGHT: next_x++; break;
        }
        
        if (ghost_chaser_can_move(vis, next_x, next_y)) {
            // Don't go backwards unless it's the only option
            if (dir != ghost_chaser_get_opposite_direction(ghost->direction) || possible_count == 0) {
                possible_directions[possible_count++] = (ChaserDirection)dir;
            }
        }
    }
    
    if (possible_count == 0) {
        // Forced to go backwards - this should prevent wall sticking
        return ghost_chaser_get_opposite_direction(ghost->direction);
    }
    
    // Remove backwards direction if we have other options
    if (possible_count > 1) {
        ChaserDirection backwards = ghost_chaser_get_opposite_direction(ghost->direction);
        for (int i = 0; i < possible_count; i++) {
            if (possible_directions[i] == backwards) {
                // Remove backwards direction
                for (int j = i; j < possible_count - 1; j++) {
                    possible_directions[j] = possible_directions[j + 1];
                }
                possible_count--;
                break;
            }
        }
    }
    
    double audio_intensity = ghost->audio_intensity;
    
    if (ghost->scared) {
        // Scared ghosts run away from player
        ChaserDirection best_direction = possible_directions[0];
        double best_distance = -1;
        
        for (int i = 0; i < possible_count; i++) {
            int test_x = ghost->grid_x;
            int test_y = ghost->grid_y;
            
            switch (possible_directions[i]) {
                case CHASER_UP: test_y--; break;
                case CHASER_DOWN: test_y++; break;
                case CHASER_LEFT: test_x--; break;
                case CHASER_RIGHT: test_x++; break;
            }
            
            double distance = sqrt(pow(test_x - player->grid_x, 2) + pow(test_y - player->grid_y, 2));
            if (distance > best_distance) {
                best_distance = distance;
                best_direction = possible_directions[i];
            }
        }
        
        // Add randomness based on audio
        if (audio_intensity > 0.5 && rand() % 100 < 25) {
            return possible_directions[rand() % possible_count];
        }
        
        return best_direction;
    } else {
        // Normal ghost behavior
        switch (ghost_index % 4) {
            case 0: // Red ghost - aggressive chase
                {
                    if (audio_intensity > 0.3) {
                        ChaserDirection toward_player = ghost_chaser_get_direction_to_target(
                            ghost->grid_x, ghost->grid_y, player->grid_x, player->grid_y);
                        
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] == toward_player) {
                                return toward_player;
                            }
                        }
                    }
                    break;
                }
                
            case 1: // Pink ghost - ambush behavior
                {
                    if (audio_intensity > 0.2) {
                        int target_x = player->grid_x;
                        int target_y = player->grid_y;
                        
                        // Target ahead of player
                        switch (player->direction) {
                            case CHASER_UP: target_y -= 4; break;
                            case CHASER_DOWN: target_y += 4; break;
                            case CHASER_LEFT: target_x -= 4; break;
                            case CHASER_RIGHT: target_x += 4; break;
                        }
                        
                        ChaserDirection toward_target = ghost_chaser_get_direction_to_target(
                            ghost->grid_x, ghost->grid_y, target_x, target_y);
                        
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] == toward_target) {
                                return toward_target;
                            }
                        }
                    }
                    break;
                }
                
            case 2: // Cyan ghost - patrol corners
                {
                    // Prefer turns at intersections
                    if (possible_count > 2) {
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] != ghost->direction) {
                                return possible_directions[i];
                            }
                        }
                    }
                    break;
                }
                
            case 3: // Orange ghost - distance dependent
                {
                    double distance_to_player = ghost_chaser_distance_to_player(ghost, player);
                    
                    if (distance_to_player > 8.0 && audio_intensity > 0.2) {
                        // Chase when far
                        ChaserDirection toward_player = ghost_chaser_get_direction_to_target(
                            ghost->grid_x, ghost->grid_y, player->grid_x, player->grid_y);
                        
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] == toward_player) {
                                return toward_player;
                            }
                        }
                    } else if (distance_to_player <= 8.0) {
                        // Run away when close
                        ChaserDirection away_from_player = ghost_chaser_get_direction_to_target(
                            player->grid_x, player->grid_y, ghost->grid_x, ghost->grid_y);
                        
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] == away_from_player) {
                                return away_from_player;
                            }
                        }
                    }
                    break;
                }
        }
        
        // Default: continue straight if possible, otherwise random
        for (int i = 0; i < possible_count; i++) {
            if (possible_directions[i] == ghost->direction) {
                return ghost->direction;
            }
        }
        
        return possible_directions[rand() % possible_count];
    }
}

// Updated player update function with smart AI
void ghost_chaser_update_player(Visualizer *vis, double dt) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    
    if (player->beat_pulse > 0) {
        player->beat_pulse -= dt * 3.0;
        if (player->beat_pulse < 0) player->beat_pulse = 0;
    }
    
    player->size_multiplier = 1.0 + vis->volume_level * 0.3 + player->beat_pulse * 0.5;
    double speed_multiplier = 1.0 + vis->volume_level * 0.5;
    
    double mouth_speed = 8.0 * speed_multiplier;
    player->mouth_angle += dt * mouth_speed;
    if (player->mouth_angle > 2.0 * M_PI) player->mouth_angle -= 2.0 * M_PI;
    
    if (player->moving) {
        double move_distance = player->speed * speed_multiplier * dt;
        
        double target_x = player->x;
        double target_y = player->y;
        
        switch (player->direction) {
            case CHASER_UP: target_y -= move_distance; break;
            case CHASER_DOWN: target_y += move_distance; break;
            case CHASER_LEFT: target_x -= move_distance; break;
            case CHASER_RIGHT: target_x += move_distance; break;
        }
        
        int target_grid_x = (int)round(target_x);
        int target_grid_y = (int)round(target_y);
        
        if (ghost_chaser_can_move(vis, target_grid_x, target_grid_y)) {
            player->x = target_x;
            player->y = target_y;
            
            if (target_grid_x != player->grid_x || target_grid_y != player->grid_y) {
                player->grid_x = target_grid_x;
                player->grid_y = target_grid_y;
                ghost_chaser_consume_pellet(vis, player->grid_x, player->grid_y);
                
                // At intersection or when reaching a cell, choose smart direction toward pellets
                player->next_direction = ghost_chaser_choose_player_direction(vis);
            }
        } else {
            // Hit a wall, choose new direction immediately
            player->next_direction = ghost_chaser_choose_player_direction(vis);
        }
        
        // Try to change direction if queued
        if (player->next_direction != player->direction) {
            int test_x = player->grid_x;
            int test_y = player->grid_y;
            
            switch (player->next_direction) {
                case CHASER_UP: test_y--; break;
                case CHASER_DOWN: test_y++; break;
                case CHASER_LEFT: test_x--; break;
                case CHASER_RIGHT: test_x++; break;
            }
            
            if (ghost_chaser_can_move(vis, test_x, test_y)) {
                player->direction = player->next_direction;
            }
        }
    }
    
    // Audio-reactive direction changes (but still smart)
    if (ghost_chaser_detect_beat(vis)) {
        player->beat_pulse = 1.0;
        
        if (vis->volume_level > 0.4) {
            // On strong beats, occasionally pick a random direction for variety
            if (rand() % 100 < 20) { // 20% chance for random movement
                ChaserDirection new_dir = (ChaserDirection)(rand() % 4);
                player->next_direction = new_dir;
            } else {
                // Otherwise, be smart about pellet chasing
                player->next_direction = ghost_chaser_choose_player_direction(vis);
            }
        }
    }
    
    // Constantly try to move toward pellets (every few frames)
    static int smart_counter = 0;
    smart_counter++;
    if (smart_counter % 10 == 0) { // Every 10 frames (~3 times per second)
        player->next_direction = ghost_chaser_choose_player_direction(vis);
    }
}

// Updated ghost update function with better wall avoidance
void ghost_chaser_update_ghosts(Visualizer *vis, double dt) {
    for (int i = 0; i < vis->ghost_chaser_ghost_count; i++) {
        ChaserGhost *ghost = &vis->ghost_chaser_ghosts[i];
        
        ghost->audio_intensity = vis->frequency_bands[ghost->frequency_band];
        
        if (ghost->scared) {
            ghost->scared_timer -= dt;
            if (ghost->scared_timer <= 0) {
                ghost->scared = FALSE;
            }
            if (ghost->scared_timer < 2.0) {
                ghost->blink_timer += dt;
                ghost->visible = (fmod(ghost->blink_timer, 0.3) < 0.15);
            } else {
                ghost->visible = TRUE;
            }
        } else {
            ghost->visible = TRUE;
        }
        
        ghost->size_multiplier = 1.0 + ghost->audio_intensity * 0.4;
        double speed_multiplier = 1.0 + ghost->audio_intensity * 0.3;
        
        if (ghost->scared) {
            ghost->target_hue = 240.0;
        } else {
            ghost->target_hue = ghost->audio_intensity * 360.0;
        }
        
        ghost->hue += (ghost->target_hue - ghost->hue) * dt * 5.0;
        
        double move_distance = ghost->speed * speed_multiplier * dt;
        double target_x = ghost->x;
        double target_y = ghost->y;
        
        switch (ghost->direction) {
            case CHASER_UP: target_y -= move_distance; break;
            case CHASER_DOWN: target_y += move_distance; break;
            case CHASER_LEFT: target_x -= move_distance; break;
            case CHASER_RIGHT: target_x += move_distance; break;
        }
        
        int target_grid_x = (int)round(target_x);
        int target_grid_y = (int)round(target_y);
        
        if (ghost_chaser_can_move(vis, target_grid_x, target_grid_y)) {
            ghost->x = target_x;
            ghost->y = target_y;
            
            if (target_grid_x != ghost->grid_x || target_grid_y != ghost->grid_y) {
                ghost->grid_x = target_grid_x;
                ghost->grid_y = target_grid_y;
                
                // Use improved AI when reaching a new cell
                ghost->direction = ghost_chaser_choose_smart_direction_v2(vis, ghost, i);
            }
        } else {
            // Hit a wall - IMMEDIATELY choose a new valid direction
            ghost->direction = ghost_chaser_choose_smart_direction_v2(vis, ghost, i);
            
            // Also snap back to grid center to prevent getting stuck in walls
            ghost->x = ghost->grid_x;
            ghost->y = ghost->grid_y;
        }
    }
    
    if (vis->ghost_chaser_power_mode) {
        vis->ghost_chaser_power_pellet_timer -= dt;
        if (vis->ghost_chaser_power_pellet_timer <= 0) {
            vis->ghost_chaser_power_mode = FALSE;
        }
    }
}

double ghost_chaser_distance_to_player(ChaserGhost *ghost, ChaserPlayer *player) {
    double dx = ghost->x - player->x;
    double dy = ghost->y - player->y;
    return sqrt(dx * dx + dy * dy);
}

ChaserDirection ghost_chaser_get_opposite_direction(ChaserDirection dir) {
    switch (dir) {
        case CHASER_UP: return CHASER_DOWN;
        case CHASER_DOWN: return CHASER_UP;
        case CHASER_LEFT: return CHASER_RIGHT;
        case CHASER_RIGHT: return CHASER_LEFT;
    }
    return dir;
}

ChaserDirection ghost_chaser_get_direction_to_target(int from_x, int from_y, int to_x, int to_y) {
    int dx = to_x - from_x;
    int dy = to_y - from_y;
    
    // Choose direction based on largest distance component
    if (abs(dx) > abs(dy)) {
        return (dx > 0) ? CHASER_RIGHT : CHASER_LEFT;
    } else {
        return (dy > 0) ? CHASER_DOWN : CHASER_UP;
    }
}

void ghost_chaser_find_nearest_pellet(Visualizer *vis, int from_x, int from_y, int *target_x, int *target_y) {
    double nearest_distance = 1000.0;
    *target_x = from_x;
    *target_y = from_y;
    
    for (int i = 0; i < vis->ghost_chaser_pellet_count; i++) {
        ChaserPellet *pellet = &vis->ghost_chaser_pellets[i];
        if (!pellet->active) continue;
        
        double dx = pellet->grid_x - from_x;
        double dy = pellet->grid_y - from_y;
        double distance = sqrt(dx * dx + dy * dy);
        
        if (distance < nearest_distance) {
            nearest_distance = distance;
            *target_x = pellet->grid_x;
            *target_y = pellet->grid_y;
        }
    }
}

