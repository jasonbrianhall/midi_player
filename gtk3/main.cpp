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
#include "midiplayer.h"
#include "dbopl_wrapper.h"
#include "wav_converter.h"

typedef struct {
    int16_t *data;
    size_t length;
    size_t position;
} AudioBuffer;

typedef struct {
    GtkWidget *window;
    GtkWidget *play_button;
    GtkWidget *pause_button;
    GtkWidget *stop_button;
    GtkWidget *progress_bar;
    GtkWidget *time_label;
    GtkWidget *volume_scale;
    GtkWidget *file_label;
    
    bool is_loaded;
    bool is_playing;
    bool is_paused;
    char current_file[512];
    char temp_wav_file[512];
    double song_duration;
    guint update_timer_id;
    
    AudioBuffer audio_buffer;
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_spec;
    pthread_mutex_t audio_mutex;
} AudioPlayer;

extern double playTime;
extern bool isPlaying;
extern bool paused;
extern int globalVolume;
extern void processEvents(void);
extern double playwait;

AudioPlayer *player = NULL;

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
    int samples_to_copy = (samples_requested < (int)samples_remaining) ? samples_requested : (int)samples_remaining;
    
    if (samples_to_copy > 0) {
        for (int i = 0; i < samples_to_copy; i++) {
            int32_t sample = player->audio_buffer.data[player->audio_buffer.position + i];
            sample = (sample * globalVolume) / 100;
            if (sample > 32767) sample = 32767;
            else if (sample < -32768) sample = -32768;
            output[i] = (int16_t)sample;
        }
        
        player->audio_buffer.position += samples_to_copy;
        
        if (player->audio_buffer.position >= player->audio_buffer.length) {
            player->is_playing = false;
        }
    }
    
    pthread_mutex_unlock(&player->audio_mutex);
}

static bool init_audio(AudioPlayer *player) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = AUDIO_CHANNELS;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = player;
    
    player->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &player->audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (player->audio_device == 0) {
        printf("Audio device open failed: %s\n", SDL_GetError());
        return false;
    }
    
    printf("Audio: %d Hz, %d channels\n", player->audio_spec.freq, player->audio_spec.channels);
    return true;
}

static bool convert_midi_to_wav(AudioPlayer *player, const char* filename) {
    char *basename = g_path_get_basename(filename);
    char *dot = strrchr(basename, '.');
    if (dot) *dot = '\0';
    snprintf(player->temp_wav_file, sizeof(player->temp_wav_file), "/tmp/%s_converted.wav", basename);
    g_free(basename);
    
    printf("Converting MIDI: %s -> %s\n", filename, player->temp_wav_file);
    
    // Temporarily shut down the main SDL audio system
    if (player->audio_device) {
        SDL_CloseAudioDevice(player->audio_device);
        player->audio_device = 0;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    
    playTime = 0;
    isPlaying = true;
    
    if (!initSDL()) {
        printf("SDL init for conversion failed\n");
        return false;
    }
    
    if (!loadMidiFile(filename)) {
        printf("MIDI file load failed\n");
        cleanup();
        return false;
    }
    
    WAVConverter* wav_converter = wav_converter_init(player->temp_wav_file, SAMPLE_RATE, AUDIO_CHANNELS);
    if (!wav_converter) {
        printf("WAV converter init failed\n");
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
            printf("WAV write failed\n");
            break;
        }
        
        playTime += buffer_duration;
        playwait -= buffer_duration;
        
        while (playwait <= 0 && isPlaying) {
            processEvents();
        }
        
        if (((int)playTime) % 10 == 0) {
            static int last_reported = -1;
            if ((int)playTime != last_reported) {
                printf("Converting... %d seconds\n", (int)playTime);
                last_reported = (int)playTime;
            }
        }
    }
    
    wav_converter_finish(wav_converter);
    wav_converter_free(wav_converter);
    cleanup(); // This closes the conversion SDL context
    
    printf("Conversion complete: %.2f seconds\n", playTime);
    
    // Reinitialize the main SDL audio system for playback
    printf("Reinitializing SDL audio for playback...\n");
    if (!init_audio(player)) {
        printf("Failed to reinitialize audio for playback\n");
        return false;
    }
    
    return true;
}

