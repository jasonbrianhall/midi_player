#include "audio_player.h"

// Global variable to track drag source row
static GtkTreeRowReference *drag_source_ref = NULL;
static int pending_delete_index = -1;
static char *pending_move_file = NULL;

void on_queue_model_row_deleted(GtkTreeModel *model, GtkTreePath *path, gpointer user_data) {
    (void)model;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    gint *indices = gtk_tree_path_get_indices(path);
    pending_delete_index = indices[0];
    
    printf("Model row deleted at index: %d\n", pending_delete_index);
    
    // Store the file path before it gets removed
    if (pending_delete_index >= 0 && pending_delete_index < player->queue.count) {
        pending_move_file = player->queue.files[pending_delete_index];
    }
}

void on_queue_model_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data) {
    (void)model;
    (void)iter;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    gint *indices = gtk_tree_path_get_indices(path);
    int insert_index = indices[0];
    
    printf("Model row inserted at index: %d (was at %d)\n", insert_index, pending_delete_index);
    
    if (pending_delete_index >= 0 && pending_move_file) {
        // Perform the actual queue reorder
        if (reorder_queue_item(&player->queue, pending_delete_index, insert_index)) {
            printf("Queue reordered: %d -> %d\n", pending_delete_index, insert_index);
        }
        
        pending_delete_index = -1;
        pending_move_file = NULL;
        
        // Update display to reflect the move
        // Note: Don't call update_queue_display here or you'll trigger infinite loop
        // Just update the play indicator
        GtkTreeIter display_iter;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(player->queue_store), &display_iter, path)) {
            const char *indicator = (insert_index == player->queue.current_index) ? "▶" : "";
            gtk_list_store_set(player->queue_store, &display_iter, COL_PLAYING, indicator, -1);
        }
    }
}


// Forward declaration of reorder function
bool reorder_queue_item(PlayQueue *queue, int from_index, int to_index) {
    if (from_index < 0 || from_index >= queue->count || 
        to_index < 0 || to_index >= queue->count || 
        from_index == to_index) {
        return false;
    }
    
    // Store the item being moved
    char *moving_item = queue->files[from_index];
    
    // Adjust current_index based on the move
    int new_current_index = queue->current_index;
    
    if (from_index == queue->current_index) {
        // Moving the currently playing item
        new_current_index = to_index;
    } else if (from_index < queue->current_index && to_index >= queue->current_index) {
        // Moving item from before current to after current
        new_current_index--;
    } else if (from_index > queue->current_index && to_index <= queue->current_index) {
        // Moving item from after current to before current
        new_current_index++;
    }
    
    // Perform the move
    if (from_index < to_index) {
        // Moving down: shift items up
        for (int i = from_index; i < to_index; i++) {
            queue->files[i] = queue->files[i + 1];
        }
    } else {
        // Moving up: shift items down
        for (int i = from_index; i > to_index; i--) {
            queue->files[i] = queue->files[i - 1];
        }
    }
    
    // Place the moved item in its new position
    queue->files[to_index] = moving_item;
    queue->current_index = new_current_index;
    
    return true;
}

void setup_queue_drag_and_drop(AudioPlayer *player) {
    GtkWidget *tree_view = player->queue_tree_view;
    
    // Enable reordering - this is the simple way for TreeView!
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree_view), TRUE);
    
    printf("Queue tree view set to reorderable\n");
}

void on_queue_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        drag_source_ref = gtk_tree_row_reference_new(model, path);
        
        gint *indices = gtk_tree_path_get_indices(path);
        int source_index = indices[0];
        
        // Create drag icon
        char *basename = g_path_get_basename(player->queue.files[source_index]);
        
        // Get the column values for a nicer drag icon
        gchar *title = NULL, *artist = NULL;
        gtk_tree_model_get(model, &iter, 
                          COL_TITLE, &title,
                          COL_ARTIST, &artist,
                          -1);
        
        char drag_text[512];
        if (title && title[0]) {
            if (artist && artist[0]) {
                snprintf(drag_text, sizeof(drag_text), "♪ %s - %s", artist, title);
            } else {
                snprintf(drag_text, sizeof(drag_text), "♪ %s", title);
            }
        } else {
            snprintf(drag_text, sizeof(drag_text), "♪ %s", basename);
        }
        
        GtkWidget *drag_icon = gtk_label_new(drag_text);
        gtk_widget_show(drag_icon);
        gtk_drag_set_icon_widget(context, drag_icon, 0, 0);
        
        g_free(basename);
        g_free(title);
        g_free(artist);
        gtk_tree_path_free(path);
        
        printf("Drag begin: source index %d\n", source_index);
    }
}

