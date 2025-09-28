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
    vis->ghost_chaser_player.speed = 2.0; // cells per second
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
        ghost->speed = 1.5; // Slightly slower than chaser
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
    // Calculate cell size to fit the screen with some padding
    double padding = 20.0;
    double available_width = vis->width - (2 * padding);
    double available_height = vis->height - (2 * padding);
    
    double cell_width = available_width / GHOST_CHASER_MAZE_WIDTH;
    double cell_height = available_height / GHOST_CHASER_MAZE_HEIGHT;
    
    // Use the smaller dimension to maintain aspect ratio
    vis->ghost_chaser_cell_size = fmin(cell_width, cell_height);
    
    // Center the maze
    double total_maze_width = vis->ghost_chaser_cell_size * GHOST_CHASER_MAZE_WIDTH;
    double total_maze_height = vis->ghost_chaser_cell_size * GHOST_CHASER_MAZE_HEIGHT;
    
    vis->ghost_chaser_offset_x = (vis->width - total_maze_width) / 2.0;
    vis->ghost_chaser_offset_y = (vis->height - total_maze_height) / 2.0;
}

void ghost_chaser_init_maze(Visualizer *vis) {
    // Copy template to working maze
    memcpy(vis->ghost_chaser_maze, ghost_chaser_maze_template, sizeof(ghost_chaser_maze_template));
    
    // Initialize pellets
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
                pellet->hue = pellet->is_power_pellet ? 60.0 : 45.0; // Yellow for both
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
            
            // If it was a power pellet, enter power mode
            if (pellet->is_power_pellet) {
                vis->ghost_chaser_power_mode = TRUE;
                vis->ghost_chaser_power_pellet_timer = 5.0; // 5 seconds of power mode
                
                // Make all ghosts scared
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
    
    gboolean beat = (vis->volume_level > 0.2 && 
                     vis->volume_level > last_volume * 1.3 && 
                     beat_cooldown <= 0);
    
    if (beat) {
        beat_cooldown = 0.3; // 300ms cooldown
    }
    
    last_volume = vis->volume_level;
    return beat;
}

void ghost_chaser_update_player(Visualizer *vis, double dt) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    
    // Update beat pulse
    if (player->beat_pulse > 0) {
        player->beat_pulse -= dt * 3.0;
        if (player->beat_pulse < 0) player->beat_pulse = 0;
    }
    
    // Audio-reactive size
    player->size_multiplier = 1.0 + vis->volume_level * 0.3 + player->beat_pulse * 0.5;
    
    // Update mouth animation
    player->mouth_angle += dt * 8.0;
    if (player->mouth_angle > 2.0 * M_PI) player->mouth_angle -= 2.0 * M_PI;
    
    // Movement logic - Fixed collision detection
    if (player->moving) {
        // Calculate how close we are to grid alignment
        double x_offset = player->x - player->grid_x;
        double y_offset = player->y - player->grid_y;
        double alignment_threshold = 0.1; // How close to center before allowing direction change
        
        // Check if we can change direction (only when close to grid center)
        if (fabs(x_offset) < alignment_threshold && fabs(y_offset) < alignment_threshold) {
            int next_x = player->grid_x;
            int next_y = player->grid_y;
            
            switch (player->next_direction) {
                case CHASER_UP: next_y--; break;
                case CHASER_DOWN: next_y++; break;
                case CHASER_LEFT: next_x--; break;
                case CHASER_RIGHT: next_x++; break;
            }
            
            if (ghost_chaser_can_move(vis, next_x, next_y)) {
                player->direction = player->next_direction;
                // Snap to grid center to prevent getting stuck
                player->x = player->grid_x;
                player->y = player->grid_y;
            }
        }
        
        // Calculate movement for current frame
        double move_distance = player->speed * dt;
        double new_x = player->x;
        double new_y = player->y;
        
        switch (player->direction) {
            case CHASER_UP: new_y -= move_distance; break;
            case CHASER_DOWN: new_y += move_distance; break;
            case CHASER_LEFT: new_x -= move_distance; break;
            case CHASER_RIGHT: new_x += move_distance; break;
        }
        
        // Check if the new position would put us in a wall
        int target_grid_x = (int)round(new_x);
        int target_grid_y = (int)round(new_y);
        
        // More robust collision detection
        gboolean can_move = TRUE;
        
        // Check all corners of the player's hitbox
        double hitbox_size = 0.3; // Slightly smaller than cell size
        int corners[4][2] = {
            {(int)(new_x - hitbox_size), (int)(new_y - hitbox_size)}, // Top-left
            {(int)(new_x + hitbox_size), (int)(new_y - hitbox_size)}, // Top-right
            {(int)(new_x - hitbox_size), (int)(new_y + hitbox_size)}, // Bottom-left
            {(int)(new_x + hitbox_size), (int)(new_y + hitbox_size)}  // Bottom-right
        };
        
        for (int i = 0; i < 4; i++) {
            if (!ghost_chaser_can_move(vis, corners[i][0], corners[i][1])) {
                can_move = FALSE;
                break;
            }
        }
        
        if (can_move) {
            player->x = new_x;
            player->y = new_y;
            
            // Update grid position based on center point
            int new_grid_x = (int)round(player->x);
            int new_grid_y = (int)round(player->y);
            
            // Only update grid position if we've moved to a new cell
            if (new_grid_x != player->grid_x || new_grid_y != player->grid_y) {
                player->grid_x = new_grid_x;
                player->grid_y = new_grid_y;
                
                // Check for pellet consumption
                ghost_chaser_consume_pellet(vis, player->grid_x, player->grid_y);
            }
        } else {
            // If we can't move, stop and align to grid
            player->x = player->grid_x;
            player->y = player->grid_y;
        }
    }
    
    // Auto-change direction on beats (for demo purposes)
    if (ghost_chaser_detect_beat(vis)) {
        player->beat_pulse = 1.0;
        // Randomly change direction on strong beats
        if (vis->volume_level > 0.4) {
            player->next_direction = (ChaserDirection)(rand() % 4);
        }
    }
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

double ghost_chaser_distance_to_player(ChaserGhost *ghost, ChaserPlayer *player) {
    double dx = ghost->x - player->x;
    double dy = ghost->y - player->y;
    return sqrt(dx * dx + dy * dy);
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

ChaserDirection ghost_chaser_choose_smart_direction(Visualizer *vis, ChaserGhost *ghost, int ghost_index) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    ChaserDirection possible_directions[4];
    int possible_count = 0;
    
    // Get all valid directions (not walls, not backwards unless necessary)
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
            // Avoid going backwards unless it's the only option
            if (dir != ghost_chaser_get_opposite_direction(ghost->direction) || possible_count == 0) {
                possible_directions[possible_count++] = (ChaserDirection)dir;
            }
        }
    }
    
    if (possible_count == 0) {
        // Forced to go backwards
        return ghost_chaser_get_opposite_direction(ghost->direction);
    }
    
    // Different behavior based on ghost state and index
    if (ghost->scared) {
        // Scared ghosts run away from player
        ChaserDirection away_from_player = ghost_chaser_get_direction_to_target(
            player->grid_x, player->grid_y, ghost->grid_x, ghost->grid_y);
        
        // Find the direction that takes us furthest from player
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
        
        return best_direction;
    } else {
        // Normal ghosts chase player with different strategies per ghost
        switch (ghost_index % 4) {
            case 0: // Red ghost - direct chase
                {
                    ChaserDirection toward_player = ghost_chaser_get_direction_to_target(
                        ghost->grid_x, ghost->grid_y, player->grid_x, player->grid_y);
                    
                    // If we can go toward player, do it
                    for (int i = 0; i < possible_count; i++) {
                        if (possible_directions[i] == toward_player) {
                            return toward_player;
                        }
                    }
                    // Otherwise pick a random valid direction
                    return possible_directions[rand() % possible_count];
                }
                
            case 1: // Pink ghost - ambush (target ahead of player)
                {
                    int target_x = player->grid_x;
                    int target_y = player->grid_y;
                    
                    // Target 4 cells ahead of player
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
                    return possible_directions[rand() % possible_count];
                }
                
            case 2: // Cyan ghost - patrol behavior
                {
                    // Patrol in a pattern, change direction at intersections
                    if (possible_count > 2) { // At an intersection
                        // Prefer to turn rather than go straight
                        ChaserDirection straight = ghost->direction;
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] != straight) {
                                return possible_directions[i];
                            }
                        }
                    }
                    // Continue straight if possible
                    for (int i = 0; i < possible_count; i++) {
                        if (possible_directions[i] == ghost->direction) {
                            return ghost->direction;
                        }
                    }
                    return possible_directions[rand() % possible_count];
                }
                
            case 3: // Orange ghost - random with distance consideration
                {
                    double distance_to_player = ghost_chaser_distance_to_player(ghost, player);
                    
                    if (distance_to_player > 8.0) {
                        // Far from player - chase
                        ChaserDirection toward_player = ghost_chaser_get_direction_to_target(
                            ghost->grid_x, ghost->grid_y, player->grid_x, player->grid_y);
                        
                        for (int i = 0; i < possible_count; i++) {
                            if (possible_directions[i] == toward_player) {
                                return toward_player;
                            }
                        }
                    }
                    // Close to player or can't chase - random movement
                    return possible_directions[rand() % possible_count];
                }
        }
    }
    
    // Fallback
    return possible_directions[rand() % possible_count];
}

