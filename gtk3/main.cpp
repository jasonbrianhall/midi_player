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

extern double playTime;
extern bool isPlaying;
extern bool paused;
extern int globalVolume;
extern void processEvents(void);
extern double playwait;

AudioPlayer *player = NULL;

// Global variable to track drag source row
int drag_source_index = -1;

static GtkWidget *vis_fullscreen_window = NULL;
static bool is_vis_fullscreen = false;
static GtkWidget *original_vis_parent = NULL;
static int original_vis_width = 0;
static int original_vis_height = 0;

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>



bool open_windows_file_dialog(char* filename, size_t filename_size, bool multiple = false) {
    OPENFILENAME ofn;
    char szFile[2048] = "";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Supported\0*.mid;*.midi;*.wav;*.mp3;*.m4a;*.aiff;*.aif;*.ogg;*.flac;*.opus;*.wma\0"
                      "MIDI Files\0*.mid;*.midi\0"
                      "WAV Files\0*.wav\0"
                      "MP3 Files\0*.mp3\0"
                      "M4A Files\0*.m4a\0"
                      "OGG Files\0*.ogg\0"
                      "FLAC Files\0*.flac\0"
                      "AIFF Files\0*.aiff\0"
                      "Opus Files\0*.opus\0"
                      "WMA Files\0*.wma\0"
                      "All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (multiple) {
        ofn.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    }
    
    if (GetOpenFileName(&ofn)) {
        strncpy(filename, szFile, filename_size - 1);
        filename[filename_size - 1] = '\0';
        return true;
    }
    return false;
}
#endif

// Queue management functions
void init_queue(PlayQueue *queue) {
    queue->files = NULL;
    queue->count = 0;
    queue->capacity = 0;
    queue->current_index = -1;
    queue->repeat_queue = true;
}

void clear_queue(PlayQueue *queue) {
    for (int i = 0; i < queue->count; i++) {
        g_free(queue->files[i]);
    }
    g_free(queue->files);
    queue->files = NULL;
    queue->count = 0;
    queue->capacity = 0;
    queue->current_index = -1;
}

bool add_to_queue(PlayQueue *queue, const char *filename) {
    if (queue->count >= queue->capacity) {
        int new_capacity = queue->capacity == 0 ? 10 : queue->capacity * 2;
        char **new_files = g_realloc(queue->files, new_capacity * sizeof(char*));
        if (!new_files) return false;
        
        queue->files = new_files;
        queue->capacity = new_capacity;
    }
    
    queue->files[queue->count] = g_strdup(filename);
    queue->count++;
    
    if (queue->current_index == -1) {
        queue->current_index = 0;
    }
    
    return true;
}

const char* get_current_queue_file(PlayQueue *queue) {
    if (queue->count == 0 || queue->current_index < 0 || queue->current_index >= queue->count) {
        return NULL;
    }
    return queue->files[queue->current_index];
}

bool advance_queue(PlayQueue *queue) {
    if (queue->count == 0) {
        printf("advance_queue: Empty queue\n");
        return false;
    }
    
    if (queue->count == 1) {
        printf("advance_queue: Single song queue - %s repeat\n", 
               queue->repeat_queue ? "restarting (repeat on)" : "stopping (repeat off)");
        if (queue->repeat_queue) {
            // For single song, just stay at index 0
            queue->current_index = 0;
            return true;
        } else {
            return false;
        }
    }
    
    printf("advance_queue: Before - index %d of %d\n", queue->current_index, queue->count);
    
    queue->current_index++;
    
    if (queue->current_index >= queue->count) {
        if (queue->repeat_queue) {
            queue->current_index = 0;
            printf("advance_queue: Wrapped to beginning (repeat on)\n");
            return true;
        } else {
            queue->current_index = queue->count - 1; // Stay at last song
            printf("advance_queue: At end, no repeat\n");
            return false;
        }
    }
    
    printf("advance_queue: After - index %d of %d\n", queue->current_index, queue->count);
    return true;
}

bool previous_queue(PlayQueue *queue) {
    if (queue->count == 0) {
        printf("previous_queue: Empty queue\n");
        return false;
    }
    
    printf("previous_queue: Before - index %d of %d\n", queue->current_index, queue->count);
    
    queue->current_index--;
    
    if (queue->current_index < 0) {
        if (queue->repeat_queue) {
            queue->current_index = queue->count - 1;
            printf("previous_queue: Wrapped to end (repeat on)\n");
            return true;
        } else {
            queue->current_index = 0;
            printf("previous_queue: At beginning, no repeat\n");
            return false;
        }
    }
    
    printf("previous_queue: After - index %d of %d\n", queue->current_index, queue->count);
    return true;
}

void on_remove_from_queue_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;
    
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "queue_index"));
    AudioPlayer *player = (AudioPlayer*)g_object_get_data(G_OBJECT(button), "player");
    
    printf("Removing item %d from queue\n", index);
    
    bool was_current_playing = (index == player->queue.current_index && player->is_playing);
    bool queue_will_be_empty = (player->queue.count <= 1);
    
    if (remove_from_queue(&player->queue, index)) {
        if (queue_will_be_empty) {
            // Queue is now empty, stop playback and clear everything
            stop_playback(player);
            player->is_loaded = false;
            gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
        } else if (was_current_playing) {
            // We removed the currently playing song, load the next one
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

void update_queue_display(AudioPlayer *player) {
    // Clear existing items
    GList *children = gtk_container_get_children(GTK_CONTAINER(player->queue_listbox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Add queue items with drag and drop support
    for (int i = 0; i < player->queue.count; i++) {
        char *basename = g_path_get_basename(player->queue.files[i]);
        
        GtkWidget *row = gtk_list_box_row_new();
        
        // Create an event box to handle events properly
        GtkWidget *event_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(row), event_box);
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_container_add(GTK_CONTAINER(event_box), box);
        
        // Current playing indicator
        const char *indicator = (i == player->queue.current_index) ? "▶ " : "  ";
        GtkWidget *indicator_label = gtk_label_new(indicator);
        gtk_box_pack_start(GTK_BOX(box), indicator_label, FALSE, FALSE, 0);
        
        // Drag handle (visual indicator that item can be dragged)
        GtkWidget *drag_handle = gtk_label_new("☰");
        gtk_widget_set_tooltip_text(drag_handle, "Drag to reorder");
        gtk_box_pack_start(GTK_BOX(box), drag_handle, FALSE, FALSE, 0);

        GtkWidget *filename_label = gtk_label_new(basename);

        // Left-align the label inside the box
        gtk_widget_set_halign(filename_label, GTK_ALIGN_START);

        // Left-align the text inside the label itself
        gtk_label_set_xalign(GTK_LABEL(filename_label), 0.0);

        gtk_box_pack_start(GTK_BOX(box), filename_label, TRUE, TRUE, 0);

        // Add remove button
        GtkWidget *remove_button = gtk_button_new_with_label("✗");
        gtk_widget_set_size_request(remove_button, 30, 30);
        gtk_widget_set_tooltip_text(remove_button, "Remove from queue");
        
        // Store the index in the button data
        g_object_set_data(G_OBJECT(remove_button), "queue_index", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(remove_button), "player", player);
        
        g_signal_connect(remove_button, "clicked", G_CALLBACK(on_remove_from_queue_clicked), NULL);
        
        gtk_box_pack_end(GTK_BOX(box), remove_button, FALSE, FALSE, 0);
        
        // Set up drag source on the EVENT BOX (not the row)
        gtk_drag_source_set(event_box, 
                           GDK_BUTTON1_MASK,
                           target_list, 
                           n_targets, 
                           GDK_ACTION_MOVE);
        
        // Connect drag signals to the EVENT BOX
        g_signal_connect(event_box, "drag-begin", G_CALLBACK(on_drag_begin), player);
        g_signal_connect(event_box, "drag-data-get", G_CALLBACK(on_drag_data_get), player);
        
        // Set up drop target on the EVENT BOX
        gtk_drag_dest_set(event_box, 
                         GTK_DEST_DEFAULT_ALL,
                         target_list, 
                         n_targets, 
                         GDK_ACTION_MOVE);
        
        // Connect drop signals to the EVENT BOX
        g_signal_connect(event_box, "drag-motion", G_CALLBACK(on_drag_motion), player);
        g_signal_connect(event_box, "drag-data-received", G_CALLBACK(on_drag_data_received), player);
        g_signal_connect(event_box, "drag-drop", G_CALLBACK(on_drag_drop), player);
        
        // Handle clicks on the event box (but not on the remove button)
        g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_queue_item_button_press), player);
        
        gtk_container_add(GTK_CONTAINER(player->queue_listbox), row);
        g_free(basename);
    }
    
    gtk_widget_show_all(player->queue_listbox);
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

void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioPlayer* player = (AudioPlayer*)userdata;
    memset(stream, 0, len);
    
    if (pthread_mutex_trylock(&player->audio_mutex) != 0) return;
    
    if (!player->is_playing || player->is_paused || !player->audio_buffer.data) {
        pthread_mutex_unlock(&player->audio_mutex);
        return;
    }
    
    int16_t* output = (int16_t*)stream;
    int samples_requested = len / sizeof(int16_t);
    size_t samples_remaining = player->audio_buffer.length - player->audio_buffer.position;
    
    // Apply speed control
    double speed = player->playback_speed;
    if (speed <= 0.0) speed = 1.0; // Safety check
    
    int samples_to_process = 0;
    
    for (int i = 0; i < samples_requested && player->audio_buffer.position < player->audio_buffer.length; i++) {
        // Get current sample with volume and EQ processing
        int32_t sample = player->audio_buffer.data[player->audio_buffer.position];
        sample = (sample * globalVolume) / 100;
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;
        
        // Apply equalizer
        int16_t eq_sample = equalizer_process_sample(player->equalizer, (int16_t)sample);
        output[i] = eq_sample;
        
        samples_to_process++;
        
        // Advance position based on speed
        player->speed_accumulator += speed;
        
        // Move to next sample when accumulator >= 1.0
        while (player->speed_accumulator >= 1.0 && player->audio_buffer.position < player->audio_buffer.length) {
            player->audio_buffer.position++;
            player->speed_accumulator -= 1.0;
        }
    }
    
    // Feed processed audio to visualizer
    if (player->visualizer && samples_to_process > 0) {
        size_t sample_count = samples_to_process / player->channels;
        visualizer_update_audio_data(player->visualizer, output, sample_count, player->channels);
    }
    
    // Check if playback finished
    if (player->audio_buffer.position >= player->audio_buffer.length) {
        player->is_playing = false;
    }
    
    pthread_mutex_unlock(&player->audio_mutex);
}

bool init_audio(AudioPlayer *player, int sample_rate, int channels) {
#ifdef _WIN32
    // Try different audio drivers in order of preference
    const char* drivers[] = {"directsound", "winmm", "wasapi", NULL};
    for (int i = 0; drivers[i]; i++) {
        if (SDL_SetHint(SDL_HINT_AUDIODRIVER, drivers[i])) {
            printf("Trying SDL audio driver: %s\n", drivers[i]);
            if (SDL_Init(SDL_INIT_AUDIO) == 0) {
                printf("Successfully initialized with driver: %s\n", drivers[i]);
                break;
            } else {
                printf("Failed with driver %s: %s\n", drivers[i], SDL_GetError());
                SDL_Quit();
            }
        }
    }
#else
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }
#endif

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = sample_rate;  // Use actual file sample rate
    want.format = AUDIO_S16SYS;
    want.channels = channels;  // Use actual file channels
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = player;
    
    // Close existing audio device if open
    if (player->audio_device) {
        SDL_CloseAudioDevice(player->audio_device);
    }
    
    player->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &player->audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (player->audio_device == 0) {
        printf("Audio device open failed: %s\n", SDL_GetError());
        return false;
    }
    
    printf("Audio: %d Hz, %d channels\n", player->audio_spec.freq, player->audio_spec.channels);
    
    // Reinitialize equalizer with new sample rate if it exists
    if (player->equalizer && player->equalizer->sample_rate != sample_rate) {
        printf("Reinitializing equalizer for new sample rate: %d Hz\n", sample_rate);
        equalizer_free(player->equalizer);
        player->equalizer = equalizer_new(sample_rate);
    }
    
    return true;
}