void on_queue_drag_data_get(GtkWidget *widget, GdkDragContext *context,
                            GtkSelectionData *selection_data, guint target_type,
                            guint time, gpointer user_data) {
    (void)widget;
    (void)context;
    (void)time;
    (void)user_data;
    
    if (target_type == TARGET_STRING && drag_source_ref) {
        GtkTreePath *path = gtk_tree_row_reference_get_path(drag_source_ref);
        if (path) {
            gint *indices = gtk_tree_path_get_indices(path);
            char index_str[16];
            snprintf(index_str, sizeof(index_str), "%d", indices[0]);
            gtk_selection_data_set_text(selection_data, index_str, -1);
            printf("Drag data get: sending index %d\n", indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

void on_queue_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                 gint x, gint y, GtkSelectionData *selection_data,
                                 guint target_type, guint time, gpointer user_data) {
    (void)x;
    (void)y;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    if (target_type == TARGET_STRING) {
        const gchar *data = (const gchar*)gtk_selection_data_get_text(selection_data);
        if (data) {
            int source_index = atoi(data);
            
            // Get drop position
            GtkTreePath *dest_path = NULL;
            GtkTreeViewDropPosition pos;
            
            gtk_tree_view_get_drag_dest_row(GTK_TREE_VIEW(widget), &dest_path, &pos);
            
            if (dest_path) {
                gint *indices = gtk_tree_path_get_indices(dest_path);
                int dest_index = indices[0];
                
                // Adjust destination based on drop position
                if (pos == GTK_TREE_VIEW_DROP_AFTER || 
                    pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
                    dest_index++;
                }
                
                // Adjust if dropping after source (since source will be removed first)
                if (dest_index > source_index) {
                    dest_index--;
                }
                
                printf("Drag data received: moving from %d to %d\n", source_index, dest_index);
                
                // Perform the reorder
                if (reorder_queue_item(&player->queue, source_index, dest_index)) {
                    update_queue_display(player);
                    update_gui_state(player);
                    printf("Queue reordered successfully\n");
                }
                
                gtk_tree_path_free(dest_path);
            }
        }
    }
    
    gtk_drag_finish(context, TRUE, FALSE, time);
}

void on_queue_drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data) {
    (void)widget;
    (void)context;
    (void)user_data;
    
    // Clean up the row reference
    if (drag_source_ref) {
        gtk_tree_row_reference_free(drag_source_ref);
        drag_source_ref = NULL;
    }
    
    printf("Drag end\n");
}

void on_queue_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                           GtkTreeViewColumn *column, gpointer user_data) {
    (void)tree_view;
    (void)column;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    gint *indices = gtk_tree_path_get_indices(path);
    if (!indices) return;
    
    int clicked_index = indices[0];
    
    printf("Queue row activated: index %d\n", clicked_index);
    
    if (clicked_index == player->queue.current_index && player->is_playing) {
        printf("Already playing this song\n");
        return;
    }
    
    stop_playback(player);
    player->queue.current_index = clicked_index;
    
    if (load_file_from_queue(player)) {
        update_queue_display(player);
        update_gui_state(player);
        start_playback(player);
        printf("Started playing: %s\n", get_current_queue_file(&player->queue));
    }
}

void update_queue_display(AudioPlayer *player) {
    // Clear existing model
    if (player->queue_store) {
        gtk_list_store_clear(player->queue_store);
    }
    
    // Add each queue item
    for (int i = 0; i < player->queue.count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(player->queue_store, &iter);
        
        // Extract metadata for this file
        char *metadata = extract_metadata(player->queue.files[i]);
        char title[256] = "", artist[256] = "", album[256] = "", genre[256] = "";
        int duration_seconds = 0;
        
        parse_metadata(metadata, title, artist, album, genre);
        
        // Try to get duration from metadata string
        const char *duration_patterns[] = {
            "<b>Duration:</b>",
            "<b>Length:</b>",
            "Duration:",
            "Length:"
        };
        
        for (int j = 0; j < 4; j++) {
            const char *duration_start = strstr(metadata, duration_patterns[j]);
            if (duration_start) {
                duration_start = strchr(duration_start, ':');
                if (duration_start) {
                    duration_start++;
                    const char *time_start = strchr(duration_start, ':');
                    if (time_start) {
                        const char *scan = time_start - 1;
                        while (scan > duration_start && isdigit(*scan)) {
                            scan--;
                        }
                        scan++;
                        
                        int minutes = 0, seconds = 0;
                        if (sscanf(scan, "%d:%d", &minutes, &seconds) == 2) {
                            duration_seconds = minutes * 60 + seconds;
                            break;
                        }
                    }
                }
            }
        }
        
        g_free(metadata);
        
        // Get basename for filename column
        char *basename = g_path_get_basename(player->queue.files[i]);
        
        // Format duration
        char duration_str[16];
        if (duration_seconds > 0) {
            snprintf(duration_str, sizeof(duration_str), "%d:%02d", 
                     duration_seconds / 60, duration_seconds % 60);
        } else {
            strcpy(duration_str, "");
        }
        
        const char *indicator = (i == player->queue.current_index) ? "▶" : "";
        
        gtk_list_store_set(player->queue_store, &iter,
            COL_FILEPATH, player->queue.files[i],
            COL_PLAYING, indicator,
            COL_FILENAME, basename,
            COL_TITLE, title,
            COL_ARTIST, artist,
            COL_ALBUM, album,
            COL_GENRE, genre,
            COL_DURATION, duration_str,
            -1);
        
        g_free(basename);
    }
    
    // Scroll to and select current item
    if (player->queue.current_index >= 0 && player->queue_tree_view) {
        GtkTreePath *path = gtk_tree_path_new_from_indices(
            player->queue.current_index, -1);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(player->queue_tree_view),
                                    path, NULL, TRUE, 0.5, 0.0);
        GtkTreeSelection *selection = gtk_tree_view_get_selection(
            GTK_TREE_VIEW(player->queue_tree_view));
        gtk_tree_selection_select_path(selection, path);
        gtk_tree_path_free(path);
    }
}

