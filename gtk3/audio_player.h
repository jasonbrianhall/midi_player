#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

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
#include "audioconverter.h"
#include "convertoggtowav.h"

// Audio buffer structure
typedef struct {
    int16_t *data;
    size_t length;
    size_t position;
} AudioBuffer;

// Play queue structure
typedef struct {
    char **files;           // Array of file paths
    int count;              // Number of files in queue
    int capacity;           // Allocated capacity
    int current_index;      // Currently playing file index
    bool repeat_queue;      // Whether to repeat the entire queue
} PlayQueue;

// Main audio player structure
typedef struct {
    GtkWidget *window;
    GtkWidget *play_button;
    GtkWidget *pause_button;
    GtkWidget *stop_button;
    GtkWidget *rewind_button;
    GtkWidget *fast_forward_button;
    GtkWidget *progress_scale;
    GtkWidget *time_label;
    GtkWidget *volume_scale;
    GtkWidget *file_label;
    
    // Queue widgets
    GtkWidget *queue_scrolled_window;
    GtkWidget *queue_listbox;
    GtkWidget *add_to_queue_button;
    GtkWidget *clear_queue_button;
    GtkWidget *repeat_queue_button;
    GtkWidget *next_button;
    GtkWidget *prev_button;
    
    PlayQueue queue;
    
    bool is_loaded;
    bool is_playing;
    bool is_paused;
    bool seeking;
    char current_file[512];
    char temp_wav_file[512];
    double song_duration;
    guint update_timer_id;
    
    AudioBuffer audio_buffer;
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_spec;
    pthread_mutex_t audio_mutex;
    
    // Audio format info for seeking calculations
    int sample_rate;
    int channels;
    int bits_per_sample;
} AudioPlayer;

// External variables from midiplayer
extern double playTime;
extern bool isPlaying;
extern bool paused;
extern int globalVolume;
extern double playwait;

// External functions from midiplayer
extern void processEvents(void);

// Global player instance
extern AudioPlayer *player;

// Queue management functions
void init_queue(PlayQueue *queue);
void clear_queue(PlayQueue *queue);
bool add_to_queue(PlayQueue *queue, const char *filename);
const char* get_current_queue_file(PlayQueue *queue);
bool advance_queue(PlayQueue *queue);
bool previous_queue(PlayQueue *queue);
void update_queue_display(AudioPlayer *player);

// Audio functions
void audio_callback(void* userdata, Uint8* stream, int len);
bool init_audio(AudioPlayer *player);

// File conversion functions
bool convert_midi_to_wav(AudioPlayer *player, const char* filename);
bool convert_mp3_to_wav(AudioPlayer *player, const char* filename);
bool convert_ogg_to_wav(AudioPlayer *player, const char* filename);
bool load_wav_file(AudioPlayer *player, const char* wav_path);
bool load_file(AudioPlayer *player, const char *filename);
bool load_file_from_queue(AudioPlayer *player);

// Playback control functions
void seek_to_position(AudioPlayer *player, double position_seconds);
void start_playback(AudioPlayer *player);
void toggle_pause(AudioPlayer *player);
void stop_playback(AudioPlayer *player);
void rewind_5_seconds(AudioPlayer *player);
void fast_forward_5_seconds(AudioPlayer *player);
void next_song(AudioPlayer *player);
void previous_song(AudioPlayer *player);
void update_gui_state(AudioPlayer *player);

// GUI callback functions
void on_progress_scale_value_changed(GtkRange *range, gpointer user_data);
void on_add_to_queue_clicked(GtkButton *button, gpointer user_data);
void on_clear_queue_clicked(GtkButton *button, gpointer user_data);
void on_repeat_queue_toggled(GtkToggleButton *button, gpointer user_data);
void on_menu_open(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_quit(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_about(GtkMenuItem *menuitem, gpointer user_data);
void on_play_clicked(GtkButton *button, gpointer user_data);
void on_pause_clicked(GtkButton *button, gpointer user_data);
void on_stop_clicked(GtkButton *button, gpointer user_data);
void on_rewind_clicked(GtkButton *button, gpointer user_data);
void on_fast_forward_clicked(GtkButton *button, gpointer user_data);
void on_next_clicked(GtkButton *button, gpointer user_data);
void on_previous_clicked(GtkButton *button, gpointer user_data);
void on_volume_changed(GtkRange *range, gpointer user_data);
void on_window_destroy(GtkWidget *widget, gpointer user_data);

// GUI creation function
void create_main_window(AudioPlayer *player);

#endif // AUDIO_PLAYER_H
