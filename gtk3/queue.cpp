#include "audio_player.h"

// Global variable to track drag source row
static GtkTreeRowReference *drag_source_ref = NULL;

static gboolean on_queue_focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer user_data) {
    (void)event;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // When queue gains focus, select the currently playing item if nothing is selected
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(widget));
    if (!selected && player->queue.count > 0) {
        // Select the currently playing song
        GtkListBoxRow *current_row = gtk_list_box_get_row_at_index(
            GTK_LIST_BOX(widget), 
            player->queue.current_index
        );
        if (current_row) {
            gtk_list_box_select_row(GTK_LIST_BOX(widget), current_row);
            printf("Auto-selected current playing song (index %d) when queue gained focus\n", 
                   player->queue.current_index);
        }
    }
    
    return FALSE;
}

void setup_queue_drag_and_drop(AudioPlayer *player) {
    GtkWidget *tree_view = player->queue_tree_view;
    
    // Enable the tree view as both drag source and drag destination
    gtk_tree_view_enable_model_drag_source(
        GTK_TREE_VIEW(tree_view),
        GDK_BUTTON1_MASK,
        target_list,
        n_targets,
        GDK_ACTION_MOVE
    );
    
    gtk_tree_view_enable_model_drag_dest(
        GTK_TREE_VIEW(tree_view),
        target_list,
        n_targets,
        GDK_ACTION_MOVE
    );
    
    // Connect drag-and-drop signals
    g_signal_connect(tree_view, "drag-begin",
                     G_CALLBACK(on_queue_drag_begin), player);
    g_signal_connect(tree_view, "drag-data-get",
                     G_CALLBACK(on_queue_drag_data_get), player);
    g_signal_connect(tree_view, "drag-data-received",
                     G_CALLBACK(on_queue_drag_data_received), player);
    g_signal_connect(tree_view, "drag-end",
                     G_CALLBACK(on_queue_drag_end), player);
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
        // Look for patterns like "Duration: 3:45" or "Length: 3:45"
        const char *duration_patterns[] = {
            "<b>Duration:</b>",
            "<b>Length:</b>",
            "Duration:",
            "Length:"
        };
        
        for (int j = 0; j < 4; j++) {
            const char *duration_start = strstr(metadata, duration_patterns[j]);
            if (duration_start) {
                // Move past the label
                duration_start = strchr(duration_start, ':');
                if (duration_start) {
                    duration_start++; // Skip the colon
                    // Try to find another colon for the time format
                    const char *time_start = strchr(duration_start, ':');
                    if (time_start) {
                        // Back up to get the minutes
                        const char *scan = time_start - 1;
                        while (scan > duration_start && isdigit(*scan)) {
                            scan--;
                        }
                        scan++;
                        
                        int minutes = 0, seconds = 0;
                        if (sscanf(scan, "%d:%d", &minutes, &seconds) == 2) {
                            duration_seconds = minutes * 60 + seconds;
                            printf("Found duration for %s: %d seconds (%d:%02d)\n", 
                                   player->queue.files[i], duration_seconds, minutes, seconds);
                            break;
                        }
                    }
                }
            }
        }
        
        if (duration_seconds == 0) {
            printf("No duration found in metadata for: %s\n", player->queue.files[i]);
            printf("Metadata string: %s\n", metadata);
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
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Get the selected row
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player->queue_tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        gint *indices = gtk_tree_path_get_indices(path);
        int index = indices[0];
        gtk_tree_path_free(path);
        
        printf("Removing item %d from queue via context menu\n", index);
        
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
            
            printf("Removing item %d from queue via middle-click\n", index);
            
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
            // Select the row
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_path(selection, path);
            
            gint *indices = gtk_tree_path_get_indices(path);
            int index = indices[0];
            gtk_tree_path_free(path);
            
            // Create context menu
            GtkWidget *menu = gtk_menu_new();
            
            // Move Up
            GtkWidget *move_up_item = gtk_menu_item_new_with_label("Move Up (Ctrl+↑)");
            g_signal_connect(move_up_item, "activate", G_CALLBACK(on_queue_move_up), player);
            gtk_widget_set_sensitive(move_up_item, index > 0);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_up_item);
            
            // Move Down
            GtkWidget *move_down_item = gtk_menu_item_new_with_label("Move Down (Ctrl+↓)");
            g_signal_connect(move_down_item, "activate", G_CALLBACK(on_queue_move_down), player);
            gtk_widget_set_sensitive(move_down_item, index < player->queue.count - 1);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_down_item);
            
            // Separator
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
            
            // Delete
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

