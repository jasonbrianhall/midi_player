#include "beatcheckers.h"
#include "visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

// ============================================================================
// CORE CHECKERS ENGINE
// ============================================================================

void checkers_init_board(CheckersGameState *game) {
    // Clear board
    for (int r = 0; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            game->board[r][c].color = CHECKERS_NONE;
            game->board[r][c].is_king = false;
        }
    }
    
    // Set up pieces - only on dark squares
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            if ((r + c) % 2 == 1) {  // Dark squares
                game->board[r][c].color = CHECKERS_BLACK;
            }
        }
    }
    
    for (int r = 5; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            if ((r + c) % 2 == 1) {  // Dark squares
                game->board[r][c].color = CHECKERS_RED;
            }
        }
    }
    
    game->turn = CHECKERS_RED;
    game->red_pieces = 12;
    game->black_pieces = 12;
}

// Find all jumps from a position (recursive for multi-jumps)
static int find_jumps_from(CheckersGameState *game, int r, int c, 
                          CheckersMove *moves, int move_count,
                          CheckersMove current_move, bool jumped[8][8]) {
    CheckersPiece piece = game->board[r][c];
    
    // Possible jump directions
    int dirs[4][2];
    int num_dirs = 0;
    
    if (piece.is_king) {
        // Kings can jump in all 4 diagonal directions
        dirs[0][0] = -1; dirs[0][1] = -1;
        dirs[1][0] = -1; dirs[1][1] = 1;
        dirs[2][0] = 1;  dirs[2][1] = -1;
        dirs[3][0] = 1;  dirs[3][1] = 1;
        num_dirs = 4;
    } else if (piece.color == CHECKERS_RED) {
        // Red moves up (decreasing row)
        dirs[0][0] = -1; dirs[0][1] = -1;
        dirs[1][0] = -1; dirs[1][1] = 1;
        num_dirs = 2;
    } else {
        // Black moves down (increasing row)
        dirs[0][0] = 1; dirs[0][1] = -1;
        dirs[1][0] = 1; dirs[1][1] = 1;
        num_dirs = 2;
    }
    
    bool found_jump = false;
    
    for (int d = 0; d < num_dirs; d++) {
        int mid_r = r + dirs[d][0];
        int mid_c = c + dirs[d][1];
        int land_r = r + dirs[d][0] * 2;
        int land_c = c + dirs[d][1] * 2;
        
        // Check bounds
        if (land_r < 0 || land_r >= CHECKERS_BOARD_SIZE || 
            land_c < 0 || land_c >= CHECKERS_BOARD_SIZE) continue;
        
        // Check if there's an opponent piece to jump
        CheckersPiece mid = game->board[mid_r][mid_c];
        if (mid.color == CHECKERS_NONE || mid.color == piece.color) continue;
        if (jumped[mid_r][mid_c]) continue;  // Already jumped this piece
        
        // Check if landing square is empty
        if (game->board[land_r][land_c].color != CHECKERS_NONE) continue;
        
        // Valid jump found!
        found_jump = true;
        
        // Mark this piece as jumped
        jumped[mid_r][mid_c] = true;
        
        // Add to current move
        CheckersMove extended = current_move;
        extended.to_row = land_r;
        extended.to_col = land_c;
        extended.jumped_rows[extended.jump_count] = mid_r;
        extended.jumped_cols[extended.jump_count] = mid_c;
        extended.jump_count++;
        
        // Check for king promotion
        if (!piece.is_king) {
            if ((piece.color == CHECKERS_RED && land_r == 0) ||
                (piece.color == CHECKERS_BLACK && land_r == 7)) {
                extended.becomes_king = true;
            }
        }
        
        // Temporarily make the jump to check for more jumps
        CheckersGameState temp = *game;
        temp.board[land_r][land_c] = temp.board[r][c];
        temp.board[r][c].color = CHECKERS_NONE;
        temp.board[mid_r][mid_c].color = CHECKERS_NONE;
        if (extended.becomes_king) {
            temp.board[land_r][land_c].is_king = true;
        }
        
        // Recursively look for more jumps
        int new_count = find_jumps_from(&temp, land_r, land_c, moves, move_count, extended, jumped);
        
        if (new_count == move_count) {
            // No more jumps found, this is a complete move
            moves[move_count++] = extended;
        } else {
            move_count = new_count;
        }
        
        // Unmark for next iteration
        jumped[mid_r][mid_c] = false;
    }
    
    return move_count;
}

