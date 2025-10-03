#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "beatchess.h"
#include "visualization.h"

#ifndef MAX_MOVES_BEFORE_DRAW
#define MAX_MOVES_BEFORE_DRAW 150
#endif

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *status_label;
    
    ChessGameState game;
    ChessThinkingState thinking_state;
    ChessGameStatus status;
    
    int selected_row;
    int selected_col;
    bool has_selection;
    
    int last_from_row, last_from_col;
    int last_to_row, last_to_col;
    
    double cell_size;
    double board_offset_x;
    double board_offset_y;
    
    bool player_is_white;
    bool zero_players;  // AI vs AI mode
    
    int move_count;
} ChessGUI;

// Forward declarations
void draw_piece(cairo_t *cr, PieceType type, ChessColor color, double x, double y, double size, double dance_offset);
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
void update_status_text(ChessGUI *gui);
void make_ai_move(ChessGUI *gui);
gboolean ai_move_timeout(gpointer data);


gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    ChessGUI *gui = (ChessGUI*)data;
    
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    gui->cell_size = fmin(width / 8.5, height / 8.5);
    gui->board_offset_x = (width - gui->cell_size * 8) / 2;
    gui->board_offset_y = (height - gui->cell_size * 8) / 2;
    
    // Draw background
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.25);
    cairo_paint(cr);
    
    // Draw board
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            bool is_light = (r + c) % 2 == 0;
            
            if (is_light) {
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.85);
            } else {
                cairo_set_source_rgb(cr, 0.4, 0.5, 0.4);
            }
            
            cairo_rectangle(cr, gui->board_offset_x + c * gui->cell_size,
                          gui->board_offset_y + r * gui->cell_size,
                          gui->cell_size, gui->cell_size);
            cairo_fill(cr);
        }
    }
    
    // Highlight selected square
    if (gui->has_selection) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.5);
        cairo_rectangle(cr, gui->board_offset_x + gui->selected_col * gui->cell_size,
                       gui->board_offset_y + gui->selected_row * gui->cell_size,
                       gui->cell_size, gui->cell_size);
        cairo_fill(cr);
    }
    
    // Highlight last move
    if (gui->last_from_row >= 0) {
        cairo_set_source_rgba(cr, 0.5, 0.8, 1.0, 0.3);
        cairo_rectangle(cr, gui->board_offset_x + gui->last_from_col * gui->cell_size,
                       gui->board_offset_y + gui->last_from_row * gui->cell_size,
                       gui->cell_size, gui->cell_size);
        cairo_fill(cr);
        cairo_rectangle(cr, gui->board_offset_x + gui->last_to_col * gui->cell_size,
                       gui->board_offset_y + gui->last_to_row * gui->cell_size,
                       gui->cell_size, gui->cell_size);
        cairo_fill(cr);
    }
    
    // Draw coordinates
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, gui->cell_size * 0.2);
    
    for (int i = 0; i < 8; i++) {
        char label[2];
        label[0] = 'a' + i;
        label[1] = '\0';
        cairo_move_to(cr, gui->board_offset_x + i * gui->cell_size + gui->cell_size * 0.05,
                     gui->board_offset_y + 8 * gui->cell_size - gui->cell_size * 0.05);
        cairo_show_text(cr, label);
        
        label[0] = '8' - i;
        cairo_move_to(cr, gui->board_offset_x + gui->cell_size * 0.05,
                     gui->board_offset_y + i * gui->cell_size + gui->cell_size * 0.25);
        cairo_show_text(cr, label);
    }
    
    // Draw pieces
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            ChessPiece piece = gui->game.board[r][c];
            if (piece.type != EMPTY) {
                double x = gui->board_offset_x + c * gui->cell_size;
                double y = gui->board_offset_y + r * gui->cell_size;
                
                cairo_save(cr);
                cairo_translate(cr, 2, 2);
                cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
                draw_piece(cr, piece.type, piece.color, x, y, gui->cell_size, 0);
                cairo_restore(cr);
                
                draw_piece(cr, piece.type, piece.color, x, y, gui->cell_size, 0);
            }
        }
    }
    
    return FALSE;
}