void ghost_chaser_update_ghosts(Visualizer *vis, double dt) {
    for (int i = 0; i < vis->ghost_chaser_ghost_count; i++) {
        ChaserGhost *ghost = &vis->ghost_chaser_ghosts[i];
        
        // Update audio intensity for this ghost
        ghost->audio_intensity = vis->frequency_bands[ghost->frequency_band];
        
        // Update scared timer
        if (ghost->scared) {
            ghost->scared_timer -= dt;
            if (ghost->scared_timer <= 0) {
                ghost->scared = FALSE;
            }
            // Blinking when scared time is running out
            if (ghost->scared_timer < 2.0) {
                ghost->blink_timer += dt;
                ghost->visible = (fmod(ghost->blink_timer, 0.5) < 0.25);
            } else {
                ghost->visible = TRUE;
            }
        } else {
            ghost->visible = TRUE;
        }
        
        // Audio-reactive size and color
        ghost->size_multiplier = 1.0 + ghost->audio_intensity * 0.4;
        
        // Change color based on audio intensity
        if (ghost->scared) {
            ghost->target_hue = 240.0; // Blue when scared
        } else {
            ghost->target_hue = ghost->audio_intensity * 360.0;
        }
        
        // Smooth color transition
        ghost->hue += (ghost->target_hue - ghost->hue) * dt * 5.0;
        
        // Smooth movement system
        double move_distance = ghost->speed * dt;
        double new_x = ghost->x;
        double new_y = ghost->y;
        
        switch (ghost->direction) {
            case CHASER_UP: new_y -= move_distance; break;
            case CHASER_DOWN: new_y += move_distance; break;
            case CHASER_LEFT: new_x -= move_distance; break;
            case CHASER_RIGHT: new_x += move_distance; break;
        }
        
        // Check if we've reached the center of the next cell
        int target_grid_x = (int)round(new_x);
        int target_grid_y = (int)round(new_y);
        
        // Use same collision detection as player
        double hitbox_size = 0.3;
        gboolean can_move = TRUE;
        
        int corners[4][2] = {
            {(int)(new_x - hitbox_size), (int)(new_y - hitbox_size)},
            {(int)(new_x + hitbox_size), (int)(new_y - hitbox_size)},
            {(int)(new_x - hitbox_size), (int)(new_y + hitbox_size)},
            {(int)(new_x + hitbox_size), (int)(new_y + hitbox_size)}
        };
        
        for (int j = 0; j < 4; j++) {
            if (!ghost_chaser_can_move(vis, corners[j][0], corners[j][1])) {
                can_move = FALSE;
                break;
            }
        }
        
        if (can_move) {
            ghost->x = new_x;
            ghost->y = new_y;
            
            // Check if we've moved to a new grid cell
            if (target_grid_x != ghost->grid_x || target_grid_y != ghost->grid_y) {
                ghost->grid_x = target_grid_x;
                ghost->grid_y = target_grid_y;
            }
        } else {
            // Hit a wall, need to choose new direction
            ghost->x = ghost->grid_x;
            ghost->y = ghost->grid_y;
            ghost->direction = ghost_chaser_choose_smart_direction(vis, ghost, i);
        }
        
        // Decision making at intersections (when close to grid center)
        double x_offset = ghost->x - ghost->grid_x;
        double y_offset = ghost->y - ghost->grid_y;
        double alignment_threshold = 0.1;
        
        if (fabs(x_offset) < alignment_threshold && fabs(y_offset) < alignment_threshold) {
            // We're at an intersection, decide if we should change direction
            // Check how many directions are available
            int direction_count = 0;
            for (int dir = 0; dir < 4; dir++) {
                int test_x = ghost->grid_x;
                int test_y = ghost->grid_y;
                
                switch (dir) {
                    case CHASER_UP: test_y--; break;
                    case CHASER_DOWN: test_y++; break;
                    case CHASER_LEFT: test_x--; break;
                    case CHASER_RIGHT: test_x++; break;
                }
                
                if (ghost_chaser_can_move(vis, test_x, test_y)) {
                    direction_count++;
                }
            }
            
            // If at an intersection (more than 2 directions), make a smart choice
            if (direction_count > 2) {
                ChaserDirection new_direction = ghost_chaser_choose_smart_direction(vis, ghost, i);
                if (new_direction != ghost->direction) {
                    ghost->direction = new_direction;
                    // Snap to grid center for clean turns
                    ghost->x = ghost->grid_x;
                    ghost->y = ghost->grid_y;
                }
            }
        }
    }
    
    // Update power mode
    if (vis->ghost_chaser_power_mode) {
        vis->ghost_chaser_power_pellet_timer -= dt;
        if (vis->ghost_chaser_power_pellet_timer <= 0) {
            vis->ghost_chaser_power_mode = FALSE;
        }
    }
}

