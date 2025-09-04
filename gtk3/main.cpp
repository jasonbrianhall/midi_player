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
#include <SDL2/SDL.h>
#include "midiplayer.h"
#include "dbopl_wrapper.h"
#include "wav_converter.h"

// Audio buffer for WAV playback
typedef struct {
    int16_t *data;
    size_t length;
    size_t position;
    bool loop;
} AudioBuffer;

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
    
    // Audio data
    AudioBuffer audio_buffer;
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_spec;
    
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

// SDL Audio callback for WAV playback
void wav_audio_callback(void* userdata, Uint8* stream, int len) {
    MidiPlayer* player = (MidiPlayer*)userdata;
    
    // Clear the buffer
    memset(stream, 0, len);
    
    if (!player->is_playing || player->is_paused || !player->audio_buffer.data) {
        return;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    
    int16_t* output = (int16_t*)stream;
    int samples_requested = len / sizeof(int16_t);
    size_t samples_remaining = player->audio_buffer.length - player->audio_buffer.position;
    int samples_to_copy = (samples_requested < (int)samples_remaining) ? samples_requested : (int)samples_remaining;
    
    if (samples_to_copy > 0) {
        // Apply volume scaling
        for (int i = 0; i < samples_to_copy; i++) {
            int32_t sample = player->audio_buffer.data[player->audio_buffer.position + i];
            sample = (sample * globalVolume) / 100;
            
            // Clamp to 16-bit range
            if (sample > 32767) sample = 32767;
            else if (sample < -32768) sample = -32768;
            
            output[i] = (int16_t)sample;
        }
        
        player->audio_buffer.position += samples_to_copy;
        
        // Update play time based on actual audio format
        double samples_per_second = (double)(player->audio_spec.freq * player->audio_spec.channels);
        playTime = (double)player->audio_buffer.position / samples_per_second;
    }
    
    // Check if we've reached the end
    if (player->audio_buffer.position >= player->audio_buffer.length) {
        player->is_playing = false;
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Initialize MIDI player
static bool init_midi_player(MidiPlayer *player_ctx) {
    // Initialize SDL Audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Setup audio spec for WAV playback
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16;
    want.channels = AUDIO_CHANNELS;
    want.samples = AUDIO_BUFFER;
    want.callback = wav_audio_callback;
    want.userdata = player_ctx;
    
    player_ctx->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &player_ctx->audio_spec, 0);
    if (player_ctx->audio_device == 0) {
        printf("Failed to open audio: %s\n", SDL_GetError());
        return false;
    }
    
    // Initialize OPL for conversion
    OPL_Init(SAMPLE_RATE);
    OPL_LoadInstruments();
    
    // Initialize audio buffer
    player_ctx->audio_buffer.data = NULL;
    player_ctx->audio_buffer.length = 0;
    player_ctx->audio_buffer.position = 0;
    player_ctx->audio_buffer.loop = false;
    
    return true;
}

// Convert MIDI to WAV file and load it
static bool convert_and_load_midi(MidiPlayer *player, const char* filename) {
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
    
    // Load WAV file into memory
    FILE* wav_file = fopen(player->temp_wav_file, "rb");
    if (!wav_file) {
        printf("Failed to open WAV file: %s\n", player->temp_wav_file);
        return false;
    }
    
    fseek(wav_file, 0, SEEK_END);
    long file_size = ftell(wav_file);
    fseek(wav_file, 44, SEEK_SET); // Skip WAV header
    
    long data_size = file_size - 44;
    int16_t* wav_data = malloc(data_size);
    if (!wav_data) {
        printf("Failed to allocate memory for WAV data\n");
        fclose(wav_file);
        return false;
    }
    
    fread(wav_data, 1, data_size, wav_file);
    fclose(wav_file);
    
    // Store in audio buffer
    if (player->audio_buffer.data) {
        free(player->audio_buffer.data);
    }
    
    player->audio_buffer.data = wav_data;
    player->audio_buffer.length = data_size / sizeof(int16_t);
    player->audio_buffer.position = 0;
    player->song_duration = playTime;
    
    printf("Loaded %zu samples into memory\n", player->audio_buffer.length);
    return true;
}

// Load MIDI file
static bool load_midi_file(MidiPlayer *player, const char *filename) {
    pthread_mutex_lock(&player->state_mutex);
    
    // Stop current playback
    if (player->is_playing) {
        player->is_playing = false;
        SDL_PauseAudioDevice(player->audio_device, 1);
    }
    
    // Convert and load
    if (!convert_and_load_midi(player, filename)) {
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

// Start playback
static void start_playback(MidiPlayer *player) {
    if (!player->is_loaded || !player->audio_buffer.data) return;
    
    pthread_mutex_lock(&player->state_mutex);
    
    player->audio_buffer.position = 0;
    player->is_playing = true;
    player->is_paused = false;
    playTime = 0;
    
    SDL_PauseAudioDevice(player->audio_device, 0);
    
    if (player->update_timer_id == 0) {
        player->update_timer_id = g_timeout_add(100, update_progress, player);
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Pause/resume playback
static void toggle_pause(MidiPlayer *player) {
    if (!player->is_playing) return;
    
    pthread_mutex_lock(&player->state_mutex);
    
    player->is_paused = !player->is_paused;
    
    if (player->is_paused) {
        SDL_PauseAudioDevice(player->audio_device, 1);
    } else {
        SDL_PauseAudioDevice(player->audio_device, 0);
    }
    
    pthread_mutex_unlock(&player->state_mutex);
}

// Stop playback
static void stop_playback(MidiPlayer *player) {
    pthread_mutex_lock(&player->state_mutex);
    
    player->is_playing = false;
    player->is_paused = false;
    player->audio_buffer.position = 0;
    playTime = 0;
    
    SDL_PauseAudioDevice(player->audio_device, 1);
    
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
    
    size_t new_position = (size_t)(position * player->audio_buffer.length);
    player->audio_buffer.position = new_position;
    
    double samples_per_second = (double)(player->audio_spec.freq * player->audio_spec.channels);
    playTime = (double)player->audio_buffer.position / samples_per_second;
    
    pthread_mutex_unlock(&player->state_mutex);
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
    
    if (!player->is_playing) {
        update_gui_state(player);
        pthread_mutex_unlock(&player->state_mutex);
        player->update_timer_id = 0;
        return FALSE;
    }
    
    double progress = 0.0;
    if (player->song_duration > 0) {
        progress = playTime / player->song_duration;
        if (progress > 1.0) progress = 1.0;
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar), progress);
    
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
}

static void on_progress_seek(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    if (!player->is_loaded) return;
    
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    double position = event->x / allocation.width;
    seek_to_position(player, position);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    MidiPlayer *player = (MidiPlayer*)user_data;
    
    stop_playback(player);
    
    if (player->audio_buffer.data) {
        free(player->audio_buffer.data);
    }
    
    if (player->audio_device) {
        SDL_CloseAudioDevice(player->audio_device);
    }
    
    SDL_Quit();
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
    g_signal_connect(player->progress_bar, "button-press-event", G_CALLBACK(on_progress_seek), player);
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
    
    if (!init_midi_player(player)) {
        printf("Failed to initialize MIDI player\n");
        return 1;
    }
    
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
