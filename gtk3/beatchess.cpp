#include "beatchess.h"
#include "visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

// ============================================================================
// CORE CHESS ENGINE
// ============================================================================

#define MAX_MOVES_BEFORE_DRAW 150

bool chess_is_in_bounds(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

bool chess_is_path_clear(ChessGameState *game, int fr, int fc, int tr, int tc) {
    int dr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
    int dc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
    
    int r = fr + dr;
    int c = fc + dc;
    
    while (r != tr || c != tc) {
        if (game->board[r][c].type != EMPTY) return false;
        r += dr;
        c += dc;
    }
    return true;
}

void chess_init_board(ChessGameState *game) {
    // Clear board
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            game->board[r][c].type = EMPTY;
            game->board[r][c].color = NONE;
        }
    }
    
    // Set up pieces
    PieceType back_row[] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    
    for (int c = 0; c < BOARD_SIZE; c++) {
        game->board[0][c].type = back_row[c];
        game->board[0][c].color = BLACK;
        game->board[1][c].type = PAWN;
        game->board[1][c].color = BLACK;
        
        game->board[6][c].type = PAWN;
        game->board[6][c].color = WHITE;
        game->board[7][c].type = back_row[c];
        game->board[7][c].color = WHITE;
    }
    
    game->turn = WHITE;
    game->white_king_moved = false;
    game->black_king_moved = false;
    game->white_rook_a_moved = false;
    game->white_rook_h_moved = false;
    game->black_rook_a_moved = false;
    game->black_rook_h_moved = false;
    game->en_passant_col = -1;
    game->en_passant_row = -1;
}

bool chess_is_valid_move(ChessGameState *game, int fr, int fc, int tr, int tc) {
    if (!chess_is_in_bounds(fr, fc) || !chess_is_in_bounds(tr, tc)) return false;
    if (fr == tr && fc == tc) return false;
    
    ChessPiece piece = game->board[fr][fc];
    ChessPiece target = game->board[tr][tc];
    
    if (piece.type == EMPTY || piece.color != game->turn) return false;
    if (target.color == piece.color) return false;
    
    int dr = tr - fr;
    int dc = tc - fc;
    
    switch (piece.type) {
        case PAWN: {
            int direction = (piece.color == WHITE) ? -1 : 1;
            int start_row = (piece.color == WHITE) ? 6 : 1;
            
            // Normal pawn moves
            if (dc == 0 && target.type == EMPTY) {
                if (dr == direction) return true;
                if (fr == start_row && dr == 2 * direction && 
                    game->board[fr + direction][fc].type == EMPTY) return true;
            }
            // Regular captures
            if (abs(dc) == 1 && dr == direction && target.type != EMPTY) return true;
            
            // En passant capture
            if (abs(dc) == 1 && dr == direction && target.type == EMPTY) {
                if (game->en_passant_col == tc && game->en_passant_row == tr) {
                    return true;
                }
            }
            return false;
        }
        
        case KNIGHT:
            return (abs(dr) == 2 && abs(dc) == 1) || (abs(dr) == 1 && abs(dc) == 2);
        
        case BISHOP:
            return abs(dr) == abs(dc) && abs(dr) > 0 && chess_is_path_clear(game, fr, fc, tr, tc);
        
        case ROOK:
            return (dr == 0 || dc == 0) && (dr != 0 || dc != 0) && chess_is_path_clear(game, fr, fc, tr, tc);
        
        case QUEEN:
            return ((dr == 0 || dc == 0) || (abs(dr) == abs(dc))) && 
                   (dr != 0 || dc != 0) &&
                   chess_is_path_clear(game, fr, fc, tr, tc);
        
        case KING: {
            // Normal king move
            if (abs(dr) <= 1 && abs(dc) <= 1 && (dr != 0 || dc != 0)) {
                return true;
            }
            
            // Castling
            if (dr == 0 && abs(dc) == 2) {
                // Must not have moved king
                if (piece.color == WHITE && game->white_king_moved) return false;
                if (piece.color == BLACK && game->black_king_moved) return false;
                
                // Kingside castling
                if (dc == 2) {
                    if (piece.color == WHITE && game->white_rook_h_moved) return false;
                    if (piece.color == BLACK && game->black_rook_h_moved) return false;
                    
                    // Check path is clear
                    if (!chess_is_path_clear(game, fr, fc, tr, 7)) return false;
                    
                    // Check king doesn't move through check
                    if (chess_is_in_check(game, piece.color)) return false;
                    
                    ChessGameState temp = *game;
                    temp.board[fr][fc+1] = piece;
                    temp.board[fr][fc].type = EMPTY;
                    if (chess_is_in_check(&temp, piece.color)) return false;
                    
                    return true;
                }
                
                // Queenside castling
                if (dc == -2) {
                    if (piece.color == WHITE && game->white_rook_a_moved) return false;
                    if (piece.color == BLACK && game->black_rook_a_moved) return false;
                    
                    // Check path is clear
                    if (!chess_is_path_clear(game, fr, fc, tr, 0)) return false;
                    
                    // Check king doesn't move through check
                    if (chess_is_in_check(game, piece.color)) return false;
                    
                    ChessGameState temp = *game;
                    temp.board[fr][fc-1] = piece;
                    temp.board[fr][fc].type = EMPTY;
                    if (chess_is_in_check(&temp, piece.color)) return false;
                    
                    return true;
                }
            }
            return false;
        }
        
        default:
            return false;
    }
}

bool chess_is_in_check(ChessGameState *game, ChessColor color) {
    int king_r = -1, king_c = -1;
    
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (game->board[r][c].type == KING && game->board[r][c].color == color) {
                king_r = r;
                king_c = c;
                break;
            }
        }
        if (king_r != -1) break;
    }
    
    if (king_r == -1) return false;
    
    ChessColor opponent = (color == WHITE) ? BLACK : WHITE;
    
    // Check each opponent piece to see if it can attack the king
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            ChessPiece piece = game->board[r][c];
            if (piece.color != opponent) continue;
            
            // Temporarily set turn to opponent to check if move is valid
            ChessColor saved_turn = game->turn;
            game->turn = opponent;
            bool can_attack = chess_is_valid_move(game, r, c, king_r, king_c);
            game->turn = saved_turn;
            
            if (can_attack) {
                return true;
            }
        }
    }
    
    return false;
}

