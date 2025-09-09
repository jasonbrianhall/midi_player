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
#include <SDL2/SDL.h>

// Add these includes for access() and F_OK
#ifdef _WIN32
    #include <io.h>
    #define F_OK 0
#else
    #include <unistd.h>
#endif

#include "visualization.h"
#include "midiplayer.h"
#include "dbopl_wrapper.h"
#include "wav_converter.h"
#include "audioconverter.h"
#include "convertoggtowav.h"
#include "convertopustowav.h"
#include "audio_player.h"
#include "vfs.h"
#include "icon.h"
#include "aiff.h"
#include "equalizer.h"


bool is_m3u_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    
    char ext_lower[10];
    strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
    ext_lower[sizeof(ext_lower) - 1] = '\0';
    for (int i = 0; ext_lower[i]; i++) {
        ext_lower[i] = tolower(ext_lower[i]);
    }
    
    return strcmp(ext_lower, ".m3u") == 0 || strcmp(ext_lower, ".m3u8") == 0;
}

bool load_m3u_playlist(AudioPlayer *player, const char *m3u_path) {
    FILE *file = fopen(m3u_path, "r");
    if (!file) {
        printf("Cannot open M3U file: %s\n", m3u_path);
        return false;
    }
    
    char line[1024];
    char m3u_dir[512];
    bool was_empty_queue = (player->queue.count == 0);
    int added_count = 0;
    
    // Get directory of M3U file for relative paths
    strncpy(m3u_dir, m3u_path, sizeof(m3u_dir) - 1);
    m3u_dir[sizeof(m3u_dir) - 1] = '\0';
    char *last_slash = strrchr(m3u_dir, '/');
    if (!last_slash) last_slash = strrchr(m3u_dir, '\\');
    if (last_slash) {
        *(last_slash + 1) = '\0';
    } else {
        strcpy(m3u_dir, "./");
    }
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = '\0';
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') continue;
        
        char full_path[1024];
        
        // Handle relative vs absolute paths
        if (line[0] == '/' || (strlen(line) > 1 && line[1] == ':')) {
            // Absolute path
            strncpy(full_path, line, sizeof(full_path) - 1);
        } else {
            // Relative path
            snprintf(full_path, sizeof(full_path), "%s%s", m3u_dir, line);
        }
        full_path[sizeof(full_path) - 1] = '\0';
        
        // Check if file exists and has supported extension
        if (access(full_path, F_OK) == 0) {
            const char *ext = strrchr(full_path, '.');
            if (ext) {
                char ext_lower[10];
                strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
                ext_lower[sizeof(ext_lower) - 1] = '\0';
                for (int i = 0; ext_lower[i]; i++) {
                    ext_lower[i] = tolower(ext_lower[i]);
                }
                
                if (strcmp(ext_lower, ".mid") == 0 || strcmp(ext_lower, ".midi") == 0 ||
                    strcmp(ext_lower, ".wav") == 0 || strcmp(ext_lower, ".mp3") == 0 ||
                    strcmp(ext_lower, ".ogg") == 0 || strcmp(ext_lower, ".flac") == 0 ||
                    strcmp(ext_lower, ".aif") == 0 || strcmp(ext_lower, ".aiff") == 0 ||
                    strcmp(ext_lower, ".opus") == 0) {
                    
                    if (add_to_queue(&player->queue, full_path)) {
                        added_count++;
                        printf("Added to queue: %s\n", full_path);
                    }
                }
            }
        } else {
            printf("File not found, skipping: %s\n", full_path);
        }
    }
    
    fclose(file);
    
    printf("M3U loaded: %d files added\n", added_count);
    
    // If queue was empty and we added files, load the first one
    if (was_empty_queue && player->queue.count > 0) {
        if (load_file_from_queue(player)) {
            update_gui_state(player);
        }
    }
    
    update_queue_display(player);
    update_gui_state(player);
    
    return added_count > 0;
}

bool save_m3u_playlist(AudioPlayer *player, const char *m3u_path) {
    if (player->queue.count == 0) {
        printf("No files in queue to save\n");
        return false;
    }
    
    FILE *file = fopen(m3u_path, "w");
    if (!file) {
        printf("Cannot create M3U file: %s\n", m3u_path);
        return false;
    }
    
    // Write M3U header
    fprintf(file, "#EXTM3U\n");
    
    // Get directory of M3U file for relative paths
    char m3u_dir[512];
    strncpy(m3u_dir, m3u_path, sizeof(m3u_dir) - 1);
    m3u_dir[sizeof(m3u_dir) - 1] = '\0';
    char *last_slash = strrchr(m3u_dir, '/');
    if (!last_slash) last_slash = strrchr(m3u_dir, '\\');
    if (last_slash) {
        *(last_slash + 1) = '\0';
    } else {
        strcpy(m3u_dir, "");
    }
    
    for (int i = 0; i < player->queue.count; i++) {
        const char *file_path = player->queue.files[i];
        
        // Try to make path relative if it's in the same directory or subdirectory
        if (strlen(m3u_dir) > 0 && strncmp(file_path, m3u_dir, strlen(m3u_dir)) == 0) {
            // File is in M3U directory or subdirectory, use relative path
            fprintf(file, "%s\n", file_path + strlen(m3u_dir));
        } else {
            // Use absolute path
            fprintf(file, "%s\n", file_path);
        }
    }
    
    fclose(file);
    printf("M3U playlist saved: %s (%d files)\n", m3u_path, player->queue.count);
    return true;
}
