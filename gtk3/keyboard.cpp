#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <ctype.h>
#include "audio_player.h"

gboolean on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Check for Ctrl modifier
    gboolean ctrl_pressed = (event->state & GDK_CONTROL_MASK) != 0;
    // Check for Shift modifier  
    gboolean shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;
    
    switch (event->keyval) {
        case GDK_KEY_F9:
            toggle_vis_fullscreen(player);
            return TRUE;
        case GDK_KEY_space:
            // Space: Play/Pause toggle
            if (player->is_playing) {
                toggle_pause(player);
            } else if (player->is_loaded) {
                start_playback(player);
            }
            update_gui_state(player);
            return TRUE;
            
        case GDK_KEY_Delete:
        case GDK_KEY_d:
            // Delete or 'd': Remove current song from queue
            if (player->queue.count > 0) {
                int current_index = player->queue.current_index;
                printf("Removing current song (index %d) via keyboard\n", current_index);
                
                bool was_current_playing = (player->is_playing);
                bool queue_will_be_empty = (player->queue.count <= 1);
                
                if (remove_from_queue(&player->queue, current_index)) {
                    if (queue_will_be_empty) {
                        stop_playback(player);
                        player->is_loaded = false;
                        gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
                    } else if (was_current_playing) {
                        stop_playback(player);
                        if (load_file_from_queue(player)) {
                            update_gui_state(player);
                            start_playback(player);
                        }
                    }
                    update_queue_display(player);
                    update_gui_state(player);
                }
            }
            return TRUE;
            
        case GDK_KEY_n:
            // 'n' or Right Arrow: Next song
            if (!ctrl_pressed) {
                next_song(player);
                return TRUE;
            }
            break;
            
        case GDK_KEY_p:
            // 'p' or Left Arrow: Previous song  
            if (!ctrl_pressed) {
                previous_song(player);
                return TRUE;
            }
            break;
            
        case GDK_KEY_s:
            // 's': Stop
            if (!ctrl_pressed) {
                stop_playback(player);
                update_gui_state(player);
                return TRUE;
            }
            break;
            
        case GDK_KEY_r:
            // 'r': Toggle repeat
            if (!ctrl_pressed) {
                player->queue.repeat_queue = !player->queue.repeat_queue;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(player->repeat_queue_button), 
                                           player->queue.repeat_queue);
                printf("Queue repeat: %s\n", player->queue.repeat_queue ? "ON" : "OFF");
                return TRUE;
            }
            break;
            
        case GDK_KEY_Left:
        case GDK_KEY_comma:
        case GDK_KEY_less:
            // ',' or '<': Rewind 5 seconds
            rewind_5_seconds(player);
            return TRUE;
            
        case GDK_KEY_Right:
        case GDK_KEY_period:
        case GDK_KEY_greater:
            // '.' or '>': Fast forward 5 seconds
            fast_forward_5_seconds(player);
            return TRUE;
            
        case GDK_KEY_Home:
            // Home: Go to beginning of song
            if (player->is_loaded) {
                seek_to_position(player, 0.0);
                gtk_range_set_value(GTK_RANGE(player->progress_scale), 0.0);
            }
            return TRUE;
            
        case GDK_KEY_End:
            // End: Go to end of song (basically skip to next)
            if (player->is_loaded && player->queue.count > 1) {
                next_song(player);
            }
            return TRUE;
            
        case GDK_KEY_Up:
            // Up Arrow: Volume up
            {
                double current = gtk_range_get_value(GTK_RANGE(player->volume_scale));
                double new_vol = current + 0.1;
                if (new_vol > 5.0) new_vol = 5.0;
                gtk_range_set_value(GTK_RANGE(player->volume_scale), new_vol);
                return TRUE;
            }
            break;
            
        case GDK_KEY_Down:
            // Down Arrow: Volume down
            {
                // Shift+Down: Volume down by 10%
                double current = gtk_range_get_value(GTK_RANGE(player->volume_scale));
                double new_vol = current - 0.1;
                if (new_vol < 0.0) new_vol = 0.0;
                gtk_range_set_value(GTK_RANGE(player->volume_scale), new_vol);
                return TRUE;
            }
            break;
            
        case GDK_KEY_o:
            // Ctrl+O: Open file
            if (ctrl_pressed) {
                on_menu_open(NULL, player);
                return TRUE;
            }
            break;
            
        case GDK_KEY_a:
            // Ctrl+A: Add to queue
            if (ctrl_pressed) {
                on_add_to_queue_clicked(NULL, player);
                return TRUE;
            }
            break;
            
        case GDK_KEY_c:
            // Ctrl+C: Clear queue
            if (ctrl_pressed) {
                on_clear_queue_clicked(NULL, player);
                return TRUE;
            }
            break;
            
        case GDK_KEY_q:
            // Ctrl+Q: Quit
            if (ctrl_pressed) {
                gtk_main_quit();
                return TRUE;
            }
            break;
            
        case GDK_KEY_F1:
            // F1: Show help
            show_keyboard_help(player);
            return TRUE;
            
        case GDK_KEY_F11:
            // F11: Toggle fullscreen
            toggle_fullscreen(player);
            return TRUE;

        case GDK_KEY_0:
        case GDK_KEY_1:
        case GDK_KEY_2:
        case GDK_KEY_3:
        case GDK_KEY_4:
        case GDK_KEY_5:
        case GDK_KEY_6:
        case GDK_KEY_7:
        case GDK_KEY_8:
        case GDK_KEY_9:
            // Number keys: Jump to queue position (1-9)
            {
                int queue_pos = event->keyval - GDK_KEY_1; // 0-based index
                if (queue_pos < player->queue.count) {
                    stop_playback(player);
                    player->queue.current_index = queue_pos;
                    if (load_file_from_queue(player)) {
                        update_queue_display(player);
                        update_gui_state(player);
                        start_playback(player);
                    }
                }
            }
            return TRUE;
    }
    
    return FALSE; // Event not handled
}