bool convert_midi_to_wav(AudioPlayer *player, const char* filename) {
    // Check cache first
    const char* cached_file = get_cached_conversion(&player->conversion_cache, filename);
    if (cached_file) {
        strncpy(player->temp_wav_file, cached_file, sizeof(player->temp_wav_file) - 1);
        player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
        return true;
    }
    
    // Generate a unique virtual filename
    static int virtual_counter = 0;
    char virtual_filename[256];
    snprintf(virtual_filename, sizeof(virtual_filename), "virtual_midi_%d.wav", virtual_counter++);
    
    strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting MIDI to virtual WAV: %s -> %s\n", filename, virtual_filename);
    
    // Try conversion up to 2 times (initial attempt + 1 retry)
    int max_attempts = 2;
    bool conversion_successful = false;
    
    for (int attempt = 1; attempt <= max_attempts && !conversion_successful; attempt++) {
        if (attempt > 1) {
            printf("MIDI conversion attempt %d failed (duration: %.2f seconds), retrying...\n", 
                   attempt - 1, playTime);
            
            // Clean up any partial virtual file from previous attempt
            delete_virtual_file(virtual_filename);
            
            // Generate new virtual filename for retry
            snprintf(virtual_filename, sizeof(virtual_filename), "virtual_midi_%d_retry%d.wav", 
                     virtual_counter++, attempt - 1);
            strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
            player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
            
            // Longer delay between attempts for more thorough cleanup
            SDL_Delay(500);
        }
        
        printf("MIDI conversion attempt %d starting...\n", attempt);
        
        // COMPLETELY shut down SDL before conversion
        if (player->audio_device) {
            SDL_PauseAudioDevice(player->audio_device, 1);  // Pause first
            SDL_CloseAudioDevice(player->audio_device);
            player->audio_device = 0;
            printf("Closed existing SDL audio device (attempt %d)\n", attempt);
        }
        
        // Complete SDL shutdown with more thorough cleanup
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        SDL_QuitSubSystem(SDL_INIT_TIMER);
        SDL_Quit();
        printf("SDL completely shut down (attempt %d)\n", attempt);
        
        // Longer delay to ensure complete cleanup
        SDL_Delay(200);
        
        // FORCE RESET ALL GLOBAL MIDI STATE
        // This is crucial - reset all the global variables used by the MIDI player
        playTime = 0.0;
        isPlaying = false;
        paused = false;
        playwait = 0.0;
        
        // Reset MIDI file state
        extern FILE* midiFile;
        extern int TrackCount;
        extern int DeltaTicks;
        extern double Tempo;
        extern int tkPtr[MAX_TRACKS];
        extern double tkDelay[MAX_TRACKS];
        extern int tkStatus[MAX_TRACKS];
        extern bool loopStart;
        extern bool loopEnd;
        extern int loPtr[MAX_TRACKS];
        extern double loDelay[MAX_TRACKS];
        extern int loStatus[MAX_TRACKS];
        extern double loopwait;
        extern int rbPtr[MAX_TRACKS];
        extern double rbDelay[MAX_TRACKS];
        extern int rbStatus[MAX_TRACKS];
        
        // Close any existing MIDI file handle
        if (midiFile) {
            fclose(midiFile);
            midiFile = NULL;
        }
        
        // Reset all track state arrays
        TrackCount = 0;
        DeltaTicks = 0;
        Tempo = 500000;  // Default 120 BPM
        loopStart = false;
        loopEnd = false;
        loopwait = 0.0;
        
        for (int i = 0; i < MAX_TRACKS; i++) {
            tkPtr[i] = 0;
            tkDelay[i] = 0.0;
            tkStatus[i] = 0;
            loPtr[i] = 0;
            loDelay[i] = 0.0;
            loStatus[i] = 0;
            rbPtr[i] = 0;
            rbDelay[i] = 0.0;
            rbStatus[i] = 0;
        }
        
        // Reset MIDI channel state
        extern int ChPatch[16];
        extern double ChBend[16];
        extern int ChVolume[16];
        extern int ChPanning[16];
        extern int ChVibrato[16];
        
        for (int i = 0; i < 16; i++) {
            ChPatch[i] = 0;
            ChBend[i] = 0.0;
            ChVolume[i] = 127;
            ChPanning[i] = 64;
            ChVibrato[i] = 0;
        }
        
        // Reset virtual mixer globals
        extern VirtualMixer* g_midi_mixer;
        extern int g_midi_mixer_channel;
        
        if (g_midi_mixer) {
            if (g_midi_mixer_channel >= 0) {
                mixer_release_channel(g_midi_mixer, g_midi_mixer_channel);
                g_midi_mixer_channel = -1;
            }
            mixer_free(g_midi_mixer);
            g_midi_mixer = NULL;
        }
        
        printf("Reset all global MIDI state (attempt %d)\n", attempt);
        
        // Initialize SDL fresh for MIDI conversion
        if (!initSDL()) {
            printf("SDL init for conversion failed (attempt %d)\n", attempt);
            // Try to restore audio for main player
            if (!init_audio(player)) {
                printf("Failed to restore main audio after conversion failure (attempt %d)\n", attempt);
            }
            continue; // Try next attempt or exit loop
        }
        printf("SDL reinitialized for MIDI conversion (attempt %d)\n", attempt);
        
        if (!loadMidiFile(filename)) {
            printf("MIDI file load failed (attempt %d)\n", attempt);
            cleanup();
            // Try to restore audio for main player
            if (!init_audio(player)) {
                printf("Failed to restore main audio after MIDI load failure (attempt %d)\n", attempt);
            }
            continue; // Try next attempt or exit loop
        }
        printf("MIDI file loaded successfully (attempt %d)\n", attempt);
        
        VirtualWAVConverter* wav_converter = virtual_wav_converter_init(virtual_filename, SAMPLE_RATE, AUDIO_CHANNELS);
        if (!wav_converter) {
            printf("Virtual WAV converter init failed (attempt %d)\n", attempt);
            cleanup();
            // Try to restore audio for main player
            if (!init_audio(player)) {
                printf("Failed to restore main audio after converter init failure (attempt %d)\n", attempt);
            }
            continue; // Try next attempt or exit loop
        }
        printf("Virtual WAV converter initialized (attempt %d)\n", attempt);
        
        // Reset timing before starting conversion
        playTime = 0.0;
        isPlaying = true;
        playwait = 0.0;
        
        int16_t audio_buffer[AUDIO_BUFFER * AUDIO_CHANNELS];
        double buffer_duration = (double)AUDIO_BUFFER / SAMPLE_RATE;
        
        // Initial event processing
        processEvents();
        
        printf("Starting MIDI audio generation (attempt %d)...\n", attempt);
        
        // Conversion loop with timeout protection
        int conversion_timeout = 300; // 5 minutes max
        int seconds_elapsed = 0;
        
        while (isPlaying && seconds_elapsed < conversion_timeout) {
            memset(audio_buffer, 0, sizeof(audio_buffer));
            OPL_Generate(audio_buffer, AUDIO_BUFFER);
            
            if (!virtual_wav_converter_write(wav_converter, audio_buffer, AUDIO_BUFFER * AUDIO_CHANNELS)) {
                printf("Virtual WAV write failed (attempt %d)\n", attempt);
                break;
            }
            
            playTime += buffer_duration;
            playwait -= buffer_duration;
            
            while (playwait <= 0 && isPlaying) {
                processEvents();
            }
            
            // Update timeout counter
            if (((int)playTime) != seconds_elapsed) {
                seconds_elapsed = (int)playTime;
                if (seconds_elapsed % 10 == 0 && seconds_elapsed > 0) {
                    printf("Converting... %d seconds (attempt %d)\n", seconds_elapsed, attempt);
                }
            }
        }
        
        // Check for timeout
        if (seconds_elapsed >= conversion_timeout) {
            printf("MIDI conversion timed out after %d seconds (attempt %d)\n", conversion_timeout, attempt);
            playTime = 0.0; // Force failure
        }
        
        virtual_wav_converter_finish(wav_converter);
        virtual_wav_converter_free(wav_converter);
        cleanup();  // This should clean up SDL used for conversion
        
        printf("Virtual conversion complete (attempt %d): %.2f seconds\n", attempt, playTime);
        
        // Check if conversion was successful (duration > 0.1 seconds)
        if (playTime > 0.1) {
            printf("MIDI conversion successful on attempt %d\n", attempt);
            conversion_successful = true;
            
            // Add to cache after successful conversion
            add_to_conversion_cache(&player->conversion_cache, filename, virtual_filename);
        } else {
            printf("MIDI conversion failed on attempt %d (duration too short: %.2f seconds)\n", 
                   attempt, playTime);
            
            // Clean up failed conversion virtual file
            delete_virtual_file(virtual_filename);
            
            if (attempt == max_attempts) {
                printf("All MIDI conversion attempts failed, giving up\n");
            }
        }
        
        // COMPLETELY reinitialize SDL for main player
        SDL_Quit();  // Ensure clean state again
        SDL_Delay(200);  // Longer delay for thorough cleanup
        
        // Reinitialize the main SDL audio system for playback
        printf("Reinitializing SDL audio for main player (attempt %d)...\n", attempt);
        if (!init_audio(player)) {
            printf("Failed to reinitialize audio for playback (attempt %d)\n", attempt);
            if (!conversion_successful) {
                return false;
            }
        } else {
            printf("SDL audio reinitialized successfully for main player (attempt %d)\n", attempt);
        }
    }
    
    return conversion_successful;
}