void on_queue_delete_item(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player->queue_tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        gint *indices = gtk_tree_path_get_indices(path);
        int index = indices[0];
        gtk_tree_path_free(path);
        
        printf("Removing item %d from queue\n", index);
        
        bool was_current_playing = (index == player->queue.current_index && player->is_playing);
        bool queue_will_be_empty = (player->queue.count <= 1);
        
        if (remove_from_queue(&player->queue, index)) {
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
                if (player->cdg_display) {
                    cdg_reset(player->cdg_display);
                    player->cdg_display->packet_count = 0;
                    player->has_cdg = false;
                }
            }
            
            update_queue_display(player);
            update_gui_state(player);
        }
    }
}

gboolean on_queue_context_menu(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Handle middle-click (button 2) - direct delete
    if (event->type == GDK_BUTTON_PRESS && event->button == 2) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), 
                                         (gint)event->x, (gint)event->y, 
                                         &path, NULL, NULL, NULL)) {
            gint *indices = gtk_tree_path_get_indices(path);
            int index = indices[0];
            gtk_tree_path_free(path);
            
            printf("Removing item %d via middle-click\n", index);
            
            bool was_current_playing = (index == player->queue.current_index && player->is_playing);
            bool queue_will_be_empty = (player->queue.count <= 1);
            
            if (remove_from_queue(&player->queue, index)) {
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
                    if (player->cdg_display) {
                        cdg_reset(player->cdg_display);
                        player->cdg_display->packet_count = 0;
                        player->has_cdg = false;
                    }
                }
                
                update_queue_display(player);
                update_gui_state(player);
            }
            
            return TRUE;
        }
    }
    
    // Handle right-click (button 3) - show context menu
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), 
                                         (gint)event->x, (gint)event->y, 
                                         &path, NULL, NULL, NULL)) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_path(selection, path);
            
            gint *indices = gtk_tree_path_get_indices(path);
            int index = indices[0];
            gtk_tree_path_free(path);
            
            GtkWidget *menu = gtk_menu_new();
            
            GtkWidget *move_up_item = gtk_menu_item_new_with_label("Move Up (Ctrl+↑)");
            g_signal_connect(move_up_item, "activate", G_CALLBACK(on_queue_move_up), player);
            gtk_widget_set_sensitive(move_up_item, index > 0);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_up_item);
            
            GtkWidget *move_down_item = gtk_menu_item_new_with_label("Move Down (Ctrl+↓)");
            g_signal_connect(move_down_item, "activate", G_CALLBACK(on_queue_move_down), player);
            gtk_widget_set_sensitive(move_down_item, index < player->queue.count - 1);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_down_item);
            
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
            
            GtkWidget *delete_item = gtk_menu_item_new_with_label("Remove from Queue");
            g_signal_connect(delete_item, "activate", G_CALLBACK(on_queue_delete_item), player);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);
            
            gtk_widget_show_all(menu);
            gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
            
            return TRUE;
        }
    }
    
    return FALSE;
}

