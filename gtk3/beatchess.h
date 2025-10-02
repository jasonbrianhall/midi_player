#ifndef BEATCHESS_H
#define BEATCHESS_H

#include <pthread.h>
#include <stdbool.h>

#define BOARD_SIZE 8
#define MAX_CHESS_DEPTH 4
#define BEAT_HISTORY_SIZE 10

typedef enum { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;
typedef enum { NONE, WHITE, BLACK } ChessColor;

typedef struct {
    PieceType type;
    ChessColor color;
} ChessPiece;

typedef struct {
    int from_row, from_col;
    int to_row, to_col;
    int score;
} ChessMove;

typedef struct {
    ChessPiece board[BOARD_SIZE][BOARD_SIZE];
    ChessColor turn;
    bool white_king_moved, black_king_moved;
    bool white_rook_a_moved, white_rook_h_moved;
    bool black_rook_a_moved, black_rook_h_moved;
} ChessGameState;

typedef struct {
    ChessGameState game;
    ChessMove best_move;
    int best_score;
    int current_depth;
    bool has_move;
    bool thinking;
    pthread_mutex_t lock;
    pthread_t thread;
} ChessThinkingState;

typedef enum {
    CHESS_PLAYING,
    CHESS_CHECKMATE_WHITE,
    CHESS_CHECKMATE_BLACK,
    CHESS_STALEMATE
} ChessGameStatus;

typedef struct {
    // Game state
    ChessGameState game;
    ChessThinkingState thinking_state;
    ChessGameStatus status;
    
    // Animation state
    double piece_x[BOARD_SIZE][BOARD_SIZE];
    double piece_y[BOARD_SIZE][BOARD_SIZE];
    double target_x[BOARD_SIZE][BOARD_SIZE];
    double target_y[BOARD_SIZE][BOARD_SIZE];
    
    // Last move highlight
    int last_from_row, last_from_col;
    int last_to_row, last_to_col;
    double last_move_glow;
    
    // Move being animated
    int animating_from_row, animating_from_col;
    int animating_to_row, animating_to_col;
    double animation_progress;
    bool is_animating;
    
    // Status display
    char status_text[256];
    double status_flash_timer;
    double status_flash_color[3]; // RGB
    int last_eval_change;
    
    // Beat detection
    double beat_volume_history[BEAT_HISTORY_SIZE];
    int beat_history_index;
    double time_since_last_move;
    double beat_threshold;
    
    // Visual elements
    double board_offset_x, board_offset_y;
    double cell_size;
    int move_count;
    
    // Evaluation bar
    double eval_bar_position; // -1 to 1, smoothed
    double eval_bar_target;

    int beats_since_game_over;
    bool waiting_for_restart;
    
} BeatChessVisualization;

#endif // BEATCHESS_H