bool convert_mp3_to_wav(AudioPlayer *player, const char* filename) {
    // Check cache first
    const char* cached_file = get_cached_conversion(&player->conversion_cache, filename);
    if (cached_file) {
        strncpy(player->temp_wav_file, cached_file, sizeof(player->temp_wav_file) - 1);
        player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
        return true;
    }
    player->sample_rate = 44100;
    player->channels = 2;
    // Generate a unique virtual filename
    static int virtual_counter = 0;
    char virtual_filename[256];
    snprintf(virtual_filename, sizeof(virtual_filename), "virtual_mp3_%d.wav", virtual_counter++);
    
    strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting MP3 to virtual WAV: %s -> %s\n", filename, virtual_filename);
    
    // Read MP3 file into memory
    FILE* mp3_file = fopen(filename, "rb");
    if (!mp3_file) {
        printf("Cannot open MP3 file: %s\n", filename);
        return false;
    }
    
    fseek(mp3_file, 0, SEEK_END);
    long mp3_size = ftell(mp3_file);
    fseek(mp3_file, 0, SEEK_SET);
    
    std::vector<uint8_t> mp3_data(mp3_size);
    if (fread(mp3_data.data(), 1, mp3_size, mp3_file) != (size_t)mp3_size) {
        printf("Failed to read MP3 file\n");
        fclose(mp3_file);
        return false;
    }
    fclose(mp3_file);
    
    // Convert MP3 to WAV in memory
    std::vector<uint8_t> wav_data;
    if (!convertMp3ToWavInMemory(mp3_data, wav_data)) {
        printf("MP3 to WAV conversion failed\n");
        return false;
    }
    
    // Create virtual file and write WAV data
    VirtualFile* vf = create_virtual_file(virtual_filename);
    if (!vf) {
        printf("Cannot create virtual WAV file: %s\n", virtual_filename);
        return false;
    }
    
    if (!virtual_file_write(vf, wav_data.data(), wav_data.size())) {
        printf("Failed to write virtual WAV file\n");
        return false;
    }
    
    // Add to cache after successful conversion
    add_to_conversion_cache(&player->conversion_cache, filename, virtual_filename);
    
    printf("MP3 conversion to virtual file complete\n");
    return true;
}

bool convert_flac_to_wav(AudioPlayer *player, const char* filename) {
    // Check cache first
    const char* cached_file = get_cached_conversion(&player->conversion_cache, filename);
    if (cached_file) {
        strncpy(player->temp_wav_file, cached_file, sizeof(player->temp_wav_file) - 1);
        player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
        return true;
    }
    
    // Generate a unique virtual filename
    static int virtual_counter = 0;
    char virtual_filename[256];
    snprintf(virtual_filename, sizeof(virtual_filename), "virtual_flac_%d.wav", virtual_counter++);
    
    strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting FLAC to virtual WAV: %s -> %s\n", filename, virtual_filename);
    
    // Read FLAC file into memory
    FILE* flac_file = fopen(filename, "rb");
    if (!flac_file) {
        printf("Cannot open FLAC file: %s\n", filename);
        return false;
    }
    
    fseek(flac_file, 0, SEEK_END);
    long flac_size = ftell(flac_file);
    fseek(flac_file, 0, SEEK_SET);
    
    std::vector<uint8_t> flac_data(flac_size);
    if (fread(flac_data.data(), 1, flac_size, flac_file) != (size_t)flac_size) {
        printf("Failed to read FLAC file\n");
        fclose(flac_file);
        return false;
    }
    fclose(flac_file);
    
    // Convert FLAC to WAV in memory
    std::vector<uint8_t> wav_data;
    if (!convertFlacToWavInMemory(flac_data, wav_data)) {
        printf("FLAC to WAV conversion failed\n");
        return false;
    }
    
    // Create virtual file and write WAV data
    VirtualFile* vf = create_virtual_file(virtual_filename);
    if (!vf) {
        printf("Cannot create virtual WAV file: %s\n", virtual_filename);
        return false;
    }
    
    if (!virtual_file_write(vf, wav_data.data(), wav_data.size())) {
        printf("Failed to write virtual WAV file\n");
        return false;
    }
    
    // Add to cache after successful conversion
    add_to_conversion_cache(&player->conversion_cache, filename, virtual_filename);
    
    printf("FLAC conversion to virtual file complete\n");
    return true;
}