void chess_make_move(ChessGameState *game, ChessMove move) {
    ChessPiece piece = game->board[move.from_row][move.from_col];
    
    // Clear en passant state before making move
    game->en_passant_col = -1;
    game->en_passant_row = -1;
    
    // Handle en passant capture
    if (piece.type == PAWN && move.to_col != move.from_col && 
        game->board[move.to_row][move.to_col].type == EMPTY) {
        // This is an en passant capture - remove the captured pawn
        int captured_pawn_row = (piece.color == WHITE) ? move.to_row + 1 : move.to_row - 1;
        game->board[captured_pawn_row][move.to_col].type = EMPTY;
        game->board[captured_pawn_row][move.to_col].color = NONE;
    }
    
    // Move the piece
    game->board[move.to_row][move.to_col] = piece;
    game->board[move.from_row][move.from_col].type = EMPTY;
    game->board[move.from_row][move.from_col].color = NONE;
    
    // Set en passant state if pawn moved two squares
    if (piece.type == PAWN && abs(move.to_row - move.from_row) == 2) {
        game->en_passant_col = move.to_col;
        game->en_passant_row = (move.from_row + move.to_row) / 2;
    }
    
    // Handle castling - move the rook too
    if (piece.type == KING && abs(move.to_col - move.from_col) == 2) {
        if (move.to_col > move.from_col) {
            // Kingside castle - move rook from h to f
            ChessPiece rook = game->board[move.from_row][7];
            game->board[move.from_row][5] = rook;
            game->board[move.from_row][7].type = EMPTY;
            game->board[move.from_row][7].color = NONE;
        } else {
            // Queenside castle - move rook from a to d
            ChessPiece rook = game->board[move.from_row][0];
            game->board[move.from_row][3] = rook;
            game->board[move.from_row][0].type = EMPTY;
            game->board[move.from_row][0].color = NONE;
        }
    }
    
    // Pawn promotion
    if (piece.type == PAWN) {
        if ((piece.color == WHITE && move.to_row == 0) || 
            (piece.color == BLACK && move.to_row == 7)) {
            // Promote to queen (90% of the time) or knight (10% for variety)
            game->board[move.to_row][move.to_col].type = (rand() % 10 == 0) ? KNIGHT : QUEEN;
        }
    }
    
    if (piece.type == KING) {
        if (piece.color == WHITE) game->white_king_moved = true;
        else game->black_king_moved = true;
    }
    if (piece.type == ROOK) {
        if (piece.color == WHITE) {
            if (move.from_col == 0) game->white_rook_a_moved = true;
            if (move.from_col == 7) game->white_rook_h_moved = true;
        } else {
            if (move.from_col == 0) game->black_rook_a_moved = true;
            if (move.from_col == 7) game->black_rook_h_moved = true;
        }
    }
    
    game->turn = (game->turn == WHITE) ? BLACK : WHITE;
}

int chess_evaluate_position(ChessGameState *game) {
    int piece_values[] = {0, 100, 320, 330, 500, 900, 20000};
    int score = 0;
    
    // Piece-square tables for positional bonuses
    int pawn_table[8][8] = {
        {0,  0,  0,  0,  0,  0,  0,  0},
        {50, 50, 50, 50, 50, 50, 50, 50},
        {10, 10, 20, 30, 30, 20, 10, 10},
        {5,  5, 10, 25, 25, 10,  5,  5},
        {0,  0,  0, 20, 20,  0,  0,  0},
        {5, -5,-10,  0,  0,-10, -5,  5},
        {5, 10, 10,-20,-20, 10, 10,  5},
        {0,  0,  0,  0,  0,  0,  0,  0}
    };
    
    int knight_table[8][8] = {
        {-50,-40,-30,-30,-30,-30,-40,-50},
        {-40,-20,  0,  0,  0,  0,-20,-40},
        {-30,  0, 10, 15, 15, 10,  0,-30},
        {-30,  5, 15, 20, 20, 15,  5,-30},
        {-30,  0, 15, 20, 20, 15,  0,-30},
        {-30,  5, 10, 15, 15, 10,  5,-30},
        {-40,-20,  0,  5,  5,  0,-20,-40},
        {-50,-40,-30,-30,-30,-30,-40,-50}
    };
    
    int bishop_table[8][8] = {
        {-20,-10,-10,-10,-10,-10,-10,-20},
        {-10,  0,  0,  0,  0,  0,  0,-10},
        {-10,  0,  5, 10, 10,  5,  0,-10},
        {-10,  5,  5, 10, 10,  5,  5,-10},
        {-10,  0, 10, 10, 10, 10,  0,-10},
        {-10, 10, 10, 10, 10, 10, 10,-10},
        {-10,  5,  0,  0,  0,  0,  5,-10},
        {-20,-10,-10,-10,-10,-10,-10,-20}
    };
    
    int king_middle_game[8][8] = {
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-20,-30,-30,-40,-40,-30,-30,-20},
        {-10,-20,-20,-20,-20,-20,-20,-10},
        { 20, 20,  0,  0,  0,  0, 20, 20},
        { 20, 30, 10,  0,  0, 10, 30, 20}
    };
    
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            ChessPiece p = game->board[r][c];
            if (p.type != EMPTY) {
                int value = piece_values[p.type];
                int positional_bonus = 0;
                
                // Add positional bonuses
                if (p.type == PAWN) {
                    if (p.color == WHITE) {
                        positional_bonus = pawn_table[r][c];
                        // Bonus for passed pawns
                        bool passed = true;
                        for (int check_r = r - 1; check_r >= 0; check_r--) {
                            for (int check_c = c - 1; check_c <= c + 1; check_c++) {
                                if (chess_is_in_bounds(check_r, check_c)) {
                                    if (game->board[check_r][check_c].type == PAWN && 
                                        game->board[check_r][check_c].color == BLACK) {
                                        passed = false;
                                    }
                                }
                            }
                        }
                        if (passed && r < 4) positional_bonus += 20;
                    } else {
                        positional_bonus = pawn_table[7-r][c];
                        // Bonus for passed pawns
                        bool passed = true;
                        for (int check_r = r + 1; check_r < 8; check_r++) {
                            for (int check_c = c - 1; check_c <= c + 1; check_c++) {
                                if (chess_is_in_bounds(check_r, check_c)) {
                                    if (game->board[check_r][check_c].type == PAWN && 
                                        game->board[check_r][check_c].color == WHITE) {
                                        passed = false;
                                    }
                                }
                            }
                        }
                        if (passed && r > 3) positional_bonus += 20;
                    }
                } else if (p.type == KNIGHT) {
                    if (p.color == WHITE) {
                        positional_bonus = knight_table[r][c];
                    } else {
                        positional_bonus = knight_table[7-r][c];
                    }
                } else if (p.type == BISHOP) {
                    if (p.color == WHITE) {
                        positional_bonus = bishop_table[r][c];
                    } else {
                        positional_bonus = bishop_table[7-r][c];
                    }
                } else if (p.type == KING) {
                    if (p.color == WHITE) {
                        positional_bonus = king_middle_game[r][c];
                    } else {
                        positional_bonus = king_middle_game[7-r][c];
                    }
                }
                
                // Bonus for center control (e4, d4, e5, d5)
                if ((r == 3 || r == 4) && (c == 3 || c == 4)) {
                    if (p.type == PAWN || p.type == KNIGHT) {
                        positional_bonus += 15;
                    }
                }
                
                int total_value = value + positional_bonus;
                score += (p.color == WHITE) ? total_value : -total_value;
            }
        }
    }
    
    // King safety - penalize if king is exposed
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (game->board[r][c].type == KING) {
                int safety_score = 0;
                ChessColor king_color = game->board[r][c].color;
                int direction = (king_color == WHITE) ? -1 : 1;
                
                // Check for pawns in front of king
                for (int dc = -1; dc <= 1; dc++) {
                    int check_r = r + direction;
                    int check_c = c + dc;
                    if (chess_is_in_bounds(check_r, check_c)) {
                        if (game->board[check_r][check_c].type == PAWN && 
                            game->board[check_r][check_c].color == king_color) {
                            safety_score += 20;
                        }
                    }
                }
                
                // Bonus for castling (king on g1/g8 or c1/c8)
                if ((king_color == WHITE && r == 7 && (c == 6 || c == 2)) ||
                    (king_color == BLACK && r == 0 && (c == 6 || c == 2))) {
                    safety_score += 30;
                }
                
                score += (king_color == WHITE) ? safety_score : -safety_score;
            }
        }
    }
    
    // Bishop pair bonus
    int white_bishops = 0, black_bishops = 0;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (game->board[r][c].type == BISHOP) {
                if (game->board[r][c].color == WHITE) white_bishops++;
                else black_bishops++;
            }
        }
    }
    if (white_bishops >= 2) score += 30;
    if (black_bishops >= 2) score -= 30;
    
    // Small random factor for variety
    score += (rand() % 10) - 5;
    
    return score;
}

