#include "audio_player.h"

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