bool load_wav_file(AudioPlayer *player, const char* wav_path) {
    FILE* wav_file = fopen(wav_path, "rb");
    if (!wav_file) {
        printf("Cannot open WAV file: %s\n", wav_path);
        return false;
    }
    
    // Read WAV header
    char header[44];
    if (fread(header, 1, 44, wav_file) != 44) {
        printf("Cannot read WAV header\n");
        fclose(wav_file);
        return false;
    }
    
    // Verify WAV format
    if (strncmp(header, "RIFF", 4) != 0 || strncmp(header + 8, "WAVE", 4) != 0) {
        printf("Invalid WAV format\n");
        fclose(wav_file);
        return false;
    }
    
    // Extract WAV info
    player->sample_rate = *(int*)(header + 24);
    player->channels = *(short*)(header + 22);
    player->bits_per_sample = *(short*)(header + 34);
    
    printf("WAV: %d Hz, %d channels, %d bits\n", player->sample_rate, player->channels, player->bits_per_sample);
    
    // Reinitialize audio with the correct sample rate and channels
    if (!init_audio(player, player->sample_rate, player->channels)) {
        printf("Failed to reinitialize audio for WAV format\n");
        fclose(wav_file);
        return false;
    }
    
    // Get file size and calculate duration
    fseek(wav_file, 0, SEEK_END);
    long file_size = ftell(wav_file);
    long data_size = file_size - 44;
    
    player->song_duration = data_size / (double)(player->sample_rate * player->channels * (player->bits_per_sample / 8));
    printf("WAV duration: %.2f seconds\n", player->song_duration);
    
    // Allocate and read audio data
    int16_t* wav_data = (int16_t*)malloc(data_size);
    if (!wav_data) {
        printf("Memory allocation failed\n");
        fclose(wav_file);
        return false;
    }
    
    fseek(wav_file, 44, SEEK_SET);
    if (fread(wav_data, 1, data_size, wav_file) != (size_t)data_size) {
        printf("WAV data read failed\n");
        free(wav_data);
        fclose(wav_file);
        return false;
    }
    
    fclose(wav_file);
    
    // Store in audio buffer
    pthread_mutex_lock(&player->audio_mutex);
    if (player->audio_buffer.data) free(player->audio_buffer.data);
    player->audio_buffer.data = wav_data;
    player->audio_buffer.length = data_size / sizeof(int16_t);
    player->audio_buffer.position = 0;
    pthread_mutex_unlock(&player->audio_mutex);
    
    printf("Loaded %zu samples\n", player->audio_buffer.length);
    return true;
}

void on_speed_changed(GtkRange *range, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    double speed = gtk_range_get_value(range);
    
    pthread_mutex_lock(&player->audio_mutex);
    player->playback_speed = speed;
    // Reset accumulator when speed changes to avoid glitches
    player->speed_accumulator = 0.0;
    pthread_mutex_unlock(&player->audio_mutex);
    
    // Update the tooltip to show current speed
    char tooltip[64];
    snprintf(tooltip, sizeof(tooltip), "Playback speed: %.2fx", speed);
    gtk_widget_set_tooltip_text(GTK_WIDGET(range), tooltip);
    
    printf("Speed changed to: %.2fx\n", speed);
}


bool load_file(AudioPlayer *player, const char *filename) {
    printf("load_file called for: %s\n", filename);
    
    // Stop current playback and clean up timer
    if (player->is_playing || player->update_timer_id > 0) {
        printf("Stopping current playback...\n");
        pthread_mutex_lock(&player->audio_mutex);
        player->is_playing = false;
        player->is_paused = false;
        SDL_PauseAudioDevice(player->audio_device, 1);
        pthread_mutex_unlock(&player->audio_mutex);
        
        if (player->update_timer_id > 0) {
            g_source_remove(player->update_timer_id);
            player->update_timer_id = 0;
            printf("Removed existing timer\n");
        }
    }
    
    // Check if this is a virtual file (starts with "virtual_")
    if (strncmp(filename, "virtual_", 8) == 0) {
        printf("Loading virtual WAV file: %s\n", filename);
        return load_virtual_wav_file(player, filename);
    }
    
    // Determine file type for regular files
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        printf("Unknown file type\n");
        return false;
    }
    
    // Convert extension to lowercase
    char ext_lower[10];
    strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
    ext_lower[sizeof(ext_lower) - 1] = '\0';
    for (int i = 0; ext_lower[i]; i++) {
        ext_lower[i] = tolower(ext_lower[i]);
    }
    
    bool success = false;
    
    if (strcmp(ext_lower, ".wav") == 0) {
        printf("Loading WAV file: %s\n", filename);
        success = load_wav_file(player, filename);
    } else if (strcmp(ext_lower, ".mid") == 0 || strcmp(ext_lower, ".midi") == 0) {
        printf("Loading MIDI file: %s\n", filename);
        if (convert_midi_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".mp3") == 0) {
        printf("Loading MP3 file: %s\n", filename);
        if (convert_mp3_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".ogg") == 0) {
        printf("Loading OGG file: %s\n", filename);
        if (convert_ogg_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".flac") == 0) {
        printf("Loading FLAC file: %s\n", filename);
        if (convert_flac_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".aif") == 0 || strcmp(ext_lower, ".aiff") == 0) {
        printf("Loading AIFF file: %s\n", filename);
        if (convert_aiff_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".opus") == 0) {
        printf("Loading Opus file: %s\n", filename);
        if (convert_opus_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".m4a") == 0) {
        printf("Loading M4A file: %s\n", filename);
        if (convert_m4a_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".wma") == 0) {
        printf("Loading WMA file: %s\n", filename);
        if (convert_wma_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }
    } else {
        printf("Trying to load unknown file: %s\n", filename);
        if (convert_audio_to_wav(player, filename)) {
            printf("Now loading converted virtual WAV file: %s\n", player->temp_wav_file);
            success = load_virtual_wav_file(player, player->temp_wav_file);
        }    
        else {
            printf("File isn't supported\n");
        }
    }
    
    if (success) {
        strcpy(player->current_file, filename);
        player->is_loaded = true;
        player->is_playing = false;
        player->is_paused = false;
        playTime = 0;
        
        // Update the progress scale range
        gtk_range_set_range(GTK_RANGE(player->progress_scale), 0.0, player->song_duration);
        gtk_range_set_value(GTK_RANGE(player->progress_scale), 0.0);
        
        // Check if the loaded file has valid audio data
        if (player->audio_buffer.length == 0 || player->song_duration <= 0.1) {
            printf("Warning: File loaded but has no/minimal audio data (duration: %.2f, samples: %zu)\n", 
                   player->song_duration, player->audio_buffer.length);
            printf("Skipping this file and advancing to next...\n");
            
            // Clean up the virtual file if it was created
            if (strncmp(player->temp_wav_file, "virtual_", 8) == 0) {
                delete_virtual_file(player->temp_wav_file);
            }
            
            update_gui_state(player);
            
            // Trigger immediate advance for invalid files
            if (player->queue.count > 1) {
                g_timeout_add(100, [](gpointer data) -> gboolean {
                    AudioPlayer *p = (AudioPlayer*)data;
                    printf("Auto-advancing from invalid file...\n");
                    if (advance_queue(&p->queue)) {
                        if (load_file_from_queue(p)) {
                            update_queue_display(p);
                            update_gui_state(p);
                        }
                    }
                    return FALSE; // Don't repeat
                }, player);
            }
            
            return true;
        }
        
        printf("File successfully loaded (duration: %.2f, samples: %zu), auto-starting playback\n", 
               player->song_duration, player->audio_buffer.length);
        
        // AUTO-START PLAYBACK for valid files
        start_playback(player);
        update_gui_state(player);
    } else {
        printf("Failed to load file: %s\n", filename);
    }
    
    return success;
}

bool load_file_from_queue(AudioPlayer *player) {
    const char *filename = get_current_queue_file(&player->queue);
    if (!filename) return false;
    
    return load_file(player, filename);
}

void seek_to_position(AudioPlayer *player, double position_seconds) {
    if (!player->is_loaded || !player->audio_buffer.data || player->song_duration <= 0) {
        return;
    }
    
    // Clamp position to valid range
    if (position_seconds < 0) position_seconds = 0;
    if (position_seconds > player->song_duration) position_seconds = player->song_duration;
    
    pthread_mutex_lock(&player->audio_mutex);
    
    // Calculate the sample position based on the time position
    double samples_per_second = (double)(player->sample_rate * player->channels);
    size_t new_position = (size_t)(position_seconds * samples_per_second);
    
    // Ensure position is within bounds
    if (new_position >= player->audio_buffer.length) {
        new_position = player->audio_buffer.length - 1;
    }
    
    player->audio_buffer.position = new_position;
    playTime = position_seconds;
    
    pthread_mutex_unlock(&player->audio_mutex);
}