int chess_get_all_moves(ChessGameState *game, ChessColor color, ChessMove *moves) {
    int count = 0;
    
    for (int fr = 0; fr < BOARD_SIZE; fr++) {
        for (int fc = 0; fc < BOARD_SIZE; fc++) {
            if (game->board[fr][fc].color == color) {
                for (int tr = 0; tr < BOARD_SIZE; tr++) {
                    for (int tc = 0; tc < BOARD_SIZE; tc++) {
                        if (chess_is_valid_move(game, fr, fc, tr, tc)) {
                            ChessGameState temp = *game;
                            ChessMove m = {fr, fc, tr, tc, 0};
                            chess_make_move(&temp, m);
                            
                            if (!chess_is_in_check(&temp, color)) {
                                moves[count++] = m;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

int chess_minimax(ChessGameState *game, int depth, int alpha, int beta, bool maximizing) {
    ChessMove moves[256];
    int move_count = chess_get_all_moves(game, game->turn, moves);
    
    if (move_count == 0) {
        if (chess_is_in_check(game, game->turn)) {
            return maximizing ? (-1000000 + depth) : (1000000 - depth);
        }
        return 0; // Stalemate
    }
    
    if (depth == 0) {
        return chess_evaluate_position(game);
    }
    
    if (maximizing) {
        int max_eval = INT_MIN;
        for (int i = 0; i < move_count; i++) {
            ChessGameState temp = *game;
            chess_make_move(&temp, moves[i]);
            int eval = chess_minimax(&temp, depth - 1, alpha, beta, false);
            max_eval = (eval > max_eval) ? eval : max_eval;
            alpha = (alpha > eval) ? alpha : eval;
            if (beta <= alpha) break;
        }
        return max_eval;
    } else {
        int min_eval = INT_MAX;
        for (int i = 0; i < move_count; i++) {
            ChessGameState temp = *game;
            chess_make_move(&temp, moves[i]);
            int eval = chess_minimax(&temp, depth - 1, alpha, beta, true);
            min_eval = (eval < min_eval) ? eval : min_eval;
            beta = (beta < eval) ? beta : eval;
            if (beta <= alpha) break;
        }
        return min_eval;
    }
}

// ============================================================================
// THINKING STATE MANAGEMENT
// ============================================================================

void* chess_think_continuously(void* arg) {
    ChessThinkingState *ts = (ChessThinkingState*)arg;
    
    while (true) {
        pthread_mutex_lock(&ts->lock);
        if (!ts->thinking) {
            pthread_mutex_unlock(&ts->lock);
            usleep(10000);
            continue;
        }
        
        ChessGameState game_copy = ts->game;
        pthread_mutex_unlock(&ts->lock);
        
        ChessMove moves[256];
        int move_count = chess_get_all_moves(&game_copy, game_copy.turn, moves);
        
        if (move_count == 0) {
            pthread_mutex_lock(&ts->lock);
            ts->has_move = false;
            ts->thinking = false;
            pthread_mutex_unlock(&ts->lock);
            //printf("THINK: No legal moves found!\n");
            continue;
        }
        
        //printf("THINK: Starting search for %s, %d legal moves\n",  game_copy.turn == WHITE ? "WHITE" : "BLACK", move_count);
        
        // Iterative deepening - LIMITED TO DEPTH 4
        for (int depth = 1; depth <= 4; depth++) {
            ChessMove best_moves[256];
            int best_move_count = 0;
            int best_score = (game_copy.turn == WHITE) ? INT_MIN : INT_MAX;
            
            bool depth_completed = true;
            
            for (int i = 0; i < move_count; i++) {
                pthread_mutex_lock(&ts->lock);
                bool should_stop = !ts->thinking;
                pthread_mutex_unlock(&ts->lock);
                
                if (should_stop) {
                    depth_completed = false;
                    break;
                }
                
                ChessGameState temp = game_copy;
                chess_make_move(&temp, moves[i]);
                int score = chess_minimax(&temp, depth - 1, INT_MIN, INT_MAX, 
                                         game_copy.turn == BLACK);
                
                if (game_copy.turn == WHITE) {
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
            
            // Update if we completed this depth
            pthread_mutex_lock(&ts->lock);
            if (depth_completed && ts->thinking && best_move_count > 0) {
                ts->best_move = best_moves[rand() % best_move_count];
                ts->best_score = best_score;
                ts->current_depth = depth;
                ts->has_move = true;
                
                // Debug output - print all best moves
                //printf("THINK: Depth %d complete (score=%d, %s), found %d best move(s):\n", depth, best_score, game_copy.turn == WHITE ? "WHITE" : "BLACK", best_move_count);
                for (int i = 0; i < best_move_count; i++) {
                    //printf("       %c%d->%c%d", 'a' + best_moves[i].from_col, 8 - best_moves[i].from_row, 'a' + best_moves[i].to_col, 8 - best_moves[i].to_row);
                          }
                //printf("       Selected: %c%d->%c%d\n", 'a' + ts->best_move.from_col, 8 - ts->best_move.from_row, 'a' + ts->best_move.to_col, 8 - ts->best_move.to_row);
            }
            pthread_mutex_unlock(&ts->lock);
            

        }
        
        pthread_mutex_lock(&ts->lock);
        ts->thinking = false;
        //printf("THINK: Finished thinking, final depth=%d, has_move=%d\n",  ts->current_depth, ts->has_move);
        pthread_mutex_unlock(&ts->lock);
    }
    
    return NULL;
}

void chess_init_thinking_state(ChessThinkingState *ts) {
    ts->thinking = false;
    ts->has_move = false;
    ts->current_depth = 0;
    ts->best_score = 0;
    pthread_mutex_init(&ts->lock, NULL);
    pthread_create(&ts->thread, NULL, chess_think_continuously, ts);
}

void chess_start_thinking(ChessThinkingState *ts, ChessGameState *game) {
    pthread_mutex_lock(&ts->lock);
    ts->game = *game;
    ts->thinking = true;
    ts->has_move = false;
    ts->current_depth = 0;
    pthread_mutex_unlock(&ts->lock);
}

ChessMove chess_get_best_move_now(ChessThinkingState *ts) {
    pthread_mutex_lock(&ts->lock);
    ChessMove move = ts->best_move;
    bool has_move = ts->has_move;
    int depth = ts->current_depth;
    ts->thinking = false; // Stop thinking
    pthread_mutex_unlock(&ts->lock);
    
    if (!has_move) {
        // No move found yet - pick random legal move as fallback
        ChessMove moves[256];
        int count = chess_get_all_moves(&ts->game, ts->game.turn, moves);
        if (count > 0) {
            move = moves[rand() % count];
        }
    }
    
    // Return the best move found so far (stored in ts->best_move)
    return move;
}

void chess_stop_thinking(ChessThinkingState *ts) {
    pthread_mutex_lock(&ts->lock);
    ts->thinking = false;
    pthread_mutex_unlock(&ts->lock);
}

// ============================================================================
// GAME STATUS
// ============================================================================

ChessGameStatus chess_check_game_status(ChessGameState *game) {
    ChessMove moves[256];
    int move_count = chess_get_all_moves(game, game->turn, moves);
    
    if (move_count == 0) {
        if (chess_is_in_check(game, game->turn)) {
            return (game->turn == WHITE) ? CHESS_CHECKMATE_BLACK : CHESS_CHECKMATE_WHITE;
        }
        return CHESS_STALEMATE;
    }
    
    return CHESS_PLAYING;
}

// ============================================================================
// VISUALIZATION SYSTEM
// ============================================================================

void init_beat_chess_system(void *vis_ptr) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatChessVisualization *chess = &vis->beat_chess;
    
    // Initialize game
    chess_init_board(&chess->game);
    chess_init_thinking_state(&chess->thinking_state);
    chess->status = CHESS_PLAYING;
    
    // Start first player thinking
    chess_start_thinking(&chess->thinking_state, &chess->game);
    
    // Initialize animation positions
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            chess->piece_x[r][c] = 0;
            chess->piece_y[r][c] = 0;
            chess->target_x[r][c] = 0;
            chess->target_y[r][c] = 0;
        }
    }
    
    // Initialize state
    chess->last_from_row = -1;
    chess->last_from_col = -1;
    chess->last_to_row = -1;
    chess->last_to_col = -1;
    chess->last_move_glow = 0;
    
    chess->is_animating = false;
    chess->animation_progress = 0;
    
    strcpy(chess->status_text, "White to move");
    chess->status_flash_timer = 0;
    chess->status_flash_color[0] = 1.0;
    chess->status_flash_color[1] = 1.0;
    chess->status_flash_color[2] = 1.0;
    chess->last_eval_change = 0;
    
    // Beat detection
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        chess->beat_volume_history[i] = 0;
    }
    chess->beat_history_index = 0;
    chess->time_since_last_move = 0;
    chess->beat_threshold = 1.3;
    
    chess->move_count = 0;
    chess->eval_bar_position = 0;
    chess->eval_bar_target = 0;
    
    // Game over handling
    chess->beats_since_game_over = 0;
    chess->waiting_for_restart = false;

    chess->time_thinking = 0;
    chess->min_think_time = 0.5;        // Wait at least 0.5s before auto-playing
    chess->good_move_threshold = 150;   // Auto-play if advantage > 150 centipawns
    chess->auto_play_enabled = true;    // Enable auto-play
    
    printf("Beat chess system initialized\n");
}

bool beat_chess_detect_beat(void *vis_ptr) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatChessVisualization *chess = &vis->beat_chess;
    
    // Update history
    chess->beat_volume_history[chess->beat_history_index] = vis->volume_level;
    chess->beat_history_index = (chess->beat_history_index + 1) % BEAT_HISTORY_SIZE;
    
    // Calculate average
    double avg = 0;
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        avg += chess->beat_volume_history[i];
    }
    avg /= BEAT_HISTORY_SIZE;
    
    // Detect beat with minimum time between moves
    if (vis->volume_level > avg * chess->beat_threshold && 
        vis->volume_level > 0.05 &&
        chess->time_since_last_move > 0.2) { // Minimum 200ms between moves
        return true;
    }
    
    return false;
}

void update_beat_chess(void *vis_ptr, double dt) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatChessVisualization *chess = &vis->beat_chess;
    
    chess->time_since_last_move += dt;
    
    // Update glow effects
    if (chess->last_move_glow > 0) {
        chess->last_move_glow -= dt * 2.0;
        if (chess->last_move_glow < 0) chess->last_move_glow = 0;
    }
    
    if (chess->status_flash_timer > 0) {
        chess->status_flash_timer -= dt * 2.0;
        if (chess->status_flash_timer < 0) chess->status_flash_timer = 0;
    }
    
    // Animate piece movement
    if (chess->is_animating) {
        chess->animation_progress += dt * 3.0;
        if (chess->animation_progress >= 1.0) {
            chess->animation_progress = 1.0;
            chess->is_animating = false;
        }
    }
    
    // Smooth eval bar
    double diff = chess->eval_bar_target - chess->eval_bar_position;
    chess->eval_bar_position += diff * dt * 3.0;
    
    // Handle game over
    if (chess->status != CHESS_PLAYING) {
        if (chess->waiting_for_restart) {
            if (beat_chess_detect_beat(vis)) {
                chess->beats_since_game_over++;
                chess->time_since_last_move = 0;
                
                if (chess->beats_since_game_over >= 2) {
                    // Restart game
                    chess_init_board(&chess->game);
                    chess->status = CHESS_PLAYING;
                    chess->beats_since_game_over = 0;
                    chess->waiting_for_restart = false;
                    chess->move_count = 0;
                    chess->eval_bar_position = 0;
                    chess->eval_bar_target = 0;
                    chess->time_thinking = 0;
                    strcpy(chess->status_text, "New game! White to move");
                    chess->status_flash_color[0] = 0.0;
                    chess->status_flash_color[1] = 1.0;
                    chess->status_flash_color[2] = 1.0;
                    chess->status_flash_timer = 1.0;
                    
                    chess_start_thinking(&chess->thinking_state, &chess->game);
                }
            }
        }
        return;
    }
    
    // Track thinking time
    pthread_mutex_lock(&chess->thinking_state.lock);
    bool is_thinking = chess->thinking_state.thinking;
    bool has_move = chess->thinking_state.has_move;
    int current_depth = chess->thinking_state.current_depth;
    int best_score = chess->thinking_state.best_score;
    pthread_mutex_unlock(&chess->thinking_state.lock);
    
    // Keep incrementing time as long as we haven't made a move yet
    if (is_thinking || has_move) {
        chess->time_thinking += dt;
    }
    
    // AUTO-PLAY: Check if we should play immediately
    bool should_auto_play = false;
    if (chess->auto_play_enabled && has_move && 
        chess->time_thinking >= chess->min_think_time) {
        
        // Force move after 4 seconds regardless of depth/evaluation
        if (chess->time_thinking >= 4.0) {
            should_auto_play = true;
        }
        // Play if we've reached depth 3 or 4
        else if (current_depth >= 3) {
            should_auto_play = true;
        }
        // Or if we found a really good move (even at depth 2)
        else {
            int eval_before = chess_evaluate_position(&chess->game);
            int advantage = (chess->game.turn == WHITE) ? 
                           (best_score - eval_before) : (eval_before - best_score);
            
            if (advantage > chess->good_move_threshold && current_depth >= 2) {
                should_auto_play = true;
            }
        }
    }
    
    // Detect beat OR auto-play trigger
    bool beat_detected = beat_chess_detect_beat(vis);
    
    if (beat_detected || should_auto_play) {
        // Get current evaluation
        int eval_before = chess_evaluate_position(&chess->game);
        
        // Force move
        ChessMove forced_move = chess_get_best_move_now(&chess->thinking_state);
        
        // Validate move
        if (!chess_is_valid_move(&chess->game, 
                                 forced_move.from_row, forced_move.from_col,
                                 forced_move.to_row, forced_move.to_col)) {
            chess_start_thinking(&chess->thinking_state, &chess->game);
            chess->time_thinking = 0;
            return;
        }
        
        // Check if move leaves king in check
        ChessGameState temp_game = chess->game;
        chess_make_move(&temp_game, forced_move);
        if (chess_is_in_check(&temp_game, chess->game.turn)) {
            chess_start_thinking(&chess->thinking_state, &chess->game);
            chess->time_thinking = 0;
            return;
        }
        
        // Get depth reached
        pthread_mutex_lock(&chess->thinking_state.lock);
        int depth_reached = chess->thinking_state.current_depth;
        pthread_mutex_unlock(&chess->thinking_state.lock);
        
        // Make the move
        ChessColor moving_color = chess->game.turn;
        chess_make_move(&chess->game, forced_move);
        
        // Evaluate
        int eval_after = chess_evaluate_position(&chess->game);
        int eval_change = (moving_color == WHITE) ? 
                         (eval_after - eval_before) : (eval_before - eval_after);
        chess->last_eval_change = eval_change;
        
        // Update eval bar
        chess->eval_bar_target = fmax(-1.0, fmin(1.0, eval_after / 1000.0));
        
        // Update display
        chess->last_from_row = forced_move.from_row;
        chess->last_from_col = forced_move.from_col;
        chess->last_to_row = forced_move.to_row;
        chess->last_to_col = forced_move.to_col;
        chess->last_move_glow = 1.0;
        
        // Animation
        chess->animating_from_row = forced_move.from_row;
        chess->animating_from_col = forced_move.from_col;
        chess->animating_to_row = forced_move.to_row;
        chess->animating_to_col = forced_move.to_col;
        chess->animation_progress = 0;
        chess->is_animating = true;
        
        // Status text
        const char *piece_names[] = {"", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
        ChessPiece moved_piece = chess->game.board[forced_move.to_row][forced_move.to_col];
        
        const char *trigger = should_auto_play ? "AUTO" : "BEAT";
        
        if (eval_change < -500) {
            snprintf(chess->status_text, sizeof(chess->status_text),
                    "[%s] BLUNDER! %s %c%d->%c%d (depth %d, -%d)",
                    trigger, piece_names[moved_piece.type],
                    'a' + forced_move.from_col, 8 - forced_move.from_row,
                    'a' + forced_move.to_col, 8 - forced_move.to_row,
                    depth_reached, -eval_change);
            chess->status_flash_color[0] = 1.0;
            chess->status_flash_color[1] = 0.0;
            chess->status_flash_color[2] = 0.0;
            chess->status_flash_timer = 1.0;
        } else if (eval_change > 200) {
            snprintf(chess->status_text, sizeof(chess->status_text),
                    "[%s] Brilliant! %s %c%d->%c%d (depth %d, +%d)",
                    trigger, piece_names[moved_piece.type],
                    'a' + forced_move.from_col, 8 - forced_move.from_row,
                    'a' + forced_move.to_col, 8 - forced_move.to_row,
                    depth_reached, eval_change);
            chess->status_flash_color[0] = 0.0;
            chess->status_flash_color[1] = 1.0;
            chess->status_flash_color[2] = 0.0;
            chess->status_flash_timer = 1.0;
        } else {
            snprintf(chess->status_text, sizeof(chess->status_text),
                    "[%s] %s: %s %c%d->%c%d (depth %d)",
                    trigger, moving_color == WHITE ? "White" : "Black",
                    piece_names[moved_piece.type],
                    'a' + forced_move.from_col, 8 - forced_move.from_row,
                    'a' + forced_move.to_col, 8 - forced_move.to_row,
                    depth_reached);
        }
        
        chess->move_count++;
        chess->time_since_last_move = 0;
        chess->time_thinking = 0;
        
        // Check move limit
        if (chess->move_count >= MAX_MOVES_BEFORE_DRAW) {
            strcpy(chess->status_text, "Draw by move limit! New game in 2 beats...");
            chess->status = CHESS_STALEMATE;
            chess->waiting_for_restart = true;
            chess->beats_since_game_over = 0;
            return;
        }
        
        // Check game status
        chess->status = chess_check_game_status(&chess->game);
        
        if (chess->status != CHESS_PLAYING) {
            chess->waiting_for_restart = true;
            chess->beats_since_game_over = 0;
            
            if (chess->status == CHESS_CHECKMATE_WHITE) {
                strcpy(chess->status_text, "Checkmate! White wins! New game in 2 beats...");
                chess->status_flash_color[0] = 1.0;
                chess->status_flash_color[1] = 1.0;
                chess->status_flash_color[2] = 1.0;
                chess->status_flash_timer = 2.0;
            } else if (chess->status == CHESS_CHECKMATE_BLACK) {
                strcpy(chess->status_text, "Checkmate! Black wins! New game in 2 beats...");
                chess->status_flash_color[0] = 0.85;
                chess->status_flash_color[1] = 0.65;
                chess->status_flash_color[2] = 0.13;
                chess->status_flash_timer = 2.0;
            } else {
                strcpy(chess->status_text, "Stalemate! New game in 2 beats...");
                chess->status_flash_color[0] = 0.7;
                chess->status_flash_color[1] = 0.7;
                chess->status_flash_color[2] = 0.7;
                chess->status_flash_timer = 2.0;
            }
        } else {
            // Start thinking for next move
            chess_start_thinking(&chess->thinking_state, &chess->game);
        }
    }
}
// ============================================================================
// DRAWING FUNCTIONS
// ============================================================================

void draw_piece(cairo_t *cr, PieceType type, ChessColor color, double x, double y, double size, double dance_offset) {
    double cx = x + size / 2;
    double cy = y + size / 2;
    double s = size * 0.4;  // Scale factor
    
    // Apply dance offset (vertical bounce)
    cy += dance_offset;
    
    // Set colors
    if (color == WHITE) {
        cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    } else {
        // Gold color for black pieces
        cairo_set_source_rgb(cr, 0.85, 0.65, 0.13);
    }
    
    switch (type) {
        case PAWN:
            // Circle on small rectangle
            cairo_arc(cr, cx, cy - s * 0.15, s * 0.25, 0, 2 * M_PI);
            cairo_fill(cr);
            cairo_rectangle(cr, cx - s * 0.2, cy + s * 0.1, s * 0.4, s * 0.3);
            cairo_fill(cr);
            break;
            
        case KNIGHT:
            // Crude horse head - blocky and angular
            // Base/neck
            cairo_rectangle(cr, cx - s * 0.15, cy, s * 0.3, s * 0.4);
            cairo_fill(cr);
            // Head (off-center rectangle)
            cairo_rectangle(cr, cx - s * 0.1, cy - s * 0.4, s * 0.35, s * 0.4);
            cairo_fill(cr);
            // Snout (small rectangle sticking out)
            cairo_rectangle(cr, cx + s * 0.15, cy - s * 0.25, s * 0.2, s * 0.15);
            cairo_fill(cr);
            // Ear (triangle on top)
            cairo_move_to(cr, cx + s * 0.05, cy - s * 0.4);
            cairo_line_to(cr, cx + s * 0.15, cy - s * 0.55);
            cairo_line_to(cr, cx + s * 0.2, cy - s * 0.35);
            cairo_fill(cr);
            break;
            
        case BISHOP:
            // Triangle with circle on top
            cairo_move_to(cr, cx, cy - s * 0.5);
            cairo_line_to(cr, cx - s * 0.25, cy + s * 0.4);
            cairo_line_to(cr, cx + s * 0.25, cy + s * 0.4);
            cairo_close_path(cr);
            cairo_fill(cr);
            cairo_arc(cr, cx, cy - s * 0.5, s * 0.12, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
            
        case ROOK:
            // Castle tower
            cairo_rectangle(cr, cx - s * 0.3, cy - s * 0.1, s * 0.6, s * 0.5);
            cairo_fill(cr);
            // Crenellations
            cairo_rectangle(cr, cx - s * 0.3, cy - s * 0.5, s * 0.15, s * 0.35);
            cairo_fill(cr);
            cairo_rectangle(cr, cx - s * 0.05, cy - s * 0.5, s * 0.1, s * 0.35);
            cairo_fill(cr);
            cairo_rectangle(cr, cx + s * 0.15, cy - s * 0.5, s * 0.15, s * 0.35);
            cairo_fill(cr);
            break;
            
        case QUEEN:
            // Crown with multiple points
            cairo_move_to(cr, cx, cy - s * 0.5);
            cairo_line_to(cr, cx - s * 0.15, cy - s * 0.2);
            cairo_line_to(cr, cx - s * 0.3, cy - s * 0.4);
            cairo_line_to(cr, cx - s * 0.3, cy + s * 0.4);
            cairo_line_to(cr, cx + s * 0.3, cy + s * 0.4);
            cairo_line_to(cr, cx + s * 0.3, cy - s * 0.4);
            cairo_line_to(cr, cx + s * 0.15, cy - s * 0.2);
            cairo_close_path(cr);
            cairo_fill(cr);
            // Center ball
            cairo_arc(cr, cx, cy - s * 0.5, s * 0.1, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
            
        case KING:
            // Crown with cross
            cairo_rectangle(cr, cx - s * 0.3, cy - s * 0.1, s * 0.6, s * 0.5);
            cairo_fill(cr);
            // Cross on top
            cairo_rectangle(cr, cx - s * 0.05, cy - s * 0.6, s * 0.1, s * 0.5);
            cairo_fill(cr);
            cairo_rectangle(cr, cx - s * 0.25, cy - s * 0.45, s * 0.5, s * 0.1);
            cairo_fill(cr);
            break;
            
        default:
            break;
    }
    
    // Outline for all pieces
    if (type != EMPTY) {
        if (color == WHITE) {
            cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        } else {
            // Darker gold outline for gold pieces
            cairo_set_source_rgb(cr, 0.5, 0.35, 0.05);
        }
        cairo_set_line_width(cr, 1.5);
        
        switch (type) {
            case PAWN:
                cairo_arc(cr, cx, cy - s * 0.15, s * 0.25, 0, 2 * M_PI);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx - s * 0.2, cy + s * 0.1, s * 0.4, s * 0.3);
                cairo_stroke(cr);
                break;
            case KNIGHT:
                cairo_rectangle(cr, cx - s * 0.15, cy, s * 0.3, s * 0.4);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx - s * 0.1, cy - s * 0.4, s * 0.35, s * 0.4);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx + s * 0.15, cy - s * 0.25, s * 0.2, s * 0.15);
                cairo_stroke(cr);
                cairo_move_to(cr, cx + s * 0.05, cy - s * 0.4);
                cairo_line_to(cr, cx + s * 0.15, cy - s * 0.55);
                cairo_line_to(cr, cx + s * 0.2, cy - s * 0.35);
                cairo_stroke(cr);
                break;
            case BISHOP:
                cairo_move_to(cr, cx, cy - s * 0.5);
                cairo_line_to(cr, cx - s * 0.25, cy + s * 0.4);
                cairo_line_to(cr, cx + s * 0.25, cy + s * 0.4);
                cairo_close_path(cr);
                cairo_stroke(cr);
                cairo_arc(cr, cx, cy - s * 0.5, s * 0.12, 0, 2 * M_PI);
                cairo_stroke(cr);
                break;
            case ROOK:
                cairo_rectangle(cr, cx - s * 0.3, cy - s * 0.1, s * 0.6, s * 0.5);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx - s * 0.3, cy - s * 0.5, s * 0.15, s * 0.35);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx - s * 0.05, cy - s * 0.5, s * 0.1, s * 0.35);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx + s * 0.15, cy - s * 0.5, s * 0.15, s * 0.35);
                cairo_stroke(cr);
                break;
            case QUEEN:
                cairo_move_to(cr, cx, cy - s * 0.5);
                cairo_line_to(cr, cx - s * 0.15, cy - s * 0.2);
                cairo_line_to(cr, cx - s * 0.3, cy - s * 0.4);
                cairo_line_to(cr, cx - s * 0.3, cy + s * 0.4);
                cairo_line_to(cr, cx + s * 0.3, cy + s * 0.4);
                cairo_line_to(cr, cx + s * 0.3, cy - s * 0.4);
                cairo_line_to(cr, cx + s * 0.15, cy - s * 0.2);
                cairo_close_path(cr);
                cairo_stroke(cr);
                cairo_arc(cr, cx, cy - s * 0.5, s * 0.1, 0, 2 * M_PI);
                cairo_stroke(cr);
                break;
            case KING:
                cairo_rectangle(cr, cx - s * 0.3, cy - s * 0.1, s * 0.6, s * 0.5);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx - s * 0.05, cy - s * 0.6, s * 0.1, s * 0.5);
                cairo_stroke(cr);
                cairo_rectangle(cr, cx - s * 0.25, cy - s * 0.45, s * 0.5, s * 0.1);
                cairo_stroke(cr);
                break;
            default:
                break;
        }
    }
}
void draw_chess_board(BeatChessVisualization *chess, cairo_t *cr) {
    double cell = chess->cell_size;
    double ox = chess->board_offset_x;
    double oy = chess->board_offset_y;
    
    // Draw board squares
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            bool is_light = (r + c) % 2 == 0;
            
            if (is_light) {
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.85);
            } else {
                cairo_set_source_rgb(cr, 0.4, 0.5, 0.4);
            }
            
            cairo_rectangle(cr, ox + c * cell, oy + r * cell, cell, cell);
            cairo_fill(cr);
        }
    }
    
    // Draw coordinates
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, cell * 0.2);
    
    for (int i = 0; i < 8; i++) {
        char label[2];
        
        // Files (a-h)
        label[0] = 'a' + i;
        label[1] = '\0';
        cairo_move_to(cr, ox + i * cell + cell * 0.05, oy + 8 * cell - cell * 0.05);
        cairo_show_text(cr, label);
        
        // Ranks (1-8)
        label[0] = '8' - i;
        cairo_move_to(cr, ox + cell * 0.05, oy + i * cell + cell * 0.25);
        cairo_show_text(cr, label);
    }
}