void ghost_chaser_update_pellets(Visualizer *vis, double dt) {
    for (int i = 0; i < vis->ghost_chaser_pellet_count; i++) {
        ChaserPellet *pellet = &vis->ghost_chaser_pellets[i];
        if (!pellet->active) continue;
        
        // Update pulse animation
        pellet->pulse_phase += dt * 4.0;
        if (pellet->pulse_phase > 2.0 * M_PI) pellet->pulse_phase -= 2.0 * M_PI;
        
        // Audio-reactive size
        double audio_factor = vis->frequency_bands[pellet->frequency_band];
        pellet->size_multiplier = 1.0 + audio_factor * 0.5;
        
        // Power pellets pulse more dramatically
        if (pellet->is_power_pellet) {
            pellet->size_multiplier *= 1.0 + 0.3 * sin(pellet->pulse_phase);
        }
    }
}

void update_ghost_chaser_visualization(Visualizer *vis, double dt) {
    // Recalculate layout in case screen size changed
    ghost_chaser_calculate_layout(vis);
    
    ghost_chaser_update_player(vis, dt);
    ghost_chaser_update_ghosts(vis, dt);
    ghost_chaser_update_pellets(vis, dt);
    
    vis->ghost_chaser_beat_timer += dt;
}

void draw_ghost_chaser_maze(Visualizer *vis, cairo_t *cr) {
    // Draw maze walls
    cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.8); // Blue walls
    
    for (int y = 0; y < GHOST_CHASER_MAZE_HEIGHT; y++) {
        for (int x = 0; x < GHOST_CHASER_MAZE_WIDTH; x++) {
            if (vis->ghost_chaser_maze[y][x] == CHASER_WALL) {
                double wall_x = vis->ghost_chaser_offset_x + x * vis->ghost_chaser_cell_size;
                double wall_y = vis->ghost_chaser_offset_y + y * vis->ghost_chaser_cell_size;
                
                cairo_rectangle(cr, wall_x, wall_y, vis->ghost_chaser_cell_size, vis->ghost_chaser_cell_size);
                cairo_fill(cr);
                
                // Add some glow based on overall audio level
                if (vis->volume_level > 0.1) {
                    cairo_set_source_rgba(cr, 0.2, 0.2, 1.0, vis->volume_level * 0.3);
                    cairo_rectangle(cr, wall_x - 2, wall_y - 2, 
                                   vis->ghost_chaser_cell_size + 4, vis->ghost_chaser_cell_size + 4);
                    cairo_fill(cr);
                    cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.8);
                }
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
        
        double r, g, b;
        hsv_to_rgb(pellet->hue, 0.8, 0.9, &r, &g, &b);
        
        double size = (pellet->is_power_pellet ? vis->ghost_chaser_cell_size * 0.3 : vis->ghost_chaser_cell_size * 0.1) 
                      * pellet->size_multiplier;
        
        // Glow effect
        cairo_set_source_rgba(cr, r, g, b, 0.3);
        cairo_arc(cr, pellet_x, pellet_y, size * 2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Main pellet
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_arc(cr, pellet_x, pellet_y, size, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_ghost_chaser_player(Visualizer *vis, cairo_t *cr) {
    ChaserPlayer *player = &vis->ghost_chaser_player;
    
    double player_x = vis->ghost_chaser_offset_x + player->x * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
    double player_y = vis->ghost_chaser_offset_y + player->y * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
    
    double size = vis->ghost_chaser_cell_size * 0.4 * player->size_multiplier;
    
    // Orange chaser (different from yellow to avoid Pac-Man similarity)
    cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, 0.9);
    
    // Calculate mouth angle based on direction and animation
    double mouth_start = 0.0;
    switch (player->direction) {
        case CHASER_RIGHT: mouth_start = 0.0; break;
        case CHASER_DOWN: mouth_start = M_PI / 2; break;
        case CHASER_LEFT: mouth_start = M_PI; break;
        case CHASER_UP: mouth_start = 3 * M_PI / 2; break;
    }
    
    // Animate mouth opening/closing
    double mouth_opening = 0.3 + 0.3 * sin(player->mouth_angle);
    double start_angle = mouth_start - mouth_opening;
    double end_angle = mouth_start + mouth_opening;
    
    cairo_arc(cr, player_x, player_y, size, start_angle, end_angle);
    cairo_line_to(cr, player_x, player_y);
    cairo_fill(cr);
    
    // Add glow on beats
    if (player->beat_pulse > 0) {
        cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, player->beat_pulse * 0.5);
        cairo_arc(cr, player_x, player_y, size * 1.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_ghost_chaser_ghosts(Visualizer *vis, cairo_t *cr) {
    for (int i = 0; i < vis->ghost_chaser_ghost_count; i++) {
        ChaserGhost *ghost = &vis->ghost_chaser_ghosts[i];
        if (!ghost->visible) continue;
        
        double ghost_x = vis->ghost_chaser_offset_x + ghost->x * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
        double ghost_y = vis->ghost_chaser_offset_y + ghost->y * vis->ghost_chaser_cell_size + vis->ghost_chaser_cell_size / 2;
        
        double size = vis->ghost_chaser_cell_size * 0.35 * ghost->size_multiplier;
        
        double r, g, b;
        if (ghost->scared) {
            hsv_to_rgb(ghost->hue, 0.8, 0.7, &r, &g, &b);
        } else {
            r = vis->ghost_chaser_ghost_colors[ghost->color_index][0];
            g = vis->ghost_chaser_ghost_colors[ghost->color_index][1];
            b = vis->ghost_chaser_ghost_colors[ghost->color_index][2];
            
            // Modulate with audio
            double intensity = ghost->audio_intensity;
            r = fmin(1.0, r + intensity * 0.3);
            g = fmin(1.0, g + intensity * 0.3);
            b = fmin(1.0, b + intensity * 0.3);
        }
        
        // Ghost body (rounded top, wavy bottom)
        cairo_arc(cr, ghost_x, ghost_y - size * 0.2, size, 0, M_PI);
        
        // Wavy bottom
        double wave_points = 6;
        for (int w = 0; w <= wave_points; w++) {
            double wave_x = ghost_x - size + (2.0 * size * w / wave_points);
            double wave_y = ghost_y + size * 0.8 + size * 0.2 * sin(w * M_PI + ghost->audio_intensity * 5.0);
            cairo_line_to(cr, wave_x, wave_y);
        }
        
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_fill(cr);
        
        // Ghost eyes
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.9);
        cairo_arc(cr, ghost_x - size * 0.3, ghost_y - size * 0.2, size * 0.15, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, ghost_x + size * 0.3, ghost_y - size * 0.2, size * 0.15, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Eye pupils
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.9);
        cairo_arc(cr, ghost_x - size * 0.3, ghost_y - size * 0.2, size * 0.08, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, ghost_x + size * 0.3, ghost_y - size * 0.2, size * 0.08, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Audio glow effect
        if (ghost->audio_intensity > 0.2) {
            cairo_set_source_rgba(cr, r, g, b, ghost->audio_intensity * 0.3);
            cairo_arc(cr, ghost_x, ghost_y, size * 1.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
}

void draw_ghost_chaser_visualization(Visualizer *vis, cairo_t *cr) {
    if (vis->width <= 0 || vis->height <= 0) return;
    
    // Draw background
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_paint(cr);
    
    // Draw maze elements in order
    draw_ghost_chaser_maze(vis, cr);
    draw_ghost_chaser_pellets(vis, cr);
    draw_ghost_chaser_ghosts(vis, cr);
    draw_ghost_chaser_player(vis, cr);
    
    // Draw power mode indicator
    if (vis->ghost_chaser_power_mode) {
        cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, 0.1 + 0.1 * sin(vis->ghost_chaser_power_pellet_timer * 5.0));
        cairo_paint(cr);
        
        // Power mode timer display
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 24);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);
        
        char timer_text[32];
        snprintf(timer_text, sizeof(timer_text), "POWER MODE: %.1f", vis->ghost_chaser_power_pellet_timer);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, timer_text, &extents);
        cairo_move_to(cr, (vis->width - extents.width) / 2, 30);
        cairo_show_text(cr, timer_text);
    }
    
    // Draw beat pulse overlay
    if (vis->ghost_chaser_player.beat_pulse > 0) {
        cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, vis->ghost_chaser_player.beat_pulse * 0.1);
        cairo_paint(cr);
    }
}