void start_playback(AudioPlayer *player) {
    if (!player->is_loaded || !player->audio_buffer.data) {
        printf("Cannot start playback - no audio data loaded\n");
        return;
    }
    
    printf("Starting WAV playback\n");
    
    pthread_mutex_lock(&player->audio_mutex);
    // If we're at the end, restart from beginning
    if (player->audio_buffer.position >= player->audio_buffer.length) {
        player->audio_buffer.position = 0;
        playTime = 0;
    }
    player->is_playing = true;
    player->is_paused = false;
    pthread_mutex_unlock(&player->audio_mutex);
    
    SDL_PauseAudioDevice(player->audio_device, 0);
    
    if (player->update_timer_id == 0) {
        player->update_timer_id = g_timeout_add(100, (GSourceFunc)([](gpointer data) -> gboolean {
            AudioPlayer *p = (AudioPlayer*)data;
            
            pthread_mutex_lock(&p->audio_mutex);
            bool song_finished = false;
            bool currently_playing = p->is_playing;
            
            // Check if song has finished
            if (p->audio_buffer.data && p->audio_buffer.length > 0) {
                // Song finished if we've reached the end of the buffer
                if (p->audio_buffer.position >= p->audio_buffer.length) {
                    if (currently_playing) {
                        p->is_playing = false;
                        currently_playing = false;
                    }
                    song_finished = true;
                    printf("Song finished - reached end of buffer (pos: %zu, len: %zu)\n", 
                           p->audio_buffer.position, p->audio_buffer.length);
                }
                // Also check if is_playing was set to false by audio callback
                else if (!currently_playing && p->audio_buffer.position > 0) {
                    song_finished = true;
                    printf("Song finished - detected by audio callback\n");
                }
            }
            
            // Update playback position if playing
            if (currently_playing && p->audio_buffer.data && p->sample_rate > 0) {
                double samples_per_second = (double)(p->sample_rate * p->channels);
                playTime = (double)p->audio_buffer.position / samples_per_second;
            }
            
            pthread_mutex_unlock(&p->audio_mutex);
            
            // Handle song completion
            if (song_finished && p->queue.count > 0) {
                printf("Song completed. Calling next_song()...\n");
                
                // Stop the current timer
                p->update_timer_id = 0;
                
                // Call next_song() after a short delay
                g_timeout_add(50, [](gpointer data) -> gboolean {
                    AudioPlayer *player = (AudioPlayer*)data;
                    next_song(player);
                    return FALSE;
                }, p);
                
                return FALSE; // Stop this timer
            }
            
            // If not playing anymore (but not due to song completion), stop timer
            if (!currently_playing && !song_finished) {
                update_gui_state(p);
                p->update_timer_id = 0;
                return FALSE;
            }
            
            // Update GUI elements (only if still playing)
            if (currently_playing) {
                // Update progress scale (only if not currently seeking)
                if (!p->seeking) {
                    gtk_range_set_value(GTK_RANGE(p->progress_scale), playTime);
                }
                
                // Update time label
                int min = (int)(playTime / 60);
                int sec = (int)playTime % 60;
                int total_min = (int)(p->song_duration / 60);
                int total_sec = (int)p->song_duration % 60;
                
                char time_text[64];
                snprintf(time_text, sizeof(time_text), "%02d:%02d / %02d:%02d", min, sec, total_min, total_sec);
                gtk_label_set_text(GTK_LABEL(p->time_label), time_text);
            }
            
            return TRUE; // Continue timer
        }), player);
    }
}

bool remove_from_queue(PlayQueue *queue, int index) {
    if (index < 0 || index >= queue->count) {
        return false;
    }
    
    // Free the filename at this index
    g_free(queue->files[index]);
    
    // Shift all items after this index down by one
    for (int i = index; i < queue->count - 1; i++) {
        queue->files[i] = queue->files[i + 1];
    }
    
    queue->count--;
    
    // Adjust current_index if necessary
    if (index < queue->current_index) {
        // Removed item was before current, so decrease current index
        queue->current_index--;
    } else if (index == queue->current_index) {
        // Removed the currently playing item
        if (queue->count == 0) {
            // Queue is now empty
            queue->current_index = -1;
        } else if (queue->current_index >= queue->count) {
            // Current index is now beyond the end, wrap to beginning
            queue->current_index = 0;
        }
        // If current_index < queue->count, it stays the same (next song takes its place)
    }
    // If index > current_index, current_index stays the same
    
    return true;
}

void toggle_pause(AudioPlayer *player) {
    if (!player->is_playing) return;
    
    pthread_mutex_lock(&player->audio_mutex);
    player->is_paused = !player->is_paused;
    
    if (player->is_paused) {
        SDL_PauseAudioDevice(player->audio_device, 1);
    } else {
        SDL_PauseAudioDevice(player->audio_device, 0);
    }
    pthread_mutex_unlock(&player->audio_mutex);
    
    gtk_button_set_label(GTK_BUTTON(player->pause_button), player->is_paused ? "⏯" : "⏸");
}

gboolean on_vis_fullscreen_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    if (event->keyval == GDK_KEY_F9 || event->keyval == GDK_KEY_Escape) {
        toggle_vis_fullscreen(player);
        return TRUE;
    }
    return FALSE;
}

gboolean on_vis_fullscreen_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    toggle_vis_fullscreen(player);
    return TRUE; // Prevent actual close, just exit fullscreen
}

void toggle_vis_fullscreen(AudioPlayer *player) {
    if (!player->visualizer || !player->visualizer->drawing_area) {
        printf("No visualizer available for fullscreen mode\n");
        return;
    }
    
    if (!is_vis_fullscreen) {
        // Enter visualization fullscreen mode
        printf("Entering visualization fullscreen mode\n");
        
        // Store original parent and size
        original_vis_parent = gtk_widget_get_parent(player->visualizer->drawing_area);
        gtk_widget_get_size_request(player->visualizer->drawing_area, &original_vis_width, &original_vis_height);
        
        // Create dedicated fullscreen window for visualization only
        vis_fullscreen_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(vis_fullscreen_window), "Audio Visualizer - Press F9 to exit");
        gtk_window_fullscreen(GTK_WINDOW(vis_fullscreen_window));
        gtk_window_set_decorated(GTK_WINDOW(vis_fullscreen_window), FALSE);
        gtk_window_set_keep_above(GTK_WINDOW(vis_fullscreen_window), TRUE);
        
        // Set black background for better visualization contrast
        GdkRGBA black = {0.0, 0.0, 0.0, 1.0};
        gtk_widget_override_background_color(vis_fullscreen_window, GTK_STATE_FLAG_NORMAL, &black);
        
        // Reparent the visualization drawing area
        if (original_vis_parent) {
            g_object_ref(player->visualizer->drawing_area);
            gtk_container_remove(GTK_CONTAINER(original_vis_parent), player->visualizer->drawing_area);
        }
        
        gtk_container_add(GTK_CONTAINER(vis_fullscreen_window), player->visualizer->drawing_area);
        g_object_unref(player->visualizer->drawing_area);
        
        // Set visualization to full screen size
        GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(vis_fullscreen_window));
        int screen_width = gdk_screen_get_width(screen);
        int screen_height = gdk_screen_get_height(screen);
        gtk_widget_set_size_request(player->visualizer->drawing_area, screen_width, screen_height);
        
        // Set up key handler for F9 and Escape to exit fullscreen
        g_signal_connect(vis_fullscreen_window, "key-press-event", 
                        G_CALLBACK(on_vis_fullscreen_key_press), player);
        
        // Handle window close button
        g_signal_connect(vis_fullscreen_window, "delete-event", 
                        G_CALLBACK(on_vis_fullscreen_delete_event), player);
        
        // Show fullscreen window
        gtk_widget_show_all(vis_fullscreen_window);
        gtk_window_present(GTK_WINDOW(vis_fullscreen_window));
        
        is_vis_fullscreen = true;
        printf("Visualization fullscreen activated (F9 or Escape to exit)\n");
        
    } else {
        // Exit visualization fullscreen mode
        printf("Exiting visualization fullscreen mode\n");
        
        if (vis_fullscreen_window && player->visualizer && player->visualizer->drawing_area) {
            // Reparent visualization back to original location
            g_object_ref(player->visualizer->drawing_area);
            gtk_container_remove(GTK_CONTAINER(vis_fullscreen_window), player->visualizer->drawing_area);
            
            if (original_vis_parent) {
                gtk_container_add(GTK_CONTAINER(original_vis_parent), player->visualizer->drawing_area);
                
                // Restore original size
                gtk_widget_set_size_request(player->visualizer->drawing_area, 
                                           original_vis_width, original_vis_height);
            }
            
            g_object_unref(player->visualizer->drawing_area);
            
            // Destroy fullscreen window
            gtk_widget_destroy(vis_fullscreen_window);
            vis_fullscreen_window = NULL;
        }
        
        // Reset state
        is_vis_fullscreen = false;
        original_vis_parent = NULL;
        original_vis_width = 0;
        original_vis_height = 0;
        
        printf("Visualization returned to normal view\n");
    }
}


void cleanup_vis_fullscreen() {
    if (is_vis_fullscreen && vis_fullscreen_window) {
        // Force exit visualization fullscreen before cleanup
        if (player) {
            toggle_vis_fullscreen(player);
        } else if (vis_fullscreen_window) {
            gtk_widget_destroy(vis_fullscreen_window);
            vis_fullscreen_window = NULL;
            is_vis_fullscreen = false;
        }
    }
}