static bool load_wav_file(AudioPlayer *player, const char* wav_path) {
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
    int sample_rate = *(int*)(header + 24);
    int channels = *(short*)(header + 22);
    int bits_per_sample = *(short*)(header + 34);
    
    printf("WAV: %d Hz, %d channels, %d bits\n", sample_rate, channels, bits_per_sample);
    
    // Get file size and calculate duration
    fseek(wav_file, 0, SEEK_END);
    long file_size = ftell(wav_file);
    long data_size = file_size - 44;
    
    player->song_duration = data_size / (double)(sample_rate * channels * (bits_per_sample / 8));
    printf("WAV duration: %.2f seconds\n", player->song_duration);
    
    // Allocate and read audio data
    int16_t* wav_data = malloc(data_size);
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

static bool load_file(AudioPlayer *player, const char *filename) {
    // Stop current playback
    if (player->is_playing) {
        pthread_mutex_lock(&player->audio_mutex);
        player->is_playing = false;
        player->is_paused = false;
        SDL_PauseAudioDevice(player->audio_device, 1);
        pthread_mutex_unlock(&player->audio_mutex);
        
        if (player->update_timer_id > 0) {
            g_source_remove(player->update_timer_id);
            player->update_timer_id = 0;
        }
    }
    
    // Determine file type
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
        // Load WAV file directly
        printf("Loading WAV file: %s\n", filename);
        success = load_wav_file(player, filename);
    } else if (strcmp(ext_lower, ".mid") == 0 || strcmp(ext_lower, ".midi") == 0) {
        // Convert MIDI to WAV, then load the converted WAV
        printf("Loading MIDI file: %s\n", filename);
        if (convert_midi_to_wav(player, filename)) {
            printf("Now loading converted WAV file: %s\n", player->temp_wav_file);
            success = load_wav_file(player, player->temp_wav_file);
        }
    } else {
        printf("Unsupported file type: %s\n", ext);
        return false;
    }
    
    if (success) {
        strcpy(player->current_file, filename);
        player->is_loaded = true;
        player->is_playing = false;
        player->is_paused = false;
        playTime = 0;
        printf("File successfully loaded and ready for playback\n");
    }
    
    return success;
}

static void start_playback(AudioPlayer *player) {
    if (!player->is_loaded || !player->audio_buffer.data) {
        printf("Cannot start playback - no audio data loaded\n");
        return;
    }
    
    printf("Starting WAV playback\n");
    
    pthread_mutex_lock(&player->audio_mutex);
    if (player->audio_buffer.position >= player->audio_buffer.length) {
        player->audio_buffer.position = 0;
    }
    player->is_playing = true;
    player->is_paused = false;
    pthread_mutex_unlock(&player->audio_mutex);
    
    SDL_PauseAudioDevice(player->audio_device, 0);
    
    if (player->update_timer_id == 0) {
        player->update_timer_id = g_timeout_add(100, (GSourceFunc)([](gpointer data) -> gboolean {
            AudioPlayer *p = (AudioPlayer*)data;
            
            pthread_mutex_lock(&p->audio_mutex);
            if (!p->is_playing) {
                pthread_mutex_unlock(&p->audio_mutex);
                p->update_timer_id = 0;
                return FALSE;
            }
            
            if (p->audio_buffer.data && p->audio_spec.freq > 0) {
                double samples_per_second = (double)(p->audio_spec.freq * p->audio_spec.channels);
                playTime = (double)p->audio_buffer.position / samples_per_second;
            }
            pthread_mutex_unlock(&p->audio_mutex);
            
            // Update progress bar
            double progress = (p->song_duration > 0) ? playTime / p->song_duration : 0.0;
            if (progress > 1.0) progress = 1.0;
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(p->progress_bar), progress);
            
            // Update time label
            int min = (int)(playTime / 60);
            int sec = (int)playTime % 60;
            int total_min = (int)(p->song_duration / 60);
            int total_sec = (int)p->song_duration % 60;
            
            char time_text[64];
            snprintf(time_text, sizeof(time_text), "%02d:%02d / %02d:%02d", min, sec, total_min, total_sec);
            gtk_label_set_text(GTK_LABEL(p->time_label), time_text);
            
            return TRUE;
        }), player);
    }
}

static void toggle_pause(AudioPlayer *player) {
    if (!player->is_playing) return;
    
    pthread_mutex_lock(&player->audio_mutex);
    player->is_paused = !player->is_paused;
    
    if (player->is_paused) {
        SDL_PauseAudioDevice(player->audio_device, 1);
    } else {
        SDL_PauseAudioDevice(player->audio_device, 0);
    }
    pthread_mutex_unlock(&player->audio_mutex);
    
    gtk_button_set_label(GTK_BUTTON(player->pause_button), player->is_paused ? "Resume" : "⏸");
}

static void stop_playback(AudioPlayer *player) {
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
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(player->progress_bar), 0.0);
    gtk_label_set_text(GTK_LABEL(player->time_label), "00:00 / 00:00");
    gtk_button_set_label(GTK_BUTTON(player->pause_button), "⏸");
}

static void update_gui_state(AudioPlayer *player) {
    gtk_widget_set_sensitive(player->play_button, player->is_loaded && !player->is_playing);
    gtk_widget_set_sensitive(player->pause_button, player->is_playing);
    gtk_widget_set_sensitive(player->stop_button, player->is_playing || player->is_paused);
    
    if (player->is_loaded) {
        char *basename = g_path_get_basename(player->current_file);
        char label_text[512];
        snprintf(label_text, sizeof(label_text), "File: %s (%.1f sec)", basename, player->song_duration);
        gtk_label_set_text(GTK_LABEL(player->file_label), label_text);
        g_free(basename);
    } else {
        gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
    }
}