void draw_chess_last_move_highlight(BeatChessVisualization *chess, cairo_t *cr) {
    if (chess->last_from_row < 0 || chess->last_move_glow <= 0) return;
    
    double cell = chess->cell_size;
    double ox = chess->board_offset_x;
    double oy = chess->board_offset_y;
    
    double alpha = chess->last_move_glow * 0.5;
    
    // Highlight from square
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, alpha);
    cairo_rectangle(cr, 
                    ox + chess->last_from_col * cell, 
                    oy + chess->last_from_row * cell, 
                    cell, cell);
    cairo_fill(cr);
    
    // Highlight to square
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, alpha);
    cairo_rectangle(cr, 
                    ox + chess->last_to_col * cell, 
                    oy + chess->last_to_row * cell, 
                    cell, cell);
    cairo_fill(cr);
}

void draw_chess_pieces(BeatChessVisualization *chess, cairo_t *cr) {
    double cell = chess->cell_size;
    double ox = chess->board_offset_x;
    double oy = chess->board_offset_y;
    
    // Get volume level from the parent visualizer structure
    // We need to pass this through from draw_beat_chess
    Visualizer *vis = (Visualizer*)((char*)chess - offsetof(Visualizer, beat_chess));
    double volume = vis->volume_level;
    
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            ChessPiece piece = chess->game.board[r][c];
            
            // Skip if animating this piece
            if (chess->is_animating && 
                r == chess->animating_from_row && 
                c == chess->animating_from_col) {
                continue;
            }
            
            if (piece.type != EMPTY) {
                double x = ox + c * cell;
                double y = oy + r * cell;
                
                // Calculate dance offset based on music volume and position
                double phase = (r * 0.5 + c * 0.3) * 3.14159;  // Different phase for each square
                double time_wave = sin(chess->time_since_last_move * 10.0 + phase);
                // Scale dance by volume - pieces bounce more with louder music
                double dance_amount = time_wave * volume * cell * 0.2;
                
                // Draw shadow - dark for all pieces
                cairo_save(cr);
                cairo_translate(cr, 3, 3);
                cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
                draw_piece(cr, piece.type, piece.color, x, y, cell, dance_amount);
                cairo_restore(cr);
                
                // Draw piece
                draw_piece(cr, piece.type, piece.color, x, y, cell, dance_amount);
            }
        }
    }
    
    // Draw animating piece
    if (chess->is_animating) {
        int fr = chess->animating_from_row;
        int fc = chess->animating_from_col;
        int tr = chess->animating_to_row;
        int tc = chess->animating_to_col;
        
        ChessPiece piece = chess->game.board[tr][tc];
        
        // Smooth interpolation
        double t = chess->animation_progress;
        t = t * t * (3.0 - 2.0 * t); // Smoothstep
        
        double x = ox + (fc + t * (tc - fc)) * cell;
        double y = oy + (fr + t * (tr - fr)) * cell;
        
        // Animating piece dances even more to the music
        double dance_amount = sin(chess->time_since_last_move * 15.0) * volume * cell * 0.3;
        
        // Draw shadow - dark for all pieces
        cairo_save(cr);
        cairo_translate(cr, 3, 3);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
        draw_piece(cr, piece.type, piece.color, x, y, cell, dance_amount);
        cairo_restore(cr);
        
        // Draw piece with slight glow
        cairo_save(cr);
        if (piece.color == WHITE) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 0.9);
        } else {
            // Brighter gold glow for animating gold pieces
            cairo_set_source_rgb(cr, 0.95, 0.75, 0.2);
        }
        draw_piece(cr, piece.type, piece.color, x, y, cell, dance_amount);
        cairo_restore(cr);
    }
}