void toggle_fullscreen(AudioPlayer *player) {
    GdkWindow *gdk_window = gtk_widget_get_window(player->window);
    if (!gdk_window) {
        printf("Cannot toggle fullscreen: window not realized\n");
        return;
    }
    
    GdkWindowState state = gdk_window_get_state(gdk_window);
    
    if (state & GDK_WINDOW_STATE_FULLSCREEN) {
        // Currently fullscreen, switch to windowed
        gtk_window_unfullscreen(GTK_WINDOW(player->window));
        printf("Exiting fullscreen mode\n");
    } else {
        // Currently windowed, switch to fullscreen
        gtk_window_fullscreen(GTK_WINDOW(player->window));
        printf("Entering fullscreen mode\n");
    }
}

void show_keyboard_help(AudioPlayer *player) {
    // Get screen resolution to adapt dialog size
    GdkScreen *screen = gtk_widget_get_screen(player->window);
    int screen_height = gdk_screen_get_height(screen);
    bool use_compact_dialog = (screen_height <= 700);
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Keyboard Shortcuts",
                                                    GTK_WINDOW(player->window),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Close", GTK_RESPONSE_CLOSE,
                                                    NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), use_compact_dialog ? 10 : 15);
    
    if (use_compact_dialog) {
        // Compact version for small screens - use scrolled window
        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(scrolled, 400, 400); // Fit in 800x600
        
        GtkWidget *label = gtk_label_new(
            "Shortcuts:\n\n"
            "Space - Play/Pause    S - Stop\n"
            "N - Next    P - Previous\n"
            "< - Rewind 5s    > - Forward 5s\n"
            "↑ - Volume up    ↓ - Volume down\n"
            "Home - Beginning    End - Next song\n\n"
            "D/Del - Remove current song\n"
            "R - Toggle repeat    1-9 - Jump to #\n\n"
            "Ctrl+O - Open    Ctrl+A - Add queue\n"
            "Ctrl+C - Clear    Ctrl+Q - Quit\n"
            "F1 - This help    F11 - Fullscreen\n"
            "F9 - Visualization Fullscreen"
        );
        
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), label);
        gtk_container_add(GTK_CONTAINER(content_area), scrolled);
        
    } else {
        // Full version for larger screens
        GtkWidget *label = gtk_label_new(
            "Keyboard Shortcuts:\n\n"
            "Playback Control:\n"
            "  Space\t\t- Play/Pause toggle\n"
            "  S\t\t- Stop\n"
            "  N\t\t- Next song\n"
            "  P\t\t- Previous song\n"
            "  , / < ←\t- Rewind 5 seconds\n"
            "  . / > →\t- Fast forward 5 seconds\n"
            "  Home\t- Go to beginning\n"
            "  End\t\t- Skip to next song\n\n"
            "Queue Management:\n"
            "  D / Delete\t- Remove current song from queue\n"
            "  R\t\t- Toggle repeat mode\n"
            "  1-9\t\t- Jump to queue position\n\n"
            "Volume:\n"
            "  ↑\t\t- Volume up\n"
            "  ↓\t\t- Volume down\n\n"
            "File Operations:\n"
            "  Ctrl+O\t- Open file\n"
            "  Ctrl+A\t- Add to queue\n"
            "  Ctrl+C\t- Clear queue\n"
            "  Ctrl+Q\t- Quit\n\n"
            "Display:\n"
            "  F1\t\t- Show this help\n"
            "  F11\t\t- Toggle fullscreen\n"
            "  F9\t\t- Toggle Visualization Fullscreen"
            
        );
        
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
        gtk_container_add(GTK_CONTAINER(content_area), label);
    }
    
    // Set dialog size constraints
    if (use_compact_dialog) {
        gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 450);
        gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    } else {
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    }
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void setup_keyboard_shortcuts(AudioPlayer *player) {
    // Make window focusable and able to receive key events
    gtk_widget_set_can_focus(player->window, TRUE);
    gtk_widget_grab_focus(player->window);
    
    // Connect key press event
    g_signal_connect(player->window, "key-press-event", 
                     G_CALLBACK(on_key_press_event), player);
}

// Separate callback function for the shortcuts menu
static void on_shortcuts_menu_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    show_keyboard_help((AudioPlayer*)user_data);
}

void add_keyboard_shortcuts_menu(AudioPlayer *player, GtkWidget *help_menu) {
    GtkWidget *shortcuts_item = gtk_menu_item_new_with_label("Keyboard Shortcuts");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), shortcuts_item);
    g_signal_connect(shortcuts_item, "activate", 
                     G_CALLBACK(on_shortcuts_menu_clicked), player);
}