void update_status_text(ChessGUI *gui) {
    char status[256];
    
    if (gui->status == CHESS_CHECKMATE_WHITE) {
        snprintf(status, sizeof(status), "Checkmate! White wins! (Move %d)", gui->move_count);
    } else if (gui->status == CHESS_CHECKMATE_BLACK) {
        snprintf(status, sizeof(status), "Checkmate! Black wins! (Move %d)", gui->move_count);
    } else if (gui->status == CHESS_STALEMATE) {
        snprintf(status, sizeof(status), "Stalemate! Draw! (Move %d)", gui->move_count);
    } else if (gui->move_count >= MAX_MOVES_BEFORE_DRAW) {
        snprintf(status, sizeof(status), "Draw by move limit! (Move %d)", gui->move_count);
    } else {
        if (chess_is_in_check(&gui->game, gui->game.turn)) {
            snprintf(status, sizeof(status), "Move %d - %s to move (CHECK!)",
                    gui->move_count, gui->game.turn == WHITE ? "White" : "Black");
        } else {
            snprintf(status, sizeof(status), "Move %d - %s to move",
                    gui->move_count, gui->game.turn == WHITE ? "White" : "Black");
        }
    }
    
    gtk_label_set_text(GTK_LABEL(gui->status_label), status);
}

void make_ai_move(ChessGUI *gui) {
    ChessMove ai_move = chess_get_best_move_now(&gui->thinking_state);
    
    if (chess_is_valid_move(&gui->game, ai_move.from_row, ai_move.from_col,
                            ai_move.to_row, ai_move.to_col)) {
        ChessGameState temp = gui->game;
        chess_make_move(&temp, ai_move);
        
        if (!chess_is_in_check(&temp, gui->game.turn)) {
            gui->last_from_row = ai_move.from_row;
            gui->last_from_col = ai_move.from_col;
            gui->last_to_row = ai_move.to_row;
            gui->last_to_col = ai_move.to_col;
            
            chess_make_move(&gui->game, ai_move);
            gui->move_count++;
            
            gui->status = chess_check_game_status(&gui->game);
            
            if (gui->status == CHESS_PLAYING && gui->move_count < MAX_MOVES_BEFORE_DRAW) {
                chess_start_thinking(&gui->thinking_state, &gui->game);
            }
        }
    }
    
    update_status_text(gui);
    gtk_widget_queue_draw(gui->drawing_area);
}

gboolean ai_move_timeout(gpointer data) {
    ChessGUI *gui = (ChessGUI*)data;
    
    if (gui->status != CHESS_PLAYING || gui->move_count >= MAX_MOVES_BEFORE_DRAW) {
        // Game over - restart
        chess_init_board(&gui->game);
        gui->status = CHESS_PLAYING;
        gui->move_count = 0;
        gui->last_from_row = -1;
        chess_start_thinking(&gui->thinking_state, &gui->game);
        update_status_text(gui);
        gtk_widget_queue_draw(gui->drawing_area);
        return G_SOURCE_CONTINUE;
    }
    
    if (gui->zero_players) {
        // AI vs AI
        make_ai_move(gui);
        return G_SOURCE_CONTINUE;
    } else {
        // Check if it's AI's turn
        if ((gui->player_is_white && gui->game.turn == BLACK) ||
            (!gui->player_is_white && gui->game.turn == WHITE)) {
            make_ai_move(gui);
        }
    }
    
    return G_SOURCE_CONTINUE;
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    ChessGUI *gui = (ChessGUI*)data;
    
    if (gui->zero_players) return FALSE;  // No clicking in AI vs AI mode
    if (gui->status != CHESS_PLAYING) return FALSE;
    if (gui->move_count >= MAX_MOVES_BEFORE_DRAW) return FALSE;
    
    // Check if it's player's turn
    if ((gui->player_is_white && gui->game.turn != WHITE) ||
        (!gui->player_is_white && gui->game.turn != BLACK)) {
        return FALSE;
    }
    
    int col = (event->x - gui->board_offset_x) / gui->cell_size;
    int row = (event->y - gui->board_offset_y) / gui->cell_size;
    
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return FALSE;
    
    if (!gui->has_selection) {
        // First click - select piece
        ChessPiece piece = gui->game.board[row][col];
        if (piece.type != EMPTY && piece.color == gui->game.turn) {
            gui->selected_row = row;
            gui->selected_col = col;
            gui->has_selection = true;
            gtk_widget_queue_draw(widget);
        }
    } else {
        // Second click - try to move
        if (row == gui->selected_row && col == gui->selected_col) {
            // Clicked same square - deselect
            gui->has_selection = false;
        } else if (chess_is_valid_move(&gui->game, gui->selected_row, gui->selected_col, row, col)) {
            ChessGameState temp = gui->game;
            ChessMove move = {gui->selected_row, gui->selected_col, row, col, 0};
            chess_make_move(&temp, move);
            
            if (!chess_is_in_check(&temp, gui->game.turn)) {
                gui->last_from_row = gui->selected_row;
                gui->last_from_col = gui->selected_col;
                gui->last_to_row = row;
                gui->last_to_col = col;
                
                chess_make_move(&gui->game, move);
                gui->move_count++;
                gui->has_selection = false;
                
                gui->status = chess_check_game_status(&gui->game);
                
                if (gui->status == CHESS_PLAYING && gui->move_count < MAX_MOVES_BEFORE_DRAW) {
                    chess_start_thinking(&gui->thinking_state, &gui->game);
                }
                
                update_status_text(gui);
            } else {
                gui->has_selection = false;
            }
        } else {
            // Try to select new piece
            ChessPiece piece = gui->game.board[row][col];
            if (piece.type != EMPTY && piece.color == gui->game.turn) {
                gui->selected_row = row;
                gui->selected_col = col;
            } else {
                gui->has_selection = false;
            }
        }
        
        gtk_widget_queue_draw(widget);
    }
    
    return TRUE;
}