void draw_chess_eval_bar(BeatChessVisualization *chess, cairo_t *cr, int width, int height) {
    double bar_width = 30;
    double bar_height = chess->cell_size * 8;
    double bar_x = chess->board_offset_x + chess->cell_size * 8 + 20;
    double bar_y = chess->board_offset_y;
    
    // Background
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Center line
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, bar_x, bar_y + bar_height / 2);
    cairo_line_to(cr, bar_x + bar_width, bar_y + bar_height / 2);
    cairo_stroke(cr);
    
    // Evaluation position (-1 to 1)
    double eval_pos = chess->eval_bar_position;
    double fill_y = bar_y + bar_height / 2 - (eval_pos * bar_height / 2);
    double fill_height = eval_pos * bar_height / 2;
    
    if (eval_pos > 0) {
        // White advantage
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
        cairo_rectangle(cr, bar_x, fill_y, bar_width, fill_height);
    } else {
        // Black advantage
        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_rectangle(cr, bar_x, bar_y + bar_height / 2, bar_width, -fill_height);
    }
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_line_width(cr, 2);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
}

void draw_chess_status(BeatChessVisualization *chess, cairo_t *cr, int width, int height) {
    // Status text
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, chess->status_text, &extents);
    
    double text_x = (width - extents.width) / 2;
    double text_y = chess->board_offset_y - 20;
    
    // Flash background if timer active
    if (chess->status_flash_timer > 0) {
        double alpha = chess->status_flash_timer * 0.3;
        cairo_set_source_rgba(cr, 
                            chess->status_flash_color[0],
                            chess->status_flash_color[1],
                            chess->status_flash_color[2],
                            alpha);
        cairo_rectangle(cr, text_x - 10, text_y - extents.height - 5, 
                       extents.width + 20, extents.height + 10);
        cairo_fill(cr);
    }
    
    // Text
    if (chess->status_flash_timer > 0) {
        cairo_set_source_rgb(cr,
                           chess->status_flash_color[0],
                           chess->status_flash_color[1],
                           chess->status_flash_color[2]);
    } else {
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    }
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, chess->status_text);
    
    // Move counter
    char move_text[64];
    snprintf(move_text, sizeof(move_text), "Move: %d", chess->move_count);
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_font_size(cr, 14);
    cairo_text_extents(cr, move_text, &extents);
    cairo_move_to(cr, (width - extents.width) / 2, 
                  chess->board_offset_y + chess->cell_size * 8 + 30);
    cairo_show_text(cr, move_text);
}

void draw_beat_chess(void *vis_ptr, cairo_t *cr) {
    Visualizer *vis = (Visualizer*)vis_ptr;
    BeatChessVisualization *chess = &vis->beat_chess;
    
    // Calculate board layout
    int width = vis->width;
    int height = vis->height;
    
    double available_width = width * 0.8;
    double available_height = height * 0.8;
    
    chess->cell_size = fmin(available_width / 8, available_height / 8);
    chess->board_offset_x = (width - chess->cell_size * 8) / 2;
    chess->board_offset_y = (height - chess->cell_size * 8) / 2;
    
    // Draw components
    draw_chess_board(chess, cr);
    draw_chess_last_move_highlight(chess, cr);
    draw_chess_pieces(chess, cr);
    draw_chess_eval_bar(chess, cr, width, height);
    draw_chess_status(chess, cr, width, height);
}
