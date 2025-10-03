#include "audio_player.h"

// Global variable to track drag source row
int drag_source_index = -1;

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
    
    // Only handle right-click
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        // Get the tree path at the click position
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), 
                                         (gint)event->x, (gint)event->y, 
                                         &path, NULL, NULL, NULL)) {
            // Select the row
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
            
            // Create context menu
            GtkWidget *menu = gtk_menu_new();
            
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



// Drag begin callback
void on_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Get the row being dragged
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW));
    if (row) {
        drag_source_index = gtk_list_box_row_get_index(row);
        printf("Drag begin: source index %d\n", drag_source_index);
        
        // Set drag icon to show which item is being dragged
        char *basename = g_path_get_basename(player->queue.files[drag_source_index]);
        GtkWidget *drag_icon = gtk_label_new(basename);
        gtk_widget_show(drag_icon);
        gtk_drag_set_icon_widget(context, drag_icon, 0, 0);
        g_free(basename);
    }
}

// Drag data get callback
void on_drag_data_get(GtkWidget *widget, GdkDragContext *context,
                           GtkSelectionData *selection_data, guint target_type,
                           guint time, gpointer user_data) {
    (void)widget;
    (void)context;
    (void)time;
    (void)user_data;
    
    if (target_type == TARGET_STRING && drag_source_index >= 0) {
        char index_str[16];
        snprintf(index_str, sizeof(index_str), "%d", drag_source_index);
        gtk_selection_data_set_text(selection_data, index_str, -1);
        printf("Drag data get: sending index %d\n", drag_source_index);
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

// Drag data received callback
void on_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                gint x, gint y, GtkSelectionData *selection_data,
                                guint target_type, guint time, gpointer user_data) {
    (void)x;
    (void)y;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    if (target_type == TARGET_STRING) {
        // Get the source index from drag data
        const gchar *data = (const gchar*)gtk_selection_data_get_text(selection_data);
        if (data) {
            int source_index = atoi(data);
            
            // Get the destination index (row we're dropping on)
            GtkListBoxRow *target_row = GTK_LIST_BOX_ROW(gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW));
            if (target_row) {
                int target_index = gtk_list_box_row_get_index(target_row);
                
                printf("Drag data received: moving from %d to %d\n", source_index, target_index);
                
                // Perform the reorder
                if (reorder_queue_item(&player->queue, source_index, target_index)) {
                    // Update the display
                    update_queue_display(player);
                    update_gui_state(player);
                    printf("Queue reordered successfully\n");
                }
            }
        }
    }
    
    gtk_drag_finish(context, TRUE, FALSE, time);
    drag_source_index = -1; // Reset
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