gboolean on_queue_item_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Only handle single left clicks (let drag system handle drags)
    if (event->type != GDK_BUTTON_PRESS || event->button != 1) {
        return FALSE;
    }
    
    // Get the row from the event box
    GtkWidget *row = gtk_widget_get_parent(widget);
    if (!GTK_IS_LIST_BOX_ROW(row)) {
        return FALSE;
    }
    
    // Get the index of the clicked row
    int clicked_index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row));
    
    printf("Queue item clicked: index %d\n", clicked_index);
    
    // If this is already the current song and it's playing, do nothing
    if (clicked_index == player->queue.current_index && player->is_playing) {
        printf("Already playing this song\n");
        return TRUE;
    }
    
    // Stop current playback
    stop_playback(player);
    
    // Set the queue to the clicked index
    player->queue.current_index = clicked_index;
    
    // Load and start playing the selected file
    if (load_file_from_queue(player)) {
        update_queue_display(player);
        update_gui_state(player);
        start_playback(player);
        printf("Started playing: %s\n", get_current_queue_file(&player->queue));
    }
    
    return TRUE; // Event handled
}

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

gboolean on_queue_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Only handle single left clicks (not drags, not right clicks)
    if (event->type != GDK_BUTTON_PRESS || event->button != 1) {
        return FALSE; // Let other handlers process this
    }
    
    // Get the row that was clicked
    GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(widget), (gint)event->y);
    if (!row) {
        return FALSE;
    }
    
    // Get the index of the clicked row
    int clicked_index = gtk_list_box_row_get_index(row);
    
    printf("Queue item clicked: index %d\n", clicked_index);
    
    // If this is already the current song and it's playing, do nothing
    if (clicked_index == player->queue.current_index && player->is_playing) {
        printf("Already playing this song\n");
        return TRUE;
    }
    
    // Stop current playback
    stop_playback(player);
    
    // Set the queue to the clicked index
    player->queue.current_index = clicked_index;
    
    // Load and start playing the selected file
    if (load_file_from_queue(player)) {
        update_queue_display(player);
        update_gui_state(player);
        start_playback(player);
        printf("Started playing: %s\n", get_current_queue_file(&player->queue));
    }
    
    return TRUE; // Event handled
}

void on_queue_item_clicked(GtkListBox *listbox, GtkListBoxRow *row, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    if (!row) return;
    
    // Get the index of the clicked row
    int clicked_index = gtk_list_box_row_get_index(row);
    
    printf("Queue item clicked: index %d\n", clicked_index);
    
    // If this is already the current song and it's playing, do nothing
    if (clicked_index == player->queue.current_index && player->is_playing) {
        printf("Already playing this song\n");
        return;
    }
    
    // Stop current playback
    stop_playback(player);
    
    // Set the queue to the clicked index
    player->queue.current_index = clicked_index;
    
    // Load and start playing the selected file
    if (load_file_from_queue(player)) {
        update_queue_display(player);
        update_gui_state(player);
        start_playback(player);
        printf("Started playing: %s\n", get_current_queue_file(&player->queue));
    }
}

// Drag motion callback (for visual feedback)
gboolean on_drag_motion(GtkWidget *widget, GdkDragContext *context,
                              gint x, gint y, guint time, gpointer user_data) {
    (void)widget;
    (void)x;
    (void)y;
    (void)time;
    (void)user_data;
    
    // Accept the drag
    gdk_drag_status(context, GDK_ACTION_MOVE, time);
    return TRUE;
}

gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context,
                            gint x, gint y, guint time, gpointer user_data) {
    (void)widget;
    (void)x;
    (void)y;
    (void)user_data;
    
    // Request the drag data
    GtkTargetList *list = gtk_drag_dest_get_target_list(widget);
    guint info; // Add this variable for the third parameter
    GdkAtom target = gtk_target_list_find(list, GDK_SELECTION_TYPE_STRING, &info);
    
    if (target != GDK_NONE) {
        gtk_drag_get_data(widget, context, target, time);
        return TRUE;
    }
    
    return FALSE;
}

void move_queue_item_up(AudioPlayer *player, int index) {
    if (index <= 0 || index >= player->queue.count) {
        return; // Can't move first item up or invalid index
    }
    
    // Swap with previous item
    char *temp = player->queue.files[index];
    player->queue.files[index] = player->queue.files[index - 1];
    player->queue.files[index - 1] = temp;
    
    // Update current_index if needed
    if (player->queue.current_index == index) {
        player->queue.current_index = index - 1;
    } else if (player->queue.current_index == index - 1) {
        player->queue.current_index = index;
    }
    
    update_queue_display(player);
    
    // Re-select the moved item
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
        return; // Can't move last item down or invalid index
    }
    
    // Swap with next item
    char *temp = player->queue.files[index];
    player->queue.files[index] = player->queue.files[index + 1];
    player->queue.files[index + 1] = temp;
    
    // Update current_index if needed
    if (player->queue.current_index == index) {
        player->queue.current_index = index + 1;
    } else if (player->queue.current_index == index + 1) {
        player->queue.current_index = index;
    }
    
    update_queue_display(player);
    
    // Re-select the moved item
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
    
    // Check for Ctrl+Up or Ctrl+Down
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
