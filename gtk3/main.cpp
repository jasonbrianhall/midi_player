#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include "midiplayer.h"
#include "dbopl_wrapper.h"

// Global variables for GUI
typedef struct {
    GtkWidget *window;
    GtkWidget *play_button;
    GtkWidget *pause_button;
    GtkWidget *stop_button;
    GtkWidget *open_button;
    GtkWidget *progress_bar;
    GtkWidget *time_label;
    GtkWidget *volume_scale;
    GtkWidget *file_label;
    
    // Playback state
    bool is_loaded;
    bool is_playing;
    bool is_paused;
    char current_file[512];
    double song_duration;
    guint update_timer_id;
    
    // Synchronization
    pthread_mutex_t state_mutex;
} MidiPlayer;

// External variables from midiplayer.cpp
extern double playTime;
extern bool isPlaying;
extern bool paused;
extern int globalVolume;
extern void processEvents(void);
extern double playwait;
extern SDL_AudioDeviceID audioDevice;
extern int TrackCount;
extern int ChPatch[16];
extern double ChBend[16];
extern int ChVolume[16];
extern int ChPanning[16];
extern int ChVibrato[16];
extern double tkDelay[];
extern int tkStatus[];

MidiPlayer *player = NULL;

// Forward declarations
static void update_gui_state(MidiPlayer *player);
static gboolean update_progress(gpointer user_data);

// Initialize MIDI player
static bool init_midi_player(MidiPlayer *player_ctx) {
    (void)player_ctx; // Suppress unused parameter warning
    
    if (!initSDL()) {
        g_print("Failed to initialize SDL\n");
        return false;
    }
    
    // Initialize OPL
    OPL_Init(SAMPLE_RATE);
    OPL_LoadInstruments();
    
    return true;
}

// Load MIDI file
static bool load_midi_file(MidiPlayer *player, const char *filename) {
    pthread_mutex_lock(&player->state_mutex);
    
    // Stop current playback
    if (player->is_playing) {
        isPlaying = false;
        player->is_playing = false;
        SDL_PauseAudioDevice(audioDevice, 1);
    }
    
    // Load new file
    if (!loadMidiFile(filename)) {
        pthread_mutex_unlock(&player->state_mutex);
        return false;
    }
    
    strcpy(player->current_file, filename);
    player->is_loaded = true;
    player->is_playing = false;
    player->is_paused = false;
    playTime = 0;
    
    // Estimate song duration (this is approximate)
    player->song_duration = 180.0; // Default 3 minutes, could be improved with MIDI analysis
    
    pthread_mutex_unlock(&player->state_mutex);
    
    return true;
}