int checkers_get_all_moves(CheckersGameState *game, CheckersColor color, CheckersMove *moves) {
    int move_count = 0;
    
    // First, look for jumps (forced if available)
    for (int r = 0; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            if (game->board[r][c].color == color) {
                CheckersMove base_move = {r, c, r, c, 0, {0}, {0}, false};
                bool jumped[8][8] = {false};
                int new_count = find_jumps_from(game, r, c, moves, move_count, base_move, jumped);
                move_count = new_count;
            }
        }
    }
    
    // If jumps are available, they're forced
    if (move_count > 0) {
        return move_count;
    }
    
    // No jumps available, find regular moves
    for (int r = 0; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            CheckersPiece piece = game->board[r][c];
            if (piece.color != color) continue;
            
            // Possible move directions
            int dirs[4][2];
            int num_dirs = 0;
            
            if (piece.is_king) {
                dirs[0][0] = -1; dirs[0][1] = -1;
                dirs[1][0] = -1; dirs[1][1] = 1;
                dirs[2][0] = 1;  dirs[2][1] = -1;
                dirs[3][0] = 1;  dirs[3][1] = 1;
                num_dirs = 4;
            } else if (piece.color == CHECKERS_RED) {
                dirs[0][0] = -1; dirs[0][1] = -1;
                dirs[1][0] = -1; dirs[1][1] = 1;
                num_dirs = 2;
            } else {
                dirs[0][0] = 1; dirs[0][1] = -1;
                dirs[1][0] = 1; dirs[1][1] = 1;
                num_dirs = 2;
            }
            
            for (int d = 0; d < num_dirs; d++) {
                int new_r = r + dirs[d][0];
                int new_c = c + dirs[d][1];
                
                if (new_r < 0 || new_r >= CHECKERS_BOARD_SIZE || 
                    new_c < 0 || new_c >= CHECKERS_BOARD_SIZE) continue;
                
                if (game->board[new_r][new_c].color == CHECKERS_NONE) {
                    CheckersMove move = {r, c, new_r, new_c, 0, {0}, {0}, false};
                    
                    // Check for king promotion
                    if (!piece.is_king) {
                        if ((piece.color == CHECKERS_RED && new_r == 0) ||
                            (piece.color == CHECKERS_BLACK && new_r == 7)) {
                            move.becomes_king = true;
                        }
                    }
                    
                    moves[move_count++] = move;
                }
            }
        }
    }
    
    return move_count;
}

void checkers_make_move(CheckersGameState *game, CheckersMove *move) {
    CheckersPiece piece = game->board[move->from_row][move->from_col];
    
    // Move piece
    game->board[move->to_row][move->to_col] = piece;
    game->board[move->from_row][move->from_col].color = CHECKERS_NONE;
    game->board[move->from_row][move->from_col].is_king = false;
    
    // Remove jumped pieces
    for (int i = 0; i < move->jump_count; i++) {
        CheckersColor jumped_color = game->board[move->jumped_rows[i]][move->jumped_cols[i]].color;
        game->board[move->jumped_rows[i]][move->jumped_cols[i]].color = CHECKERS_NONE;
        game->board[move->jumped_rows[i]][move->jumped_cols[i]].is_king = false;
        
        if (jumped_color == CHECKERS_RED) game->red_pieces--;
        else if (jumped_color == CHECKERS_BLACK) game->black_pieces--;
    }
    
    // King promotion
    if (move->becomes_king) {
        game->board[move->to_row][move->to_col].is_king = true;
    }
    
    // Switch turn
    game->turn = (game->turn == CHECKERS_RED) ? CHECKERS_BLACK : CHECKERS_RED;
}

int checkers_evaluate_position(CheckersGameState *game) {
    int score = 0;
    
    // Piece-square table - favor center and advancing
    int position_value[8][8] = {
        {4, 4, 4, 4, 4, 4, 4, 4},
        {3, 4, 4, 4, 4, 4, 4, 3},
        {3, 3, 5, 5, 5, 5, 3, 3},
        {2, 3, 3, 6, 6, 3, 3, 2},
        {2, 3, 3, 6, 6, 3, 3, 2},
        {3, 3, 5, 5, 5, 5, 3, 3},
        {3, 4, 4, 4, 4, 4, 4, 3},
        {4, 4, 4, 4, 4, 4, 4, 4}
    };
    
    for (int r = 0; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            CheckersPiece piece = game->board[r][c];
            if (piece.color == CHECKERS_NONE) continue;
            
            int value = piece.is_king ? 300 : 100;
            int pos_value = position_value[r][c];
            
            // Bonus for advancement (for regular pieces)
            if (!piece.is_king) {
                if (piece.color == CHECKERS_RED) {
                    value += (7 - r) * 3;  // Red advances upward
                } else {
                    value += r * 3;  // Black advances downward
                }
            }
            
            // Back row bonus (defensive)
            if ((piece.color == CHECKERS_RED && r == 7) ||
                (piece.color == CHECKERS_BLACK && r == 0)) {
                value += 5;
            }
            
            int total = value + pos_value;
            score += (piece.color == CHECKERS_RED) ? total : -total;
        }
    }
    
    // Mobility bonus
    CheckersMove moves[MAX_CHECKERS_MOVES];
    int red_mobility = checkers_get_all_moves(game, CHECKERS_RED, moves);
    int black_mobility = checkers_get_all_moves(game, CHECKERS_BLACK, moves);
    score += (red_mobility - black_mobility) * 5;
    
    return score;
}