void stop_playback(AudioPlayer *player) {
    pthread_mutex_lock(&player->audio_mutex);
    player->is_playing = false;
    player->is_paused = false;
    player->audio_buffer.position = 0;
    playTime = 0;
    pthread_mutex_unlock(&player->audio_mutex);
    
    SDL_PauseAudioDevice(player->audio_device, 1);
    
    if (player->update_timer_id > 0) {
        g_source_remove(player->update_timer_id);
        player->update_timer_id = 0;
    }
    
    gtk_range_set_value(GTK_RANGE(player->progress_scale), 0.0);
    gtk_label_set_text(GTK_LABEL(player->time_label), "00:00 / 00:00");
    gtk_button_set_label(GTK_BUTTON(player->pause_button), "⏸");
}

void rewind_5_seconds(AudioPlayer *player) {
    if (!player->is_loaded) return;
    
    double current_time = playTime;
    double new_time = current_time - 5.0;
    if (new_time < 0) new_time = 0;
    
    seek_to_position(player, new_time);
    gtk_range_set_value(GTK_RANGE(player->progress_scale), new_time);
    
    printf("Rewinded 5 seconds to %.2f\n", new_time);
}

void fast_forward_5_seconds(AudioPlayer *player) {
    if (!player->is_loaded) return;
    
    double current_time = playTime;
    double new_time = current_time + 5.0;
    if (new_time > player->song_duration) new_time = player->song_duration;
    
    seek_to_position(player, new_time);
    gtk_range_set_value(GTK_RANGE(player->progress_scale), new_time);
    
    printf("Fast forwarded 5 seconds to %.2f\n", new_time);
}

void next_song(AudioPlayer *player) {
    if (player->queue.count <= 1) return;
    
    stop_playback(player);
    
    if (advance_queue(&player->queue)) {
        if (load_file_from_queue(player)) {
            update_queue_display(player);
            update_gui_state(player);
            start_playback(player);
        }
    }
}

void previous_song(AudioPlayer *player) {
    if (player->queue.count <= 1) return;
    
    stop_playback(player);
    
    if (previous_queue(&player->queue)) {
        if (load_file_from_queue(player)) {
            update_queue_display(player);
            update_gui_state(player);
            start_playback(player);
        }
    }
}

void update_gui_state(AudioPlayer *player) {
    gtk_widget_set_sensitive(player->play_button, player->is_loaded && !player->is_playing);
    gtk_widget_set_sensitive(player->pause_button, player->is_playing);
    gtk_widget_set_sensitive(player->stop_button, player->is_playing || player->is_paused);
    gtk_widget_set_sensitive(player->rewind_button, player->is_loaded);
    gtk_widget_set_sensitive(player->fast_forward_button, player->is_loaded);
    gtk_widget_set_sensitive(player->progress_scale, player->is_loaded);
    gtk_widget_set_sensitive(player->next_button, player->queue.count > 1);
    gtk_widget_set_sensitive(player->prev_button, player->queue.count > 1);
    
    if (player->is_loaded) {
        char *basename = g_path_get_basename(player->current_file);
        char label_text[512];
        snprintf(label_text, sizeof(label_text), "File: %s (%.1f sec) [%d/%d]", 
                basename, player->song_duration, player->queue.current_index + 1, player->queue.count);
        gtk_label_set_text(GTK_LABEL(player->file_label), label_text);
        g_free(basename);
    } else {
        gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
    }
}

// Progress scale value changed handler for seeking
void on_progress_scale_value_changed(GtkRange *range, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    if (!player->is_loaded || player->seeking) {
        return;
    }
    
    double new_position = gtk_range_get_value(range);
    
    // Set seeking flag to prevent feedback
    player->seeking = true;
    
    // Seek to the new position
    seek_to_position(player, new_position);
    
    // Clear seeking flag after a short delay
    g_timeout_add(50, [](gpointer data) -> gboolean {
        AudioPlayer *p = (AudioPlayer*)data;
        p->seeking = false;
        return FALSE; // Don't repeat
    }, player);
}

// Queue button callbacks
void on_add_to_queue_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
#ifdef _WIN32
    char filename[2048] = "";  // Larger buffer for multiple files
    if (open_windows_file_dialog(filename, sizeof(filename), true)) {  // true for multiple selection
        bool was_empty_queue = (player->queue.count == 0);
        
        // Helper function to check if file extension is supported
        auto is_supported_extension = [](const char* filename) -> bool {
            const char* ext = strrchr(filename, '.');
            if (!ext) return false;
            
            // Convert to lowercase for comparison
            char ext_lower[10];
            strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
            ext_lower[sizeof(ext_lower) - 1] = '\0';
            for (int i = 0; ext_lower[i]; i++) {
                ext_lower[i] = tolower(ext_lower[i]);
            }
            
            return (strcmp(ext_lower, ".mid") == 0 || 
                    strcmp(ext_lower, ".midi") == 0 ||
                    strcmp(ext_lower, ".wav") == 0 ||
                    strcmp(ext_lower, ".mp3") == 0 ||
                    strcmp(ext_lower, ".m4a") == 0 ||
                    strcmp(ext_lower, ".ogg") == 0 ||
                    strcmp(ext_lower, ".aif") == 0 ||
                    strcmp(ext_lower, ".aiff") == 0 ||
                    strcmp(ext_lower, ".opus") == 0 ||
                    strcmp(ext_lower, ".flac") == 0);
        };
        
        // Parse multiple filenames from Windows dialog
        // Windows returns multiple files as: "directory\0file1.ext\0file2.ext\0\0"
        // or single file as: "full\path\to\file.ext\0"
        
        char *ptr = filename;
        char directory[512] = "";
        
        // Check if this is multiple files (contains directory + files)
        char *next_null = strchr(ptr, '\0');
        if (next_null && *(next_null + 1) != '\0') {
            // Multiple files: first string is directory
            strcpy(directory, ptr);
            ptr = next_null + 1;
            
            // Add each file (with extension validation)
            while (*ptr) {
                char full_path[32768];
                snprintf(full_path, sizeof(full_path), "%s\\%s", directory, ptr);
                
                if (is_supported_extension(full_path)) {
                    add_to_queue(&player->queue, full_path);
                } else {
                    printf("Skipping unsupported file: %s\n", full_path);
                }
                
                // Move to next filename
                ptr += strlen(ptr) + 1;
            }
        } else {
            // Single file (with extension validation)
            if (is_supported_extension(filename)) {
                add_to_queue(&player->queue, filename);
            } else {
                printf("Unsupported file type: %s\n", filename);
            }
        }
        
        // If this was the first file(s) added to an empty queue, load and start playing
        if (was_empty_queue && player->queue.count > 0) {
            if (load_file_from_queue(player)) {
                update_gui_state(player);
                // load_file now auto-starts playback
            }
        }
        
        update_queue_display(player);
        update_gui_state(player);
    }
#else
    // Your existing GTK file dialog code for Linux/Mac
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Add to Queue",
                                                    GTK_WINDOW(player->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Add", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    
    // Add file filters
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Supported Files");
    gtk_file_filter_add_pattern(all_filter, "*.mid");
    gtk_file_filter_add_pattern(all_filter, "*.midi");
    gtk_file_filter_add_pattern(all_filter, "*.wav");
    gtk_file_filter_add_pattern(all_filter, "*.mp3");
    gtk_file_filter_add_pattern(all_filter, "*.m4a");
    gtk_file_filter_add_pattern(all_filter, "*.ogg");
    gtk_file_filter_add_pattern(all_filter, "*.flac");
    gtk_file_filter_add_pattern(all_filter, "*.aif");
    gtk_file_filter_add_pattern(all_filter, "*.aiff");
    gtk_file_filter_add_pattern(all_filter, "*.opus");
    gtk_file_filter_add_pattern(all_filter, "*.wma");


    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);
    
    GtkFileFilter *midi_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(midi_filter, "MIDI Files (*.mid, *.midi)");
    gtk_file_filter_add_pattern(midi_filter, "*.mid");
    gtk_file_filter_add_pattern(midi_filter, "*.midi");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), midi_filter);
    
    GtkFileFilter *wav_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(wav_filter, "WAV Files (*.wav)");
    gtk_file_filter_add_pattern(wav_filter, "*.wav");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), wav_filter);
    
    GtkFileFilter *mp3_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(mp3_filter, "MP3 Files (*.mp3)");
    gtk_file_filter_add_pattern(mp3_filter, "*.mp3");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), mp3_filter);
    
    GtkFileFilter *ogg_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(ogg_filter, "OGG Files (*.ogg)");
    gtk_file_filter_add_pattern(ogg_filter, "*.ogg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ogg_filter);

    GtkFileFilter *flac_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(flac_filter, "FLAC Files (*.flac)");
    gtk_file_filter_add_pattern(flac_filter, "*.flac");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flac_filter);

    GtkFileFilter *aiff_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(aiff_filter, "AIFF Files (*.aiff)");
    gtk_file_filter_add_pattern(aiff_filter, "*.aiff");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), aiff_filter);

    GtkFileFilter *opus_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(opus_filter, "OPUS Files (*.opus)");
    gtk_file_filter_add_pattern(opus_filter, "*.opus");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), opus_filter);

    GtkFileFilter *m4a_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(m4a_filter, "M4A Files (*.m4a)");
    gtk_file_filter_add_pattern(m4a_filter, "*.m4a");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), m4a_filter);

    GtkFileFilter *wma_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(wma_filter, "wma Files (*.wma)");
    gtk_file_filter_add_pattern(wma_filter, "*.wma");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), wma_filter);

    GtkFileFilter *generic_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(generic_filter, "All other files (*.*)");
    gtk_file_filter_add_pattern(generic_filter, "*.*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), generic_filter);

    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        
        bool was_empty_queue = (player->queue.count == 0);
        
        for (GSList *iter = filenames; iter != NULL; iter = g_slist_next(iter)) {
            char *filename = (char*)iter->data;
            add_to_queue(&player->queue, filename);
            g_free(filename);
        }
        
        g_slist_free(filenames);
        
        // If this was the first file(s) added to an empty queue, load and start playing
        if (was_empty_queue && player->queue.count > 0) {
            if (load_file_from_queue(player)) {
                update_gui_state(player);
                // load_file now auto-starts playback
            }
        }
        
        update_queue_display(player);
        update_gui_state(player);
    }
    
    gtk_widget_destroy(dialog);
