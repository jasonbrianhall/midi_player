#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "midiplayer.h"
#include "dbopl_wrapper.h"
#include "wav_converter.h"

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
    char temp_wav_file[512];
    double song_duration;
    guint update_timer_id;
    
    // Audio playback
    pid_t audio_player_pid;
    
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

MidiPlayer *player = NULL;

// Forward declarations
static void update_gui_state(MidiPlayer *player);
static gboolean update_progress(gpointer user_data);
static void stop_playback(MidiPlayer *player);

// Convert MIDI to WAV file
static bool convert_midi_to_wav(MidiPlayer *player, const char* filename) {
    // Create WAV file path in /tmp
    char *basename = g_path_get_basename(filename);
    char *dot = strrchr(basename, '.');
    if (dot) *dot = '\0';
    snprintf(player->temp_wav_file, sizeof(player->temp_wav_file), "/tmp/%s_converted.wav", basename);
    g_free(basename);
    
    printf("\n========================================\n");
    printf("CONVERTING MIDI TO WAV\n");
    printf("Input:  %s\n", filename);
    printf("Output: %s\n", player->temp_wav_file);
    printf("========================================\n");
    
    // Convert MIDI to WAV using original working code
    playTime = 0;
    isPlaying = true;
    
    if (!initSDL()) {
        printf("Failed to initialize SDL for conversion\n");
        return false;
    }
    
    if (!loadMidiFile(filename)) {
        printf("Failed to load MIDI file\n");
        cleanup();
        return false;
    }
    
    WAVConverter* wav_converter = wav_converter_init(player->temp_wav_file, SAMPLE_RATE, AUDIO_CHANNELS);
    if (!wav_converter) {
        printf("Failed to create WAV converter\n");
        cleanup();
        return false;
    }
    
    int16_t audio_buffer[AUDIO_BUFFER * AUDIO_CHANNELS];
    double buffer_duration = (double)AUDIO_BUFFER / SAMPLE_RATE;
    
    processEvents();
    
    while (isPlaying) {
        memset(audio_buffer, 0, sizeof(audio_buffer));
        OPL_Generate(audio_buffer, AUDIO_BUFFER);
        
        if (!wav_converter_write(wav_converter, audio_buffer, AUDIO_BUFFER * AUDIO_CHANNELS)) {
            printf("Failed to write audio data\n");
            break;
        }
        
        playTime += buffer_duration;
        playwait -= buffer_duration;
        
        while (playwait <= 0 && isPlaying) {
            processEvents();
        }
        
        if (((int)playTime) % 5 == 0) {
            static int last_reported = -1;
            if ((int)playTime != last_reported) {
                printf("Converting... %d seconds\n", (int)playTime);
                last_reported = (int)playTime;
            }
        }
    }
    
    wav_converter_finish(wav_converter);
    wav_converter_free(wav_converter);
    cleanup();
    
    printf("\n========================================\n");
    printf("CONVERSION COMPLETE!\n");
    printf("WAV file: %s\n", player->temp_wav_file);
    printf("Duration: %.2f seconds\n", playTime);
    printf("Test with: aplay %s\n", player->temp_wav_file);
    printf("========================================\n");
    
    player->song_duration = playTime;
    return true;
}

// Load MIDI file and convert
static bool load_midi_file(MidiPlayer *player, const char *filename) {
    pthread_mutex_lock(&player->state_mutex);
    
    // Stop current playback
    if (player->is_playing) {
        stop_playback(player);
    }
    
    // Convert MIDI to WAV
    if (!convert_midi_to_wav(player, filename)) {
        pthread_mutex_unlock(&player->state_mutex);
        return false;
    }
    
    strcpy(player->current_file, filename);
    player->is_loaded = true;
    player->is_playing = false;
    player->is_paused = false;
    playTime = 0;
    
    pthread_mutex_unlock(&player->state_mutex);
    return true;
}