void on_new_game(GtkWidget *widget, gpointer data) {
    ChessGUI *gui = (ChessGUI*)data;
    
    chess_stop_thinking(&gui->thinking_state);
    chess_init_board(&gui->game);
    gui->status = CHESS_PLAYING;
    gui->move_count = 0;
    gui->has_selection = false;
    gui->last_from_row = -1;
    
    chess_start_thinking(&gui->thinking_state, &gui->game);
    update_status_text(gui);
    gtk_widget_queue_draw(gui->drawing_area);
}

void on_flip_board(GtkWidget *widget, gpointer data) {
    ChessGUI *gui = (ChessGUI*)data;
    gui->player_is_white = !gui->player_is_white;
    gtk_widget_queue_draw(gui->drawing_area);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    gtk_init(&argc, &argv);
    
    ChessGUI gui;
    memset(&gui, 0, sizeof(gui));
    
    gui.player_is_white = true;
    gui.zero_players = false;
    gui.last_from_row = -1;
    
    // Parse command line args
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--black") == 0 || strcmp(argv[i], "-b") == 0) {
            gui.player_is_white = false;
        }
        if (strcmp(argv[i], "--zero-players") == 0 || strcmp(argv[i], "-z") == 0) {
            gui.zero_players = true;
        }
    }
    
    // Initialize game
    chess_init_board(&gui.game);
    chess_init_thinking_state(&gui.thinking_state);
    gui.status = CHESS_PLAYING;
    gui.move_count = 0;
    
    // Start AI thinking
    chess_start_thinking(&gui.thinking_state, &gui.game);
    
    // Create window
    gui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui.window), "Chess");
    gtk_window_set_default_size(GTK_WINDOW(gui.window), 600, 650);
    g_signal_connect(gui.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Create vbox
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(gui.window), vbox);
    
    // Status label
    gui.status_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), gui.status_label, FALSE, FALSE, 5);
    update_status_text(&gui);
    
    // Drawing area
    gui.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(gui.drawing_area, 600, 600);
    gtk_widget_add_events(gui.drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(gui.drawing_area, "draw", G_CALLBACK(on_draw), &gui);
    g_signal_connect(gui.drawing_area, "button-press-event", G_CALLBACK(on_button_press), &gui);
    gtk_box_pack_start(GTK_BOX(vbox), gui.drawing_area, TRUE, TRUE, 0);
    
    // Button box
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 5);
    
    GtkWidget *new_game_button = gtk_button_new_with_label("New Game");
    g_signal_connect(new_game_button, "clicked", G_CALLBACK(on_new_game), &gui);
    gtk_box_pack_start(GTK_BOX(button_box), new_game_button, TRUE, TRUE, 0);
    
    if (!gui.zero_players) {
        GtkWidget *flip_button = gtk_button_new_with_label("Flip Board");
        g_signal_connect(flip_button, "clicked", G_CALLBACK(on_flip_board), &gui);
        gtk_box_pack_start(GTK_BOX(button_box), flip_button, TRUE, TRUE, 0);
    }
    
    gtk_widget_show_all(gui.window);
    
    // Start AI move timer (1 second delay between moves)
    g_timeout_add(1000, ai_move_timeout, &gui);
    
    gtk_main();
    
    return 0;
}