#endif
}


void on_clear_queue_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    stop_playback(player);
    clear_queue(&player->queue);
    update_queue_display(player);
    update_gui_state(player);
    player->is_loaded = false;
    
    gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
}

void on_repeat_queue_toggled(GtkToggleButton *button, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    player->queue.repeat_queue = gtk_toggle_button_get_active(button);
    
    printf("Queue repeat: %s\n", player->queue.repeat_queue ? "ON" : "OFF");
}

// Menu callbacks
void on_menu_open(GtkMenuItem *menuitem, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
#ifdef _WIN32
    char filename[32768];
    if (open_windows_file_dialog(filename, sizeof(filename))) {
        // Clear queue and add this single file
        clear_queue(&player->queue);
        add_to_queue(&player->queue, filename);
        
        if (load_file_from_queue(player)) {
            printf("Successfully loaded: %s\n", filename);
            update_queue_display(player);
            update_gui_state(player);
        }
    }
#else
    // Your existing GTK file dialog code for Linux/Mac
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Audio File",
                                                    GTK_WINDOW(player->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    // File filters
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Supported Files");
    gtk_file_filter_add_pattern(all_filter, "*.mid");
    gtk_file_filter_add_pattern(all_filter, "*.midi");
    gtk_file_filter_add_pattern(all_filter, "*.wav");
    gtk_file_filter_add_pattern(all_filter, "*.mp3");
    gtk_file_filter_add_pattern(all_filter, "*.ogg");
    gtk_file_filter_add_pattern(all_filter, "*.flac");
    gtk_file_filter_add_pattern(all_filter, "*.aiff");
    gtk_file_filter_add_pattern(all_filter, "*.aif");
    gtk_file_filter_add_pattern(all_filter, "*.opus");
    gtk_file_filter_add_pattern(all_filter, "*.m4a");
    gtk_file_filter_add_pattern(all_filter, "*.wma");


    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);
    
    GtkFileFilter *midi_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(midi_filter, "MIDI Files (*.mid, *.midi)");
    gtk_file_filter_add_pattern(midi_filter, "*.mid");
    gtk_file_filter_add_pattern(midi_filter, "*.midi");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), midi_filter);
    
    GtkFileFilter *wav_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(wav_filter, "WAV Files (*.wav)");
    gtk_file_filter_add_pattern(wav_filter, "*.wav");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), wav_filter);
    
    GtkFileFilter *mp3_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(mp3_filter, "MP3 Files (*.mp3)");
    gtk_file_filter_add_pattern(mp3_filter, "*.mp3");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), mp3_filter);
    
    GtkFileFilter *ogg_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(ogg_filter, "OGG Files (*.ogg)");
    gtk_file_filter_add_pattern(ogg_filter, "*.ogg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ogg_filter);

    GtkFileFilter *flac_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(flac_filter, "FLAC Files (*.flac)");
    gtk_file_filter_add_pattern(flac_filter, "*.flac");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flac_filter);

    GtkFileFilter *aiff_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(aiff_filter, "AIFF Files (*.aiff)");
    gtk_file_filter_add_pattern(aiff_filter, "*.aiff");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), aiff_filter);

    GtkFileFilter *opus_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(opus_filter, "OPUS Files (*.opus)");
    gtk_file_filter_add_pattern(opus_filter, "*.opus");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), opus_filter);

    GtkFileFilter *m4a_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(m4a_filter, "M4A Files (*.m4a)");
    gtk_file_filter_add_pattern(m4a_filter, "*.m4a");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), m4a_filter);

    GtkFileFilter *wma_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(wma_filter, "WMA Files (*.wma)");
    gtk_file_filter_add_pattern(wma_filter, "*.wma");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), wma_filter);

    GtkFileFilter *generic_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(generic_filter, "All Other Files (*.*)");
    gtk_file_filter_add_pattern(generic_filter, "*.*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), generic_filter);


    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Clear queue and add this single file
        clear_queue(&player->queue);
        add_to_queue(&player->queue, filename);
        
        if (load_file_from_queue(player)) {
            printf("Successfully loaded: %s\n", filename);
            update_queue_display(player);
            update_gui_state(player);
            // load_file now auto-starts playback
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                                             GTK_MESSAGE_ERROR,
                                                             GTK_BUTTONS_CLOSE,
                                                             "Failed to load file:\n%s", filename);
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
#endif
}


void on_window_resize(GtkWidget *widget, gpointer user_data) {

    // Get screen resolution
    GdkScreen *screen = gtk_widget_get_screen(widget);
    int screen_width = gdk_screen_get_width(screen);
    int screen_height = gdk_screen_get_height(screen);
    printf("Screen %i %i\n", screen_width, screen_height);

    printf("Screen resolution: %dx%d\n", screen_width, screen_height);

    // Adaptive base sizes based on screen resolution category
    int base_window_width, base_window_height, base_player_width;
    int base_vis_width, base_vis_height, base_queue_width, base_queue_height;
    
    if (screen_width <= 800 || screen_height <= 600) {
        // Very small screens (800x600, etc.) - use much smaller visualization
        base_window_width = screen_width-50;
        base_window_height = screen_height-50;
        base_player_width = 200;
        base_vis_width = 100;  // Much smaller visualization
        base_vis_height = 80;  // Much smaller visualization
        base_queue_width = 100;
        base_queue_height = 100;
        printf("Using very small screen base sizes\n");
    } else if (screen_width < 1200 || screen_height < 900) {
        // Medium screens (1024x768, etc.) - moderately smaller visualization
        base_window_width = 800;
        base_window_height = 600;
        base_player_width = 400;
        base_vis_width = 260;  // Smaller visualization
        base_vis_height = 120; // Smaller visualization
        base_queue_width = 250;
        base_queue_height = 350;
        printf("Using medium-screen base sizes\n");
    } else {
        // Large screens (1920x1080+) - keep current size
        base_window_width = 900;
        base_window_height = 700;
        base_player_width = 500;
        base_vis_width = 400;
        base_vis_height = 200;
        base_queue_width = 300;
        base_queue_height = 400;
        printf("Using large-screen base sizes\n");
    }

    // Use a more appropriate reference resolution based on screen category
    int ref_width = (screen_width < 1200) ? 1024 : 1920;
    int ref_height = (screen_height < 900) ? 768 : 1080;

    // Calculate appropriate sizes
    int window_width = scale_size(base_window_width, screen_width, ref_width);
    int window_height = scale_size(base_window_height, screen_height, ref_height);
    int player_width = scale_size(base_player_width, screen_width, ref_width);
    int vis_width = scale_size(base_vis_width, screen_width, ref_width);
    int vis_height = scale_size(base_vis_height, screen_height, ref_height);
    int queue_width = scale_size(base_queue_width, screen_width, ref_width);
    int queue_height = scale_size(base_queue_height, screen_height, ref_height);

    int scale = gtk_widget_get_scale_factor(player->window);
    if (scale>1) {
        window_width/=scale;
        window_height/=scale;
        player_width/=scale;
        vis_width/=scale;
        vis_height/=scale;
        queue_width/=scale;
        queue_height/=scale;
    }


    // Apply more aggressive minimum sizes for very small screens
    if (screen_width <= 800) {
        //window_width = fmax(window_width, screen_width - 50);  // Leave some margin
        //window_height = fmax(window_height, screen_height - 50);        player_width = fmax(player_width, 300);
        window_width = screen_width;
        window_height = screen_height;
        vis_width = fmax(vis_width, 180);   // Smaller minimum
        vis_height = fmax(vis_height, 60);  // Much smaller minimum
        queue_width = fmax(queue_width, 180);
        queue_height = fmax(queue_height, 250);
    } else if (screen_width <= 1024) {
        window_width = fmax(window_width, 800);
        window_height = fmax(window_height, 600);
        player_width = fmax(player_width, 400);
        vis_width = fmax(vis_width, 220);   // Smaller minimum
        vis_height = fmax(vis_height, 100); // Smaller minimum
        queue_width = fmax(queue_width, 250);
        queue_height = fmax(queue_height, 300);
    } else {
        window_width = fmax(window_width, 800);
        window_height = fmax(window_height, 600);
        player_width = fmax(player_width, 400);
        vis_width = fmax(vis_width, 300);
        vis_height = fmax(vis_height, 150);
        queue_width = fmax(queue_width, 250);
        queue_height = fmax(queue_height, 300);
    }

    printf("Final sizes: window=%dx%d, player=%d, vis=%dx%d, queue=%dx%d\n",
           window_width, window_height, player_width, vis_width, vis_height, queue_width, queue_height);

    // Resize window
    //gtk_window_resize(GTK_WINDOW(widget), window_width, window_height);

    // Adjust player vbox width
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
    if (children && children->data) {
        GtkWidget *main_hbox = GTK_WIDGET(children->data);
        GList *hbox_children = gtk_container_get_children(GTK_CONTAINER(main_hbox));
        if (hbox_children && hbox_children->data) {
            GtkWidget *player_vbox = GTK_WIDGET(hbox_children->data);
            gtk_widget_set_size_request(player_vbox, player_width, -1);
        }
        g_list_free(hbox_children);
    }
    g_list_free(children);

    // Adjust visualizer size
    if (player->visualizer && player->visualizer->drawing_area) {
        gtk_widget_set_size_request(player->visualizer->drawing_area, vis_width, vis_height);
        printf("Set visualizer size to: %dx%d\n", vis_width, vis_height);
    }

    // Adjust queue scrolled window
    if (player->queue_scrolled_window) {
        gtk_widget_set_size_request(player->queue_scrolled_window, queue_width, queue_height);
    }

}