// Start playback
static void start_playback(MidiPlayer *player) {
    if (!player->is_loaded) return;
    
    pthread_mutex_lock(&player->state_mutex);
    
    // Initialize playback variables
    playTime = 0;
    isPlaying = true;
    paused = false;
    player->is_playing = true;
    player->is_paused = false;
    
    // Reset OPL
    OPL_Reset();
    
    // Initialize MIDI channels
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 64;
        ChVibrato[i] = 0;
    }
    
    // Reset track state
    for (int tk = 0; tk < TrackCount; tk++) {
        // Reset to beginning positions (this would need proper MIDI file reset)
        tkDelay[tk] = 0;
        tkStatus[tk] = 0;
    }
    
    playwait = 0;
    
    // Start audio
    SDL_PauseAudioDevice(audioDevice, 0);
    
    // Start progress updates
    if (player->update_timer_id == 0) {
        player->update_timer_id = g_timeout_add(100, update_progress, player);
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Pause/resume playback
static void toggle_pause(MidiPlayer *player) {
    if (!player->is_playing) return;
    
    pthread_mutex_lock(&player->state_mutex);
    
    paused = !paused;
    player->is_paused = paused;
    
    if (paused) {
        SDL_PauseAudioDevice(audioDevice, 1);
    } else {
        SDL_PauseAudioDevice(audioDevice, 0);
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Stop playback
static void stop_playback(MidiPlayer *player) {
    pthread_mutex_lock(&player->state_mutex);
    
    isPlaying = false;
    paused = false;
    player->is_playing = false;
    player->is_paused = false;
    playTime = 0;
    
    SDL_PauseAudioDevice(audioDevice, 1);
    OPL_Reset();
    
    // Remove progress timer
    if (player->update_timer_id > 0) {
        g_source_remove(player->update_timer_id);
        player->update_timer_id = 0;
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Seek to position (0.0 to 1.0)
static void seek_to_position(MidiPlayer *player, double position) {
    if (!player->is_loaded || position < 0.0 || position > 1.0) return;
    
    pthread_mutex_lock(&player->state_mutex);
    
    // This is a simplified seek - in a real implementation,
    // you'd need to parse through the MIDI file to the correct time position
    double target_time = position * player->song_duration;
    
    // For now, we'll just restart from the beginning if seeking backwards
    // or continue if seeking forward (very basic implementation)
    if (target_time < playTime) {
        // Restart from beginning
        playTime = 0;
        // Reset MIDI state (simplified)
        OPL_Reset();
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Update GUI state
static void update_gui_state(MidiPlayer *player) {
    // Update button sensitivity
    gtk_widget_set_sensitive(player->play_button, player->is_loaded && !player->is_playing);
    gtk_widget_set_sensitive(player->pause_button, player->is_playing);
    gtk_widget_set_sensitive(player->stop_button, player->is_playing || player->is_paused);
    
    // Update file label
    if (player->is_loaded) {
        char *basename = g_path_get_basename(player->current_file);
        char label_text[256];
        snprintf(label_text, sizeof(label_text), "File: %s", basename);
        gtk_label_set_text(GTK_LABEL(player->file_label), label_text);
        g_free(basename);
    } else {
        gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
    }
}

// Progress update callback
static gboolean update_progress(gpointer user_data) {
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    pthread_mutex_lock(&player->state_mutex);
    
    if (!player->is_playing) {
        pthread_mutex_unlock(&player->state_mutex);
        return FALSE; // Remove timer
    }
    
    // Update progress bar
    double progress = 0.0;
    if (player->song_duration > 0) {
        progress = playTime / player->song_duration;
        if (progress > 1.0) progress = 1.0;
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar), progress);
    
    // Update time label
    int minutes = (int)(playTime / 60);
    int seconds = (int)playTime % 60;
    int total_minutes = (int)(player->song_duration / 60);
    int total_seconds = (int)player->song_duration % 60;
    
    char time_text[64];
    snprintf(time_text, sizeof(time_text), "%02d:%02d / %02d:%02d", 
             minutes, seconds, total_minutes, total_seconds);
    gtk_label_set_text(GTK_LABEL(player->time_label), time_text);
    
    // Check if song ended
    if (!isPlaying) {
        player->is_playing = false;
        player->is_paused = false;
        update_gui_state(player);
        pthread_mutex_unlock(&player->state_mutex);
        return FALSE; // Remove timer
    }
    
    pthread_mutex_unlock(&player->state_mutex);
    return TRUE; // Continue timer
}

// Button callback functions
static void on_play_clicked(GtkButton *button, gpointer user_data) {
    (void)button; // Suppress unused parameter warning
    MidiPlayer *player = (MidiPlayer*)user_data;
    start_playback(player);
    update_gui_state(player);
}

static void on_pause_clicked(GtkButton *button, gpointer user_data) {
    (void)button; // Suppress unused parameter warning
    MidiPlayer *player = (MidiPlayer*)user_data;
    toggle_pause(player);
    update_gui_state(player);
}

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
    (void)button; // Suppress unused parameter warning
    MidiPlayer *player = (MidiPlayer*)user_data;
    stop_playback(player);
    update_gui_state(player);
}

static void on_open_clicked(GtkButton *button, gpointer user_data) {
    (void)button; // Suppress unused parameter warning
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open MIDI File",
                                                    GTK_WINDOW(player->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    // Add MIDI file filter
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "MIDI Files");
    gtk_file_filter_add_pattern(filter, "*.mid");
    gtk_file_filter_add_pattern(filter, "*.midi");
    gtk_file_filter_add_pattern(filter, "*.MID");
    gtk_file_filter_add_pattern(filter, "*.MIDI");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (load_midi_file(player, filename)) {
            g_print("Loaded: %s\n", filename);
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                                             GTK_MESSAGE_ERROR,
                                                             GTK_BUTTONS_CLOSE,
                                                             "Failed to load MIDI file:\n%s", filename);
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
        
        g_free(filename);
        update_gui_state(player);
    }
    
    gtk_widget_destroy(dialog);
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data; // Suppress unused parameter warning
    double value = gtk_range_get_value(range);
    globalVolume = (int)(value * 100);
}

static void on_progress_seek(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    if (!player->is_loaded) return;
    
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    double position = event->x / allocation.width;
    seek_to_position(player, position);
}

// Window destroy callback
static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget; // Suppress unused parameter warning
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    // Stop playback
    stop_playback(player);
    
    // Cleanup
    cleanup();
    
    gtk_main_quit();
}

// Create the main window
static GtkWidget* create_main_window(MidiPlayer *player) {
    // Main window
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), "GTK MIDI Player");
    gtk_window_set_default_size(GTK_WINDOW(player->window), 600, 200);
    gtk_container_set_border_width(GTK_CONTAINER(player->window), 10);
    
    // Main vertical box
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(player->window), main_vbox);
    
    // File info
    player->file_label = gtk_label_new("No file loaded");
    gtk_box_pack_start(GTK_BOX(main_vbox), player->file_label, FALSE, FALSE, 0);
    
    // Progress bar
    player->progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(player->progress_bar), FALSE);
    g_signal_connect(player->progress_bar, "button-press-event", G_CALLBACK(on_progress_seek), player);
    gtk_box_pack_start(GTK_BOX(main_vbox), player->progress_bar, FALSE, FALSE, 0);
    
    // Time label
    player->time_label = gtk_label_new("00:00 / 00:00");
    gtk_box_pack_start(GTK_BOX(main_vbox), player->time_label, FALSE, FALSE, 0);
    
    // Control buttons
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(button_hbox), TRUE);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_hbox, FALSE, FALSE, 0);
    
    player->open_button = gtk_button_new_with_label("Open");
    player->play_button = gtk_button_new_with_label("Play");
    player->pause_button = gtk_button_new_with_label("Pause");
    player->stop_button = gtk_button_new_with_label("Stop");
    
    gtk_box_pack_start(GTK_BOX(button_hbox), player->open_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_hbox), player->play_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_hbox), player->pause_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_hbox), player->stop_button, TRUE, TRUE, 0);
    
    // Volume control
    GtkWidget *volume_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), volume_hbox, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Volume:");
    player->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
    gtk_range_set_value(GTK_RANGE(player->volume_scale), 1.0);
    gtk_scale_set_value_pos(GTK_SCALE(player->volume_scale), GTK_POS_RIGHT);
    
    gtk_box_pack_start(GTK_BOX(volume_hbox), volume_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(volume_hbox), player->volume_scale, TRUE, TRUE, 0);
    
    // Connect signals
    g_signal_connect(player->window, "destroy", G_CALLBACK(on_window_destroy), player);
    g_signal_connect(player->open_button, "clicked", G_CALLBACK(on_open_clicked), player);
    g_signal_connect(player->play_button, "clicked", G_CALLBACK(on_play_clicked), player);
    g_signal_connect(player->pause_button, "clicked", G_CALLBACK(on_pause_clicked), player);
    g_signal_connect(player->stop_button, "clicked", G_CALLBACK(on_stop_clicked), player);
    g_signal_connect(player->volume_scale, "value-changed", G_CALLBACK(on_volume_changed), player);
    
    return player->window;
}

// Main function
int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create player structure
    player = g_malloc0(sizeof(MidiPlayer));
    pthread_mutex_init(&player->state_mutex, NULL);
    
    // Initialize MIDI player
    if (!init_midi_player(player)) {
        g_print("Failed to initialize MIDI player\n");
        return 1;
    }
    
    // Create GUI
    create_main_window(player);
    update_gui_state(player);
    
    // Show window
    gtk_widget_show_all(player->window);
    
    // Load file from command line if provided
    if (argc > 1) {
        if (load_midi_file(player, argv[1])) {
            g_print("Loaded: %s\n", argv[1]);
            update_gui_state(player);
        }
    }
    
    // Run main loop
    gtk_main();
    
    // Cleanup
    pthread_mutex_destroy(&player->state_mutex);
    g_free(player);
    
    return 0;
}