CheckersGameStatus checkers_check_game_status(CheckersGameState *game) {
    if (game->red_pieces == 0) return CHECKERS_BLACK_WINS;
    if (game->black_pieces == 0) return CHECKERS_RED_WINS;
    
    CheckersMove moves[MAX_CHECKERS_MOVES];
    int move_count = checkers_get_all_moves(game, game->turn, moves);
    
    if (move_count == 0) {
        return (game->turn == CHECKERS_RED) ? CHECKERS_BLACK_WINS : CHECKERS_RED_WINS;
    }
    
    return CHECKERS_PLAYING;
}

// ============================================================================
// AI / THINKING
// ============================================================================

static int checkers_minimax(CheckersGameState *game, int depth, int alpha, int beta, bool maximizing) {
    CheckersMove moves[MAX_CHECKERS_MOVES];
    int move_count = checkers_get_all_moves(game, game->turn, moves);
    
    if (move_count == 0) {
        return maximizing ? -1000000 : 1000000;
    }
    
    if (depth == 0) {
        return checkers_evaluate_position(game);
    }
    
    if (maximizing) {
        int max_eval = INT_MIN;
        for (int i = 0; i < move_count; i++) {
            CheckersGameState temp = *game;
            checkers_make_move(&temp, &moves[i]);
            int eval = checkers_minimax(&temp, depth - 1, alpha, beta, false);
            max_eval = (eval > max_eval) ? eval : max_eval;
            alpha = (alpha > eval) ? alpha : eval;
            if (beta <= alpha) break;
        }
        return max_eval;
    } else {
        int min_eval = INT_MAX;
        for (int i = 0; i < move_count; i++) {
            CheckersGameState temp = *game;
            checkers_make_move(&temp, &moves[i]);
            int eval = checkers_minimax(&temp, depth - 1, alpha, beta, true);
            min_eval = (eval < min_eval) ? eval : min_eval;
            beta = (beta < eval) ? beta : eval;
            if (beta <= alpha) break;
        }
        return min_eval;
    }
}

void* checkers_think_continuously(void* arg) {
    CheckersThinkingState *ts = (CheckersThinkingState*)arg;
    
    while (true) {
        pthread_mutex_lock(&ts->lock);
        if (!ts->thinking) {
            pthread_mutex_unlock(&ts->lock);
            usleep(10000);
            continue;
        }
        
        CheckersGameState game_copy = ts->game;
        pthread_mutex_unlock(&ts->lock);
        
        CheckersMove moves[MAX_CHECKERS_MOVES];
        int move_count = checkers_get_all_moves(&game_copy, game_copy.turn, moves);
        
        if (move_count == 0) {
            pthread_mutex_lock(&ts->lock);
            ts->has_move = false;
            ts->thinking = false;
            pthread_mutex_unlock(&ts->lock);
            continue;
        }
        
        // Iterative deepening - up to depth 6 (checkers is simpler than chess)
        for (int depth = 1; depth <= 6; depth++) {
            CheckersMove best_moves[MAX_CHECKERS_MOVES];
            int best_move_count = 0;
            int best_score = (game_copy.turn == CHECKERS_RED) ? INT_MIN : INT_MAX;
            
            bool depth_completed = true;
            
            for (int i = 0; i < move_count; i++) {
                pthread_mutex_lock(&ts->lock);
                bool should_stop = !ts->thinking;
                pthread_mutex_unlock(&ts->lock);
                
                if (should_stop) {
                    depth_completed = false;
                    break;
                }
                
                CheckersGameState temp = game_copy;
                checkers_make_move(&temp, &moves[i]);
                int score = checkers_minimax(&temp, depth - 1, INT_MIN, INT_MAX, 
                                            game_copy.turn == CHECKERS_BLACK);
                
                if (game_copy.turn == CHECKERS_RED) {
                    if (score > best_score) {
                        best_score = score;
                        best_moves[0] = moves[i];
                        best_move_count = 1;
                    } else if (score == best_score) {
                        best_moves[best_move_count++] = moves[i];
                    }
                } else {
                    if (score < best_score) {
                        best_score = score;
                        best_moves[0] = moves[i];
                        best_move_count = 1;
                    } else if (score == best_score) {
                        best_moves[best_move_count++] = moves[i];
                    }
                }
            }
            
            pthread_mutex_lock(&ts->lock);
            if (depth_completed && ts->thinking && best_move_count > 0) {
                ts->best_move = best_moves[rand() % best_move_count];
                ts->best_score = best_score;
                ts->current_depth = depth;
                ts->has_move = true;
            }
            pthread_mutex_unlock(&ts->lock);
        }
        
        pthread_mutex_lock(&ts->lock);
        ts->thinking = false;
        pthread_mutex_unlock(&ts->lock);
    }
    
    return NULL;
}