void on_window_realize(GtkWidget *widget, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
}

int scale_size(int base_size, int screen_dimension, int base_dimension) {
    if (screen_dimension < base_dimension) {
        // Scale down for smaller screens, but with a minimum ratio
        double scale_ratio = (double)screen_dimension / base_dimension;
        
        // Don't scale below 60% of original size to keep things usable
        scale_ratio = fmax(scale_ratio, 0.6);
        
        return (int)(base_size * scale_ratio);
    } else {
        // Keep base size or scale up slightly for larger screens
        double scale = fmin(1.5, (double)screen_dimension / base_dimension);
        return (int)(base_size * scale);
    }
}

double get_scale_factor(GtkWidget *widget) {
    if (!widget || !gtk_widget_get_realized(widget)) {
        return 1.0;
    }
    
    GdkWindow *window = gtk_widget_get_window(widget);
    if (!window) {
        return 1.0;
    }
    
    GdkDisplay *display = gdk_window_get_display(window);
    GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, window);
    
    if (monitor) {
        return gdk_monitor_get_scale_factor(monitor);
    }
    
    return 1.0;
}

void on_menu_quit(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    gtk_main_quit();
}

// Button callbacks
void on_play_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    start_playback((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

void on_pause_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    toggle_pause((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

void on_stop_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    stop_playback((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

void on_rewind_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    rewind_5_seconds((AudioPlayer*)user_data);
}

void on_fast_forward_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    fast_forward_5_seconds((AudioPlayer*)user_data);
}

void on_next_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    next_song((AudioPlayer*)user_data);
}

void on_previous_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    previous_song((AudioPlayer*)user_data);
}

void on_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double value = gtk_range_get_value(range);
    globalVolume = (int)(value * 100);
}

void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (player->equalizer) {
        equalizer_free(player->equalizer);
    }
    
    // This will be called after delete-event, so just quit
    gtk_main_quit();
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

gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    (void)widget;
    (void)event;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    printf("Window close button pressed, cleaning up...\n");
    
    stop_playback(player);
    clear_queue(&player->queue);
    cleanup_conversion_cache(&player->conversion_cache);
    cleanup_virtual_filesystem();
    
    if (player->audio_buffer.data) free(player->audio_buffer.data);
    if (player->audio_device) SDL_CloseAudioDevice(player->audio_device);
    
    SDL_Quit();
    
    gtk_main_quit();
    return FALSE; // Allow the window to be destroyed
}

void on_menu_load_playlist(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
#ifdef _WIN32
    char filename[32768];
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = "M3U Playlists\0*.m3u;*.m3u8\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    filename[0] = '\0';
    
    if (GetOpenFileName(&ofn)) {
        if (load_m3u_playlist(player, filename)) {
            // ADD TO RECENT FILES
            add_to_recent_files(filename, "audio/x-mpegurl");
        }
    }
#else
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Load Playlist",
                                                    GTK_WINDOW(player->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Load", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    GtkFileFilter *m3u_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(m3u_filter, "M3U Playlists (*.m3u, *.m3u8)");
    gtk_file_filter_add_pattern(m3u_filter, "*.m3u");
    gtk_file_filter_add_pattern(m3u_filter, "*.m3u8");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), m3u_filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (load_m3u_playlist(player, filename)) {
            // ADD TO RECENT FILES
            add_to_recent_files(filename, "audio/x-mpegurl");
        }
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
#endif
}

// Update your existing on_menu_save_playlist function:
void on_menu_save_playlist(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    if (player->queue.count == 0) {
        GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                                         GTK_MESSAGE_WARNING,
                                                         GTK_BUTTONS_OK,
                                                         "No files in queue to save");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return;
    }
    
#ifdef _WIN32
    char filename[32768];
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = "M3U Playlists\0*.m3u\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = "m3u";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    strcpy(filename, "playlist.m3u");
    
    if (GetSaveFileName(&ofn)) {
        if (save_m3u_playlist(player, filename)) {
            // ADD TO RECENT FILES
            add_to_recent_files(filename, "audio/x-mpegurl");
        }
    }
#else
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Playlist",
                                                    GTK_WINDOW(player->window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "playlist.m3u");
    
    GtkFileFilter *m3u_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(m3u_filter, "M3U Playlists (*.m3u)");
    gtk_file_filter_add_pattern(m3u_filter, "*.m3u");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), m3u_filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (save_m3u_playlist(player, filename)) {
            // ADD TO RECENT FILES
            add_to_recent_files(filename, "audio/x-mpegurl");
        }
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
#endif
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



int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Initialize virtual filesystem
    init_virtual_filesystem();
    
    player = (AudioPlayer*)g_malloc0(sizeof(AudioPlayer));
    pthread_mutex_init(&player->audio_mutex, NULL);
    player->playback_speed = 1.0; 
    player->speed_accumulator = 0.0;    
    
    // Initialize queue and conversion cache
    init_queue(&player->queue);
    init_conversion_cache(&player->conversion_cache);
    
    if (!init_audio(player)) {
        printf("Audio initialization failed\n");
        cleanup_conversion_cache(&player->conversion_cache);
        cleanup_virtual_filesystem();
        return 1;
    }
    
    player->equalizer = equalizer_new(SAMPLE_RATE);
    if (!player->equalizer) {
        printf("Failed to initialize equalizer\n");
    }
    
    // Initialize OPL for MIDI conversion
    OPL_Init(SAMPLE_RATE);
    OPL_LoadInstruments();
    
    create_main_window(player);
    update_gui_state(player);
    gtk_widget_show_all(player->window);
    
    if (argc > 1) {
        // Check if first argument is an M3U playlist
        const char *first_arg = argv[1];
        const char *ext = strrchr(first_arg, '.');
        
        if (ext && (strcasecmp(ext, ".m3u") == 0 || strcasecmp(ext, ".m3u8") == 0)) {
            // Load M3U playlist
            printf("Loading M3U playlist: %s\n", first_arg);
            load_m3u_playlist(player, first_arg);
            
            // Add any additional arguments as individual files
            for (int i = 2; i < argc; i++) {
                add_to_queue(&player->queue, argv[i]);
            }
        } else {
            // Add all command line arguments to queue as individual files
            for (int i = 1; i < argc; i++) {
                add_to_queue(&player->queue, argv[i]);
            }
        }
        
        if (player->queue.count > 0 && load_file_from_queue(player)) {
            printf("Loaded and auto-starting first file in queue\n");
            update_queue_display(player);
            update_gui_state(player);
            // load_file now auto-starts playback
        }
    }
    
    gtk_main();
    
    // Clean up visualizer
    if (player->visualizer) {
        visualizer_free(player->visualizer);
    }
    
    clear_queue(&player->queue);
    cleanup_conversion_cache(&player->conversion_cache);
    cleanup_virtual_filesystem();
    pthread_mutex_destroy(&player->audio_mutex);
    
    
    g_free(player);
    return 0;
}