void move_queue_item_up(AudioPlayer *player, int index) {
    if (index <= 0 || index >= player->queue.count) {
        return;
    }
    
    char *temp = player->queue.files[index];
    player->queue.files[index] = player->queue.files[index - 1];
    player->queue.files[index - 1] = temp;
    
    if (player->queue.current_index == index) {
        player->queue.current_index = index - 1;
    } else if (player->queue.current_index == index - 1) {
        player->queue.current_index = index;
    }
    
    update_queue_display(player);
    
    if (player->queue_tree_view) {
        GtkTreePath *path = gtk_tree_path_new_from_indices(index - 1, -1);
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player->queue_tree_view));
        gtk_tree_selection_select_path(selection, path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(player->queue_tree_view), path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(path);
    }
}

void move_queue_item_down(AudioPlayer *player, int index) {
    if (index < 0 || index >= player->queue.count - 1) {
        return;
    }
    
    char *temp = player->queue.files[index];
    player->queue.files[index] = player->queue.files[index + 1];
    player->queue.files[index + 1] = temp;
    
    if (player->queue.current_index == index) {
        player->queue.current_index = index + 1;
    } else if (player->queue.current_index == index + 1) {
        player->queue.current_index = index;
    }
    
    update_queue_display(player);
    
    if (player->queue_tree_view) {
        GtkTreePath *path = gtk_tree_path_new_from_indices(index + 1, -1);
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player->queue_tree_view));
        gtk_tree_selection_select_path(selection, path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(player->queue_tree_view), path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(path);
    }
}

void on_queue_move_up(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player->queue_tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        gint *indices = gtk_tree_path_get_indices(path);
        int index = indices[0];
        gtk_tree_path_free(path);
        
        move_queue_item_up(player, index);
    }
}

void on_queue_move_down(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player->queue_tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        gint *indices = gtk_tree_path_get_indices(path);
        int index = indices[0];
        gtk_tree_path_free(path);
        
        move_queue_item_down(player, index);
    }
}

gboolean on_queue_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return FALSE;
    }
    
    GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
    gint *indices = gtk_tree_path_get_indices(path);
    int index = indices[0];
    gtk_tree_path_free(path);
    
    if (event->state & GDK_CONTROL_MASK) {
        if (event->keyval == GDK_KEY_Up) {
            move_queue_item_up(player, index);
            return TRUE;
        } else if (event->keyval == GDK_KEY_Down) {
            move_queue_item_down(player, index);
            return TRUE;
        }
    }
    
    return FALSE;
}