void checkers_init_thinking_state(CheckersThinkingState *ts) {
    ts->thinking = false;
    ts->has_move = false;
    ts->current_depth = 0;
    ts->best_score = 0;
    pthread_mutex_init(&ts->lock, NULL);
    pthread_create(&ts->thread, NULL, checkers_think_continuously, ts);
}

void checkers_start_thinking(CheckersThinkingState *ts, CheckersGameState *game) {
    pthread_mutex_lock(&ts->lock);
    ts->game = *game;
    ts->thinking = true;
    ts->has_move = false;
    ts->current_depth = 0;
    pthread_mutex_unlock(&ts->lock);
}

CheckersMove checkers_get_best_move_now(CheckersThinkingState *ts) {
    pthread_mutex_lock(&ts->lock);
    CheckersMove move = ts->best_move;
    bool has_move = ts->has_move;
    ts->thinking = false;
    pthread_mutex_unlock(&ts->lock);
    
    if (!has_move) {
        CheckersMove moves[MAX_CHECKERS_MOVES];
        int count = checkers_get_all_moves(&ts->game, ts->game.turn, moves);
        if (count > 0) {
            move = moves[rand() % count];
        }
    }
    
    return move;
}

void checkers_stop_thinking(CheckersThinkingState *ts) {
    pthread_mutex_lock(&ts->lock);
    ts->thinking = false;
    pthread_mutex_unlock(&ts->lock);
}

// ============================================================================
// VISUALIZATION
// ============================================================================

void init_beat_checkers_system(void *vis_ptr) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatCheckersVisualization *checkers = &vis->beat_checkers;
    
    checkers_init_board(&checkers->game);
    checkers_init_thinking_state(&checkers->thinking_state);
    checkers->status = CHECKERS_PLAYING;
    
    checkers_start_thinking(&checkers->thinking_state, &checkers->game);
    
    checkers->last_from_row = -1;
    checkers->last_from_col = -1;
    checkers->last_to_row = -1;
    checkers->last_to_col = -1;
    checkers->last_move_glow = 0;
    
    checkers->is_animating = false;
    checkers->animation_progress = 0;
    checkers->jump_animation_index = 0;
    
    checkers->captured_count = 0;
    
    strcpy(checkers->status_text, "Red to move");
    checkers->status_flash_timer = 0;
    checkers->status_flash_color[0] = 1.0;
    checkers->status_flash_color[1] = 1.0;
    checkers->status_flash_color[2] = 1.0;
    
    for (int i = 0; i < CHECKERS_BEAT_HISTORY; i++) {
        checkers->beat_volume_history[i] = 0;
    }
    checkers->beat_history_index = 0;
    checkers->time_since_last_move = 0;
    checkers->beat_threshold = 1.3;
    
    checkers->move_count = 0;
    checkers->beats_since_game_over = 0;
    checkers->waiting_for_restart = false;
    
    checkers->time_thinking = 0;
    checkers->min_think_time = 0.4;
    checkers->good_move_threshold = 100;
    checkers->auto_play_enabled = true;
    
    checkers->king_promotion_active = false;
    checkers->king_promotion_glow = 0;
    
    printf("Beat checkers system initialized\n");
}

bool beat_checkers_detect_beat(void *vis_ptr) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatCheckersVisualization *checkers = &vis->beat_checkers;
    
    checkers->beat_volume_history[checkers->beat_history_index] = vis->volume_level;
    checkers->beat_history_index = (checkers->beat_history_index + 1) % CHECKERS_BEAT_HISTORY;
    
    double avg = 0;
    for (int i = 0; i < CHECKERS_BEAT_HISTORY; i++) {
        avg += checkers->beat_volume_history[i];
    }
    avg /= CHECKERS_BEAT_HISTORY;
    
    if (vis->volume_level > avg * checkers->beat_threshold && 
        vis->volume_level > 0.05 &&
        checkers->time_since_last_move > 0.15) {
        return true;
    }
    
    return false;
}