// Start playback using aplay
static void start_playback(MidiPlayer *player) {
    if (!player->is_loaded) return;
    
    printf("Starting playback with aplay: %s\n", player->temp_wav_file);
    
    pthread_mutex_lock(&player->state_mutex);
    
    // Kill any existing audio player
    if (player->audio_player_pid > 0) {
        kill(player->audio_player_pid, SIGTERM);
        waitpid(player->audio_player_pid, NULL, 0);
    }
    
    // Start aplay in background
    player->audio_player_pid = fork();
    if (player->audio_player_pid == 0) {
        // Child process - run aplay
        execlp("aplay", "aplay", player->temp_wav_file, NULL);
        exit(1); // If execlp fails
    } else if (player->audio_player_pid > 0) {
        // Parent process
        player->is_playing = true;
        player->is_paused = false;
        playTime = 0;
        
        // Start progress updates
        if (player->update_timer_id == 0) {
            player->update_timer_id = g_timeout_add(100, update_progress, player);
        }
        
        printf("Started aplay with PID %d\n", player->audio_player_pid);
    } else {
        printf("Failed to fork audio player process\n");
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Stop playback
static void stop_playback(MidiPlayer *player) {
    pthread_mutex_lock(&player->state_mutex);
    
    printf("Stopping playback\n");
    
    // Kill audio player process
    if (player->audio_player_pid > 0) {
        printf("Killing aplay process %d\n", player->audio_player_pid);
        kill(player->audio_player_pid, SIGTERM);
        waitpid(player->audio_player_pid, NULL, 0);
        player->audio_player_pid = 0;
    }
    
    player->is_playing = false;
    player->is_paused = false;
    playTime = 0;
    
    // Remove progress timer
    if (player->update_timer_id > 0) {
        g_source_remove(player->update_timer_id);
        player->update_timer_id = 0;
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Pause/resume playback (simplified - just stop/start)
static void toggle_pause(MidiPlayer *player) {
    if (!player->is_playing) return;
    
    pthread_mutex_lock(&player->state_mutex);
    
    if (player->is_paused) {
        // Resume - restart from current position (simplified)
        player->is_paused = false;
        printf("Resume not fully implemented - restarting from beginning\n");
        pthread_mutex_unlock(&player->state_mutex);
        start_playback(player);
    } else {
        // Pause - stop playback
        player->is_paused = true;
        if (player->audio_player_pid > 0) {
            kill(player->audio_player_pid, SIGSTOP);
            printf("Paused aplay process\n");
        }
        pthread_mutex_unlock(&player->state_mutex);
    }
}

// Update GUI state
static void update_gui_state(MidiPlayer *player) {
    gtk_widget_set_sensitive(player->play_button, player->is_loaded && !player->is_playing);
    gtk_widget_set_sensitive(player->pause_button, player->is_playing && !player->is_paused);
    gtk_widget_set_sensitive(player->stop_button, player->is_playing || player->is_paused);
    
    if (player->is_paused) {
        gtk_button_set_label(GTK_BUTTON(player->pause_button), "Resume");
    } else {
        gtk_button_set_label(GTK_BUTTON(player->pause_button), "Pause");
    }
    
    if (player->is_loaded) {
        char *basename = g_path_get_basename(player->current_file);
        char label_text[512];
        snprintf(label_text, sizeof(label_text), "File: %s (%.1f sec)\nWAV: %s", 
                 basename, player->song_duration, player->temp_wav_file);
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
    
    // Check if aplay process is still running
    if (player->audio_player_pid > 0) {
        int status;
        pid_t result = waitpid(player->audio_player_pid, &status, WNOHANG);
        if (result != 0) {
            // Process has ended
            printf("aplay process finished\n");
            player->is_playing = false;
            player->audio_player_pid = 0;
            playTime = player->song_duration; // Jump to end
        } else {
            // Process still running, estimate progress
            playTime += 0.1; // Rough estimate (100ms timer)
            if (playTime > player->song_duration) {
                playTime = player->song_duration;
            }
        }
    }
    
    if (!player->is_playing) {
        update_gui_state(player);
        pthread_mutex_unlock(&player->state_mutex);
        player->update_timer_id = 0;
        return FALSE;
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
    
    update_gui_state(player);
    
    pthread_mutex_unlock(&player->state_mutex);
    return TRUE;
}

// Button callbacks
static void on_play_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MidiPlayer *player = (MidiPlayer*)user_data;
    start_playback(player);
    update_gui_state(player);
}

static void on_pause_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MidiPlayer *player = (MidiPlayer*)user_data;
    toggle_pause(player);
    update_gui_state(player);
}

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MidiPlayer *player = (MidiPlayer*)user_data;
    stop_playback(player);
    update_gui_state(player);
}

static void on_open_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open MIDI File",
                                                    GTK_WINDOW(player->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "MIDI Files");
    gtk_file_filter_add_pattern(filter, "*.mid");
    gtk_file_filter_add_pattern(filter, "*.midi");
    gtk_file_filter_add_pattern(filter, "*.MID");
    gtk_file_filter_add_pattern(filter, "*.MIDI");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        GtkWidget *loading_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_INFO,
                                                          GTK_BUTTONS_NONE,
                                                          "Converting MIDI file to WAV...\nCheck console for progress.");
        gtk_widget_show_all(loading_dialog);
        
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
        
        if (load_midi_file(player, filename)) {
            printf("Successfully loaded and converted: %s\n", filename);
        } else {
            gtk_widget_destroy(loading_dialog);
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                                             GTK_MESSAGE_ERROR,
                                                             GTK_BUTTONS_CLOSE,
                                                             "Failed to convert MIDI file:\n%s", filename);
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
        
        gtk_widget_destroy(loading_dialog);
        g_free(filename);
        update_gui_state(player);
    }
    
    gtk_widget_destroy(dialog);
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double value = gtk_range_get_value(range);
    globalVolume = (int)(value * 100);
    printf("Volume changed to %d%%\n", globalVolume);
    // Note: Volume control would need to be implemented differently with external player
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    stop_playback(player);
    gtk_main_quit();
}