// Menu callbacks
static void on_menu_open(GtkMenuItem *menuitem, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
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
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (load_file(player, filename)) {
            printf("Successfully loaded: %s\n", filename);
            update_gui_state(player);
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
}

static void on_menu_quit(GtkMenuItem *menuitem, gpointer user_data) {
    gtk_main_quit();
}

static void on_menu_about(GtkMenuItem *menuitem, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkWidget *about_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_INFO,
                                                     GTK_BUTTONS_CLOSE,
                                                     "GTK Media Player\n\nSupports MIDI (.mid, .midi) and WAV (.wav) files.\nMIDI files are converted to WAV using OPL3 synthesis.");
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
}

// Button callbacks
static void on_play_clicked(GtkButton *button, gpointer user_data) {
    start_playback((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

static void on_pause_clicked(GtkButton *button, gpointer user_data) {
    toggle_pause((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
    stop_playback((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    double value = gtk_range_get_value(range);
    globalVolume = (int)(value * 100);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    stop_playback(player);
    
    if (player->audio_buffer.data) free(player->audio_buffer.data);
    if (player->audio_device) SDL_CloseAudioDevice(player->audio_device);
    
    SDL_Quit();
    gtk_main_quit();
}

static void create_main_window(AudioPlayer *player) {
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), "GTK Media Player");
    gtk_window_set_default_size(GTK_WINDOW(player->window), 500, 250);
    gtk_container_set_border_width(GTK_CONTAINER(player->window), 10);
    
    // Main vbox
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(player->window), main_vbox);
    
    // Menu bar
    GtkWidget *menubar = gtk_menu_bar_new();
    
    // File menu
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
    
    GtkWidget *open_item = gtk_menu_item_new_with_label("Open...");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    g_signal_connect(open_item, "activate", G_CALLBACK(on_menu_open), player);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_menu_quit), player);
    
    // Help menu
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);
    
    GtkWidget *about_item = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);
    g_signal_connect(about_item, "activate", G_CALLBACK(on_menu_about), player);
    
    gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
    
    // Content area
    GtkWidget *content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), content_vbox, TRUE, TRUE, 0);
    
    player->file_label = gtk_label_new("No file loaded");
    gtk_box_pack_start(GTK_BOX(content_vbox), player->file_label, FALSE, FALSE, 0);
    
    player->progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(content_vbox), player->progress_bar, FALSE, FALSE, 0);
    
    player->time_label = gtk_label_new("00:00 / 00:00");
    gtk_box_pack_start(GTK_BOX(content_vbox), player->time_label, FALSE, FALSE, 0);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(button_box), TRUE);
    gtk_box_pack_start(GTK_BOX(content_vbox), button_box, FALSE, FALSE, 0);
    
    player->play_button = gtk_button_new_with_label("▶");
    player->pause_button = gtk_button_new_with_label("⏸");
    player->stop_button = gtk_button_new_with_label("⏹");
    
    gtk_box_pack_start(GTK_BOX(button_box), player->play_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), player->pause_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), player->stop_button, TRUE, TRUE, 0);
    
    GtkWidget *volume_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(content_vbox), volume_box, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Volume:");
    player->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
    gtk_range_set_value(GTK_RANGE(player->volume_scale), (double)globalVolume / 100.0);
    
    gtk_box_pack_start(GTK_BOX(volume_box), volume_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(volume_box), player->volume_scale, TRUE, TRUE, 0);
    
    // Connect signals
    g_signal_connect(player->window, "destroy", G_CALLBACK(on_window_destroy), player);
    g_signal_connect(player->play_button, "clicked", G_CALLBACK(on_play_clicked), player);
    g_signal_connect(player->pause_button, "clicked", G_CALLBACK(on_pause_clicked), player);
    g_signal_connect(player->stop_button, "clicked", G_CALLBACK(on_stop_clicked), player);
    g_signal_connect(player->volume_scale, "value-changed", G_CALLBACK(on_volume_changed), player);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    player = g_malloc0(sizeof(AudioPlayer));
    pthread_mutex_init(&player->audio_mutex, NULL);
    
    if (!init_audio(player)) {
        printf("Audio initialization failed\n");
        return 1;
    }
    
    // Initialize OPL for MIDI conversion
    OPL_Init(SAMPLE_RATE);
    OPL_LoadInstruments();
    
    create_main_window(player);
    update_gui_state(player);
    gtk_widget_show_all(player->window);
    
    if (argc > 1) {
        if (load_file(player, argv[1])) {
            printf("Loaded: %s\n", argv[1]);
            update_gui_state(player);
        }
    }
    
    gtk_main();
    
    pthread_mutex_destroy(&player->audio_mutex);
    g_free(player);
    return 0;
}