void update_beat_checkers(void *vis_ptr, double dt) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatCheckersVisualization *checkers = &vis->beat_checkers;
    
    checkers->time_since_last_move += dt;
    
    // Update glow effects
    if (checkers->last_move_glow > 0) {
        checkers->last_move_glow -= dt * 2.0;
        if (checkers->last_move_glow < 0) checkers->last_move_glow = 0;
    }
    
    if (checkers->status_flash_timer > 0) {
        checkers->status_flash_timer -= dt * 2.0;
        if (checkers->status_flash_timer < 0) checkers->status_flash_timer = 0;
    }
    
    if (checkers->king_promotion_glow > 0) {
        checkers->king_promotion_glow -= dt * 1.5;
        if (checkers->king_promotion_glow < 0) {
            checkers->king_promotion_glow = 0;
            checkers->king_promotion_active = false;
        }
    }
    
    // Fade out captured pieces
    for (int i = 0; i < checkers->captured_count; i++) {
        checkers->captured_fade[i] -= dt * 2.0;
        if (checkers->captured_fade[i] < 0) checkers->captured_fade[i] = 0;
    }
    
    // Animate piece movement
    if (checkers->is_animating) {
        checkers->animation_progress += dt * 5.0;
        if (checkers->animation_progress >= 1.0) {
            checkers->animation_progress = 1.0;
            checkers->is_animating = false;
        }
    }
    
    // Handle game over
    if (checkers->status != CHECKERS_PLAYING) {
        if (checkers->waiting_for_restart) {
            if (beat_checkers_detect_beat(vis)) {
                checkers->beats_since_game_over++;
                checkers->time_since_last_move = 0;
                
                if (checkers->beats_since_game_over >= 2) {
                    checkers_init_board(&checkers->game);
                    checkers->status = CHECKERS_PLAYING;
                    checkers->beats_since_game_over = 0;
                    checkers->waiting_for_restart = false;
                    checkers->move_count = 0;
                    checkers->time_thinking = 0;
                    checkers->captured_count = 0;
                    strcpy(checkers->status_text, "New game! Red to move");
                    checkers->status_flash_color[0] = 1.0;
                    checkers->status_flash_color[1] = 0.3;
                    checkers->status_flash_color[2] = 0.3;
                    checkers->status_flash_timer = 1.0;
                    
                    checkers_start_thinking(&checkers->thinking_state, &checkers->game);
                }
            }
        }
        return;
    }
    
    // Track thinking time
    pthread_mutex_lock(&checkers->thinking_state.lock);
    bool is_thinking = checkers->thinking_state.thinking;
    bool has_move = checkers->thinking_state.has_move;
    int current_depth = checkers->thinking_state.current_depth;
    int best_score = checkers->thinking_state.best_score;
    pthread_mutex_unlock(&checkers->thinking_state.lock);
    
    if (is_thinking || has_move) {
        checkers->time_thinking += dt;
    }
    
    // Auto-play logic
    bool should_auto_play = false;
    if (checkers->auto_play_enabled && has_move && 
        checkers->time_thinking >= checkers->min_think_time) {
        
        if (checkers->time_thinking >= 3.0) {
            should_auto_play = true;
        } else if (current_depth >= 4) {
            should_auto_play = true;
        } else {
            int eval_before = checkers_evaluate_position(&checkers->game);
            int advantage = (checkers->game.turn == CHECKERS_RED) ? 
                           (best_score - eval_before) : (eval_before - best_score);
            
            if (advantage > checkers->good_move_threshold && current_depth >= 3) {
                should_auto_play = true;
            }
        }
    }
    
    // Detect beat or auto-play
    bool beat_detected = beat_checkers_detect_beat(vis);
    
    if (beat_detected || should_auto_play) {
        CheckersMove move = checkers_get_best_move_now(&checkers->thinking_state);
        
        pthread_mutex_lock(&checkers->thinking_state.lock);
        int depth_reached = checkers->thinking_state.current_depth;
        pthread_mutex_unlock(&checkers->thinking_state.lock);
        
        CheckersColor moving_color = checkers->game.turn;
        
        // Store captured pieces for fade animation
        checkers->captured_count = move.jump_count;
        for (int i = 0; i < move.jump_count; i++) {
            int idx = move.jumped_rows[i] * CHECKERS_BOARD_SIZE + move.jumped_cols[i];
            checkers->captured_pieces[i] = idx;
            checkers->captured_fade[i] = 1.0;
        }
        
        // Make the move
        bool was_king_promotion = move.becomes_king;
        checkers_make_move(&checkers->game, &move);
        
        // King promotion celebration
        if (was_king_promotion) {
            checkers->king_promotion_active = true;
            checkers->king_promotion_glow = 1.5;
            checkers->king_promotion_row = move.to_row;
            checkers->king_promotion_col = move.to_col;
        }
        
        // Update display
        checkers->last_from_row = move.from_row;
        checkers->last_from_col = move.from_col;
        checkers->last_to_row = move.to_row;
        checkers->last_to_col = move.to_col;
        checkers->last_move_glow = 1.0;
        
        // Animation
        checkers->animating_from_row = move.from_row;
        checkers->animating_from_col = move.from_col;
        checkers->animating_to_row = move.to_row;
        checkers->animating_to_col = move.to_col;
        checkers->animation_progress = 0;
        checkers->is_animating = true;
        checkers->current_jump_chain = move;
        checkers->jump_animation_index = 0;
        
        // Status text
        const char *trigger = should_auto_play ? "AUTO" : "BEAT";
        const char *color_name = (moving_color == CHECKERS_RED) ? "Red" : "Black";
        
        if (move.jump_count > 0) {
            if (move.jump_count > 2) {
                snprintf(checkers->status_text, sizeof(checkers->status_text),
                        "[%s] %s: Multi-jump x%d! %c%d->%c%d (depth %d)",
                        trigger, color_name, move.jump_count,
                        'a' + move.from_col, 8 - move.from_row,
                        'a' + move.to_col, 8 - move.to_row, depth_reached);
                checkers->status_flash_color[0] = 1.0;
                checkers->status_flash_color[1] = 0.5;
                checkers->status_flash_color[2] = 0.0;
                checkers->status_flash_timer = 1.0;
            } else {
                snprintf(checkers->status_text, sizeof(checkers->status_text),
                        "[%s] %s: Jump %c%d->%c%d (depth %d)",
                        trigger, color_name,
                        'a' + move.from_col, 8 - move.from_row,
                        'a' + move.to_col, 8 - move.to_row, depth_reached);
            }
        } else {
            snprintf(checkers->status_text, sizeof(checkers->status_text),
                    "[%s] %s: %c%d->%c%d (depth %d)",
                    trigger, color_name,
                    'a' + move.from_col, 8 - move.from_row,
                    'a' + move.to_col, 8 - move.to_row, depth_reached);
        }
        
        if (was_king_promotion) {
            strcat(checkers->status_text, " - KING!");
            checkers->status_flash_color[0] = 1.0;
            checkers->status_flash_color[1] = 0.8;
            checkers->status_flash_color[2] = 0.0;
            checkers->status_flash_timer = 1.5;
        }
        
        checkers->move_count++;
        checkers->time_since_last_move = 0;
        checkers->time_thinking = 0;
        
        // Check game status
        checkers->status = checkers_check_game_status(&checkers->game);
        
        if (checkers->status != CHECKERS_PLAYING) {
            checkers->waiting_for_restart = true;
            checkers->beats_since_game_over = 0;
            
            if (checkers->status == CHECKERS_RED_WINS) {
                strcpy(checkers->status_text, "Red wins! New game in 2 beats...");
                checkers->status_flash_color[0] = 1.0;
                checkers->status_flash_color[1] = 0.2;
                checkers->status_flash_color[2] = 0.2;
                checkers->status_flash_timer = 2.0;
            } else if (checkers->status == CHECKERS_BLACK_WINS) {
                strcpy(checkers->status_text, "Black wins! New game in 2 beats...");
                checkers->status_flash_color[0] = 0.2;
                checkers->status_flash_color[1] = 0.2;
                checkers->status_flash_color[2] = 0.2;
                checkers->status_flash_timer = 2.0;
            } else {
                strcpy(checkers->status_text, "Draw! New game in 2 beats...");
                checkers->status_flash_color[0] = 0.7;
                checkers->status_flash_color[1] = 0.7;
                checkers->status_flash_color[2] = 0.7;
                checkers->status_flash_timer = 2.0;
            }
        } else {
            checkers_start_thinking(&checkers->thinking_state, &checkers->game);
        }
    }
}