static GtkWidget* create_main_window(MidiPlayer *player) {
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), "GTK MIDI Player");
    gtk_window_set_default_size(GTK_WINDOW(player->window), 600, 250);
    gtk_container_set_border_width(GTK_CONTAINER(player->window), 10);
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(player->window), main_vbox);
    
    player->file_label = gtk_label_new("No file loaded");
    gtk_label_set_justify(GTK_LABEL(player->file_label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(main_vbox), player->file_label, FALSE, FALSE, 0);
    
    player->progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(player->progress_bar), FALSE);
    gtk_box_pack_start(GTK_BOX(main_vbox), player->progress_bar, FALSE, FALSE, 0);
    
    player->time_label = gtk_label_new("00:00 / 00:00");
    gtk_box_pack_start(GTK_BOX(main_vbox), player->time_label, FALSE, FALSE, 0);
    
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
    
    GtkWidget *volume_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), volume_hbox, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Volume:");
    player->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 5.0, 0.1);
    gtk_range_set_value(GTK_RANGE(player->volume_scale), (double)globalVolume / 100.0);
    gtk_scale_set_value_pos(GTK_SCALE(player->volume_scale), GTK_POS_RIGHT);
    
    gtk_box_pack_start(GTK_BOX(volume_hbox), volume_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(volume_hbox), player->volume_scale, TRUE, TRUE, 0);
    
    g_signal_connect(player->window, "destroy", G_CALLBACK(on_window_destroy), player);
    g_signal_connect(player->open_button, "clicked", G_CALLBACK(on_open_clicked), player);
    g_signal_connect(player->play_button, "clicked", G_CALLBACK(on_play_clicked), player);
    g_signal_connect(player->pause_button, "clicked", G_CALLBACK(on_pause_clicked), player);
    g_signal_connect(player->stop_button, "clicked", G_CALLBACK(on_stop_clicked), player);
    g_signal_connect(player->volume_scale, "value-changed", G_CALLBACK(on_volume_changed), player);
    
    return player->window;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    player = g_malloc0(sizeof(MidiPlayer));
    pthread_mutex_init(&player->state_mutex, NULL);
    
    // Initialize OPL for conversion (no SDL audio device needed)
    OPL_Init(SAMPLE_RATE);
    OPL_LoadInstruments();
    
    create_main_window(player);
    update_gui_state(player);
    
    gtk_widget_show_all(player->window);
    
    if (argc > 1) {
        if (load_midi_file(player, argv[1])) {
            printf("Loaded: %s\n", argv[1]);
            update_gui_state(player);
        }
    }
    
    gtk_main();
    
    pthread_mutex_destroy(&player->state_mutex);
    g_free(player);
    
    return 0;
}