// ============================================================================
// DRAWING
// ============================================================================

void draw_checkers_piece(cairo_t *cr, CheckersColor color, bool is_king, 
                         double x, double y, double size, double dance_offset) {
    double cx = x + size / 2;
    double cy = y + size / 2 + dance_offset;
    double radius = size * 0.35;
    
    // Shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
    cairo_arc(cr, cx + 3, cy + 3, radius, 0, 2 * M_PI);
    cairo_fill(cr);
    
    // Piece body with gradient
    cairo_pattern_t *gradient = cairo_pattern_create_radial(
        cx - radius * 0.3, cy - radius * 0.3, radius * 0.1,
        cx, cy, radius);
    
    if (color == CHECKERS_RED) {
        cairo_pattern_add_color_stop_rgb(gradient, 0, 0.95, 0.3, 0.2);
        cairo_pattern_add_color_stop_rgb(gradient, 1, 0.7, 0.1, 0.05);
    } else {
        cairo_pattern_add_color_stop_rgb(gradient, 0, 0.3, 0.3, 0.3);
        cairo_pattern_add_color_stop_rgb(gradient, 1, 0.1, 0.1, 0.1);
    }
    
    cairo_set_source(cr, gradient);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(gradient);
    
    // Outline
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_set_line_width(cr, 2.0);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Highlight
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
    cairo_arc(cr, cx - radius * 0.25, cy - radius * 0.25, radius * 0.2, 0, 2 * M_PI);
    cairo_fill(cr);
    
    // King crown
    if (is_king) {
        double crown_size = radius * 0.5;
        
        if (color == CHECKERS_RED) {
            cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);
        } else {
            cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
        }
        
        // Simple crown shape
        cairo_move_to(cr, cx - crown_size, cy + crown_size * 0.2);
        cairo_line_to(cr, cx - crown_size * 0.6, cy - crown_size * 0.5);
        cairo_line_to(cr, cx - crown_size * 0.3, cy - crown_size * 0.2);
        cairo_line_to(cr, cx, cy - crown_size * 0.7);
        cairo_line_to(cr, cx + crown_size * 0.3, cy - crown_size * 0.2);
        cairo_line_to(cr, cx + crown_size * 0.6, cy - crown_size * 0.5);
        cairo_line_to(cr, cx + crown_size, cy + crown_size * 0.2);
        cairo_close_path(cr);
        cairo_fill(cr);
        
        // Crown outline
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, cx - crown_size, cy + crown_size * 0.2);
        cairo_line_to(cr, cx - crown_size * 0.6, cy - crown_size * 0.5);
        cairo_line_to(cr, cx - crown_size * 0.3, cy - crown_size * 0.2);
        cairo_line_to(cr, cx, cy - crown_size * 0.7);
        cairo_line_to(cr, cx + crown_size * 0.3, cy - crown_size * 0.2);
        cairo_line_to(cr, cx + crown_size * 0.6, cy - crown_size * 0.5);
        cairo_line_to(cr, cx + crown_size, cy + crown_size * 0.2);
        cairo_close_path(cr);
        cairo_stroke(cr);
    }
}

void draw_beat_checkers(void *vis_ptr, cairo_t *cr) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatCheckersVisualization *checkers = &vis->beat_checkers;
    
    int width = vis->width;
    int height = vis->height;
    
    // Calculate board layout
    double available = fmin(width * 0.85, height * 0.85);
    checkers->cell_size = available / 8;
    checkers->board_offset_x = (width - checkers->cell_size * 8) / 2;
    checkers->board_offset_y = (height - checkers->cell_size * 8) / 2 + 20;
    
    double cell = checkers->cell_size;
    double ox = checkers->board_offset_x;
    double oy = checkers->board_offset_y;
    
    // Background gradient
    cairo_pattern_t *bg = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(bg, 0, 0.15, 0.15, 0.18);
    cairo_pattern_add_color_stop_rgb(bg, 1, 0.08, 0.08, 0.1);
    cairo_set_source(cr, bg);
    cairo_paint(cr);
    cairo_pattern_destroy(bg);
    
    // Draw board squares
    for (int r = 0; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            bool is_light = (r + c) % 2 == 0;
            
            if (is_light) {
                cairo_set_source_rgb(cr, 0.85, 0.8, 0.7);
            } else {
                cairo_set_source_rgb(cr, 0.3, 0.25, 0.2);
            }
            
            cairo_rectangle(cr, ox + c * cell, oy + r * cell, cell, cell);
            cairo_fill(cr);
        }
    }
    
    // Last move highlight
    if (checkers->last_from_row >= 0 && checkers->last_move_glow > 0) {
        double alpha = checkers->last_move_glow * 0.4;
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.3, alpha);
        cairo_rectangle(cr, ox + checkers->last_from_col * cell, 
                       oy + checkers->last_from_row * cell, cell, cell);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.3, alpha);
        cairo_rectangle(cr, ox + checkers->last_to_col * cell, 
                       oy + checkers->last_to_row * cell, cell, cell);
        cairo_fill(cr);
    }
    
    // Board border
    cairo_set_source_rgb(cr, 0.5, 0.4, 0.3);
    cairo_set_line_width(cr, 4);
    cairo_rectangle(cr, ox, oy, cell * 8, cell * 8);
    cairo_stroke(cr);
    
    // Coordinates
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, cell * 0.18);
    
    for (int i = 0; i < 8; i++) {
        char label[2];
        label[0] = 'a' + i;
        label[1] = '\0';
        cairo_move_to(cr, ox + i * cell + cell * 0.45, oy + 8 * cell + cell * 0.3);
        cairo_show_text(cr, label);
        
        label[0] = '8' - i;
        cairo_move_to(cr, ox - cell * 0.3, oy + i * cell + cell * 0.55);
        cairo_show_text(cr, label);
    }
    
    // Draw fading captured pieces
    for (int i = 0; i < checkers->captured_count; i++) {
        if (checkers->captured_fade[i] > 0) {
            int idx = checkers->captured_pieces[i];
            int r = idx / CHECKERS_BOARD_SIZE;
            int c = idx % CHECKERS_BOARD_SIZE;
            
            double x = ox + c * cell;
            double y = oy + r * cell;
            
            cairo_save(cr);
            cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, checkers->captured_fade[i]);
            cairo_arc(cr, x + cell/2, y + cell/2, cell * 0.35, 0, 2 * M_PI);
            cairo_fill(cr);
            cairo_restore(cr);
        }
    }
    
    // Draw pieces with music-reactive dance
    double volume = vis->volume_level;
    
    for (int r = 0; r < CHECKERS_BOARD_SIZE; r++) {
        for (int c = 0; c < CHECKERS_BOARD_SIZE; c++) {
            CheckersPiece piece = checkers->game.board[r][c];
            
            // Skip animating piece
            if (checkers->is_animating && 
                r == checkers->animating_from_row && 
                c == checkers->animating_from_col) {
                continue;
            }
            
            if (piece.color != CHECKERS_NONE) {
                double x = ox + c * cell;
                double y = oy + r * cell;
                
                // Dance to the music
                double phase = (r * 0.7 + c * 0.5) * 3.14159;
                double time_wave = sin(checkers->time_since_last_move * 12.0 + phase);
                double dance_amount = time_wave * volume * cell * 0.15;
                
                // King promotion glow
                if (checkers->king_promotion_active && 
                    r == checkers->king_promotion_row && 
                    c == checkers->king_promotion_col) {
                    cairo_set_source_rgba(cr, 1.0, 0.8, 0.0, checkers->king_promotion_glow * 0.5);
                    cairo_arc(cr, x + cell/2, y + cell/2, cell * 0.5, 0, 2 * M_PI);
                    cairo_fill(cr);
                }
                
                draw_checkers_piece(cr, piece.color, piece.is_king, x, y, cell, dance_amount);
            }
        }
    }
    
    // Draw animating piece
    if (checkers->is_animating) {
        int fr = checkers->animating_from_row;
        int fc = checkers->animating_from_col;
        int tr = checkers->animating_to_row;
        int tc = checkers->animating_to_col;
        
        CheckersPiece piece = checkers->game.board[tr][tc];
        
        double t = checkers->animation_progress;
        t = t * t * (3.0 - 2.0 * t);  // Smoothstep
        
        double x = ox + (fc + t * (tc - fc)) * cell;
        double y = oy + (fr + t * (tr - fr)) * cell;
        
        double dance = sin(checkers->time_since_last_move * 18.0) * volume * cell * 0.25;
        
        // Glow for moving piece
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.5, 0.6);
        cairo_arc(cr, x + cell/2, y + cell/2 + dance, cell * 0.45, 0, 2 * M_PI);
        cairo_fill(cr);
        
        draw_checkers_piece(cr, piece.color, piece.is_king, x, y, cell, dance);
    }
    
    // Status text at top
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, checkers->status_text, &extents);
    
    double text_x = (width - extents.width) / 2;
    double text_y = oy - 25;
    
    if (checkers->status_flash_timer > 0) {
        double alpha = checkers->status_flash_timer * 0.3;
        cairo_set_source_rgba(cr, 
                            checkers->status_flash_color[0],
                            checkers->status_flash_color[1],
                            checkers->status_flash_color[2],
                            alpha);
        cairo_rectangle(cr, text_x - 10, text_y - extents.height - 5, 
                       extents.width + 20, extents.height + 10);
        cairo_fill(cr);
    }
    
    if (checkers->status_flash_timer > 0) {
        cairo_set_source_rgb(cr,
                           checkers->status_flash_color[0],
                           checkers->status_flash_color[1],
                           checkers->status_flash_color[2]);
    } else {
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    }
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, checkers->status_text);
    
    // Piece count
    char count_text[64];
    snprintf(count_text, sizeof(count_text), "Red: %d | Black: %d | Move: %d", 
             checkers->game.red_pieces, checkers->game.black_pieces, checkers->move_count);
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_font_size(cr, 14);
    cairo_text_extents(cr, count_text, &extents);
    cairo_move_to(cr, (width - extents.width) / 2, oy + cell * 8 + 35);
    cairo_show_text(cr, count_text);
}

void checkers_cleanup_thinking_state(CheckersThinkingState *ts) {
    // Stop thinking
    pthread_mutex_lock(&ts->lock);
    ts->thinking = false;
    pthread_mutex_unlock(&ts->lock);
    
    // Cancel and wait for thread to finish
    pthread_cancel(ts->thread);
    pthread_join(ts->thread, NULL);
    
    // Destroy mutex
    pthread_mutex_destroy(&ts->lock);
}
