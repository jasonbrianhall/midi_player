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

typedef struct {
    int16_t *data;
    size_t length;
    size_t position;
} AudioBuffer;

typedef struct {
    char **files;           // Array of file paths
    int count;              // Number of files in queue
    int capacity;           // Allocated capacity
    int current_index;      // Currently playing file index
    bool repeat_queue;      // Whether to repeat the entire queue
} PlayQueue;

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

extern double playTime;
extern bool isPlaying;
extern bool paused;
extern int globalVolume;
extern void processEvents(void);
extern double playwait;

AudioPlayer *player = NULL;

// Forward declarations
static void start_playback(AudioPlayer *player);
static void update_gui_state(AudioPlayer *player);
static bool load_file_from_queue(AudioPlayer *player);

// Queue management functions
static void init_queue(PlayQueue *queue) {
    queue->files = NULL;
    queue->count = 0;
    queue->capacity = 0;
    queue->current_index = -1;
    queue->repeat_queue = true;
}

static void clear_queue(PlayQueue *queue) {
    for (int i = 0; i < queue->count; i++) {
        g_free(queue->files[i]);
    }
    g_free(queue->files);
    queue->files = NULL;
    queue->count = 0;
    queue->capacity = 0;
    queue->current_index = -1;
}

static bool add_to_queue(PlayQueue *queue, const char *filename) {
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

static const char* get_current_queue_file(PlayQueue *queue) {
    if (queue->count == 0 || queue->current_index < 0 || queue->current_index >= queue->count) {
        return NULL;
    }
    return queue->files[queue->current_index];
}

static bool advance_queue(PlayQueue *queue) {
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

static bool previous_queue(PlayQueue *queue) {
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

static void update_queue_display(AudioPlayer *player) {
    // Clear existing items
    GList *children = gtk_container_get_children(GTK_CONTAINER(player->queue_listbox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Add queue items
    for (int i = 0; i < player->queue.count; i++) {
        char *basename = g_path_get_basename(player->queue.files[i]);
        
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_container_add(GTK_CONTAINER(row), box);
        
        // Current playing indicator
        const char *indicator = (i == player->queue.current_index) ? "▶ " : "  ";
        GtkWidget *indicator_label = gtk_label_new(indicator);
        gtk_box_pack_start(GTK_BOX(box), indicator_label, FALSE, FALSE, 0);
        
        GtkWidget *filename_label = gtk_label_new(basename);
        gtk_box_pack_start(GTK_BOX(box), filename_label, TRUE, TRUE, 0);
        
        gtk_container_add(GTK_CONTAINER(player->queue_listbox), row);
        
        g_free(basename);
    }
    
    gtk_widget_show_all(player->queue_listbox);
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
    // Create secure temporary file
    char temp_template[] = "/tmp/midi_convert_XXXXXX.wav";
    int temp_fd = mkstemps(temp_template, 4); // 4 for ".wav" suffix
    if (temp_fd == -1) {
        printf("Failed to create temporary file for MIDI conversion\n");
        return false;
    }
    close(temp_fd); // We just need the filename, will reopen later
    
    strncpy(player->temp_wav_file, temp_template, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
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
    cleanup();
    
    printf("Conversion complete: %.2f seconds\n", playTime);
    
    // Reinitialize the main SDL audio system for playback
    printf("Reinitializing SDL audio for playback...\n");
    if (!init_audio(player)) {
        printf("Failed to reinitialize audio for playback\n");
        return false;
    }
    
    return true;
}

static bool convert_mp3_to_wav(AudioPlayer *player, const char* filename) {
    // Create secure temporary file
    char temp_template[] = "/tmp/mp3_convert_XXXXXX.wav";
    int temp_fd = mkstemps(temp_template, 4); // 4 for ".wav" suffix
    if (temp_fd == -1) {
        printf("Failed to create temporary file for MP3 conversion\n");
        return false;
    }
    close(temp_fd); // We just need the filename, will reopen later
    
    strncpy(player->temp_wav_file, temp_template, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting MP3: %s -> %s\n", filename, player->temp_wav_file);
    
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
    
    // Write WAV data to file
    FILE* wav_file = fopen(player->temp_wav_file, "wb");
    if (!wav_file) {
        printf("Cannot create temporary WAV file: %s\n", player->temp_wav_file);
        return false;
    }
    
    if (fwrite(wav_data.data(), 1, wav_data.size(), wav_file) != wav_data.size()) {
        printf("Failed to write WAV file\n");
        fclose(wav_file);
        return false;
    }
    fclose(wav_file);
    
    printf("MP3 conversion complete\n");
    return true;
}

static bool convert_ogg_to_wav(AudioPlayer *player, const char* filename) {
    // Create secure temporary file
    char temp_template[] = "/tmp/ogg_convert_XXXXXX.wav";
    int temp_fd = mkstemps(temp_template, 4); // 4 for ".wav" suffix
    if (temp_fd == -1) {
        printf("Failed to create temporary file for OGG conversion\n");
        return false;
    }
    close(temp_fd); // We just need the filename, will reopen later
    
    strncpy(player->temp_wav_file, temp_template, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting OGG: %s -> %s\n", filename, player->temp_wav_file);
    
    return convertOggToWav(filename, player->temp_wav_file);
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
    player->sample_rate = *(int*)(header + 24);
    player->channels = *(short*)(header + 22);
    player->bits_per_sample = *(short*)(header + 34);
    
    printf("WAV: %d Hz, %d channels, %d bits\n", player->sample_rate, player->channels, player->bits_per_sample);
    
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

static bool load_file(AudioPlayer *player, const char *filename) {
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
        printf("Loading WAV file: %s\n", filename);
        success = load_wav_file(player, filename);
    } else if (strcmp(ext_lower, ".mid") == 0 || strcmp(ext_lower, ".midi") == 0) {
        printf("Loading MIDI file: %s\n", filename);
        if (convert_midi_to_wav(player, filename)) {
            printf("Now loading converted WAV file: %s\n", player->temp_wav_file);
            success = load_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".mp3") == 0) {
        printf("Loading MP3 file: %s\n", filename);
        if (convert_mp3_to_wav(player, filename)) {
            printf("Now loading converted WAV file: %s\n", player->temp_wav_file);
            success = load_wav_file(player, player->temp_wav_file);
        }
    } else if (strcmp(ext_lower, ".ogg") == 0) {
        printf("Loading OGG file: %s\n", filename);
        if (convert_ogg_to_wav(player, filename)) {
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
        
        // Update the progress scale range
        gtk_range_set_range(GTK_RANGE(player->progress_scale), 0.0, player->song_duration);
        gtk_range_set_value(GTK_RANGE(player->progress_scale), 0.0);
        
        // Check if the loaded file has valid audio data
        if (player->audio_buffer.length == 0 || player->song_duration <= 0.1) {
            printf("Warning: File loaded but has no/minimal audio data (duration: %.2f, samples: %zu)\n", 
                   player->song_duration, player->audio_buffer.length);
            printf("Skipping this file and advancing to next...\n");
            
            // Don't auto-start playback for invalid files, let the timer logic handle advancing
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
            
            return true; // Return true since file was "loaded" even if invalid
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

static bool load_file_from_queue(AudioPlayer *player) {
    const char *filename = get_current_queue_file(&player->queue);
    if (!filename) return false;
    
    return load_file(player, filename);
}

static void seek_to_position(AudioPlayer *player, double position_seconds) {
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

static void start_playback(AudioPlayer *player) {
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
            
            // Only check for song completion if we were actually playing
            if (currently_playing && p->audio_buffer.data && p->audio_buffer.length > 0) {
                if (p->audio_buffer.position >= p->audio_buffer.length) {
                    p->is_playing = false;
                    song_finished = true;
                    printf("Song finished naturally (pos: %zu, len: %zu)\n", 
                           p->audio_buffer.position, p->audio_buffer.length);
                }
            }
            
            // Update playback position if playing
            if (p->is_playing && p->audio_buffer.data && p->sample_rate > 0) {
                double samples_per_second = (double)(p->sample_rate * p->channels);
                playTime = (double)p->audio_buffer.position / samples_per_second;
            }
            
            pthread_mutex_unlock(&p->audio_mutex);
            
            // Handle song completion OUTSIDE the mutex lock
            if (song_finished && p->queue.count > 0) {
                printf("Song completed. Attempting to advance queue (current: %d/%d, repeat: %s)\n", 
                       p->queue.current_index + 1, p->queue.count, 
                       p->queue.repeat_queue ? "ON" : "OFF");
                
                // Stop the current timer to prevent race conditions
                p->update_timer_id = 0;
                
                // Try to advance to next song
                if (advance_queue(&p->queue)) {
                    printf("Advanced to next song (now at %d/%d), loading...\n", 
                           p->queue.current_index + 1, p->queue.count);
                    if (load_file_from_queue(p)) {
                        update_queue_display(p);
                        update_gui_state(p);
                        // load_file_from_queue calls load_file which calls start_playback
                        // which will create a new timer, so we return FALSE here
                        return FALSE;
                    } else {
                        printf("Failed to load next song\n");
                        update_gui_state(p);
                        return FALSE;
                    }
                } else {
                    printf("Queue advance failed - at end with no repeat\n");
                    update_gui_state(p);
                    return FALSE;
                }
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
    
    gtk_range_set_value(GTK_RANGE(player->progress_scale), 0.0);
    gtk_label_set_text(GTK_LABEL(player->time_label), "00:00 / 00:00");
    gtk_button_set_label(GTK_BUTTON(player->pause_button), "⏸");
}

static void rewind_5_seconds(AudioPlayer *player) {
    if (!player->is_loaded) return;
    
    double current_time = playTime;
    double new_time = current_time - 5.0;
    if (new_time < 0) new_time = 0;
    
    seek_to_position(player, new_time);
    gtk_range_set_value(GTK_RANGE(player->progress_scale), new_time);
    
    printf("Rewinded 5 seconds to %.2f\n", new_time);
}

static void fast_forward_5_seconds(AudioPlayer *player) {
    if (!player->is_loaded) return;
    
    double current_time = playTime;
    double new_time = current_time + 5.0;
    if (new_time > player->song_duration) new_time = player->song_duration;
    
    seek_to_position(player, new_time);
    gtk_range_set_value(GTK_RANGE(player->progress_scale), new_time);
    
    printf("Fast forwarded 5 seconds to %.2f\n", new_time);
}

static void next_song(AudioPlayer *player) {
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

static void previous_song(AudioPlayer *player) {
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

static void update_gui_state(AudioPlayer *player) {
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
static void on_progress_scale_value_changed(GtkRange *range, gpointer user_data) {
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
static void on_add_to_queue_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
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
    gtk_file_filter_add_pattern(all_filter, "*.ogg");
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
}


static void on_clear_queue_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    stop_playback(player);
    clear_queue(&player->queue);
    update_queue_display(player);
    update_gui_state(player);
    player->is_loaded = false;
    
    gtk_label_set_text(GTK_LABEL(player->file_label), "No file loaded");
}

static void on_repeat_queue_toggled(GtkToggleButton *button, gpointer user_data) {
    AudioPlayer *player = (AudioPlayer*)user_data;
    player->queue.repeat_queue = gtk_toggle_button_get_active(button);
    
    printf("Queue repeat: %s\n", player->queue.repeat_queue ? "ON" : "OFF");
}

// Menu callbacks
static void on_menu_open(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
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
    gtk_file_filter_add_pattern(all_filter, "*.mp3");
    gtk_file_filter_add_pattern(all_filter, "*.ogg");
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
}


static void on_menu_quit(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    gtk_main_quit();
}

static void on_menu_about(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkWidget *about_dialog = gtk_message_dialog_new(GTK_WINDOW(player->window),
                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_INFO,
                                                     GTK_BUTTONS_CLOSE,
                                                     "GTK Music Player\n\nSupports MIDI (.mid, .midi), WAV (.wav), MP3 (.mp3), and OGG (.ogg) files.\nMIDI files are converted to WAV using OPL3 synthesis.\nMP3 and OGG files are decoded and converted to WAV.\n\nFeatures:\n- Playlist queue with repeat\n- Drag the progress slider to seek\n- Use << and >> buttons for 5-second rewind/fast-forward\n- Use |< and >| buttons for previous/next song");
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
}

// Button callbacks
static void on_play_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    start_playback((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

static void on_pause_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    toggle_pause((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    stop_playback((AudioPlayer*)user_data);
    update_gui_state((AudioPlayer*)user_data);
}

static void on_rewind_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    rewind_5_seconds((AudioPlayer*)user_data);
}

static void on_fast_forward_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    fast_forward_5_seconds((AudioPlayer*)user_data);
}

static void on_next_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    next_song((AudioPlayer*)user_data);
}

static void on_previous_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    previous_song((AudioPlayer*)user_data);
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double value = gtk_range_get_value(range);
    globalVolume = (int)(value * 100);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    stop_playback(player);
    clear_queue(&player->queue);
    
    // Clean up temporary file if it exists
    if (strlen(player->temp_wav_file) > 0) {
        if (unlink(player->temp_wav_file) == 0) {
            printf("Cleaned up temporary file: %s\n", player->temp_wav_file);
        }
    }
    
    if (player->audio_buffer.data) free(player->audio_buffer.data);
    if (player->audio_device) SDL_CloseAudioDevice(player->audio_device);
    
    SDL_Quit();
    gtk_main_quit();
}

static void create_main_window(AudioPlayer *player) {
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), "GTK Music Player");
    gtk_window_set_default_size(GTK_WINDOW(player->window), 800, 600);
    gtk_container_set_border_width(GTK_CONTAINER(player->window), 10);
    
    // Main hbox to split player controls and queue
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(player->window), main_hbox);
    
    // Player controls vbox (left side)
    GtkWidget *player_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(player_vbox, 400, -1);
    gtk_box_pack_start(GTK_BOX(main_hbox), player_vbox, FALSE, FALSE, 0);
    
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
    
    gtk_box_pack_start(GTK_BOX(player_vbox), menubar, FALSE, FALSE, 0);
    
    // Content area
    GtkWidget *content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 10);
    gtk_box_pack_start(GTK_BOX(player_vbox), content_vbox, TRUE, TRUE, 0);
    
    player->file_label = gtk_label_new("No file loaded");
    gtk_box_pack_start(GTK_BOX(content_vbox), player->file_label, FALSE, FALSE, 0);
    
    // Progress scale
    player->progress_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 0.1);
    gtk_scale_set_draw_value(GTK_SCALE(player->progress_scale), FALSE);
    gtk_widget_set_sensitive(player->progress_scale, FALSE);
    g_signal_connect(player->progress_scale, "value-changed", G_CALLBACK(on_progress_scale_value_changed), player);
    gtk_box_pack_start(GTK_BOX(content_vbox), player->progress_scale, FALSE, FALSE, 0);
    
    player->time_label = gtk_label_new("00:00 / 00:00");
    gtk_box_pack_start(GTK_BOX(content_vbox), player->time_label, FALSE, FALSE, 0);
    
    // Navigation buttons
    GtkWidget *nav_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(nav_button_box), TRUE);
    gtk_box_pack_start(GTK_BOX(content_vbox), nav_button_box, FALSE, FALSE, 0);
    
    player->prev_button = gtk_button_new_with_label("|◀");
    player->rewind_button = gtk_button_new_with_label("◀◀ 5s");
    player->play_button = gtk_button_new_with_label("▶");
    player->pause_button = gtk_button_new_with_label("⏸");
    player->stop_button = gtk_button_new_with_label("⏹");
    player->fast_forward_button = gtk_button_new_with_label("5s ▶▶");
    player->next_button = gtk_button_new_with_label("▶|");
    
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->prev_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->rewind_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->play_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->pause_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->stop_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->fast_forward_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nav_button_box), player->next_button, TRUE, TRUE, 0);
    
    GtkWidget *volume_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(content_vbox), volume_box, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Volume:");
    player->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
    gtk_range_set_value(GTK_RANGE(player->volume_scale), (double)globalVolume / 100.0);
    
    gtk_box_pack_start(GTK_BOX(volume_box), volume_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(volume_box), player->volume_scale, TRUE, TRUE, 0);
    
    // Queue control buttons
    GtkWidget *queue_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(content_vbox), queue_button_box, FALSE, FALSE, 0);
    
    player->add_to_queue_button = gtk_button_new_with_label("Add to Queue");
    player->clear_queue_button = gtk_button_new_with_label("Clear Queue");
    player->repeat_queue_button = gtk_check_button_new_with_label("Repeat Queue");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(player->repeat_queue_button), TRUE);
    
    gtk_box_pack_start(GTK_BOX(queue_button_box), player->add_to_queue_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(queue_button_box), player->clear_queue_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(queue_button_box), player->repeat_queue_button, TRUE, TRUE, 0);
    
    // Queue display (right side)
    GtkWidget *queue_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(main_hbox), queue_vbox, TRUE, TRUE, 0);
    
    GtkWidget *queue_label = gtk_label_new("Queue:");
    gtk_widget_set_halign(queue_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(queue_vbox), queue_label, FALSE, FALSE, 0);
    
    player->queue_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(player->queue_scrolled_window), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(player->queue_scrolled_window, 300, 400);
    
    player->queue_listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(player->queue_scrolled_window), player->queue_listbox);
    gtk_box_pack_start(GTK_BOX(queue_vbox), player->queue_scrolled_window, TRUE, TRUE, 0);
    
    // Connect signals
    g_signal_connect(player->window, "destroy", G_CALLBACK(on_window_destroy), player);
    g_signal_connect(player->play_button, "clicked", G_CALLBACK(on_play_clicked), player);
    g_signal_connect(player->pause_button, "clicked", G_CALLBACK(on_pause_clicked), player);
    g_signal_connect(player->stop_button, "clicked", G_CALLBACK(on_stop_clicked), player);
    g_signal_connect(player->rewind_button, "clicked", G_CALLBACK(on_rewind_clicked), player);
    g_signal_connect(player->fast_forward_button, "clicked", G_CALLBACK(on_fast_forward_clicked), player);
    g_signal_connect(player->next_button, "clicked", G_CALLBACK(on_next_clicked), player);
    g_signal_connect(player->prev_button, "clicked", G_CALLBACK(on_previous_clicked), player);
    g_signal_connect(player->volume_scale, "value-changed", G_CALLBACK(on_volume_changed), player);
    g_signal_connect(player->add_to_queue_button, "clicked", G_CALLBACK(on_add_to_queue_clicked), player);
    g_signal_connect(player->clear_queue_button, "clicked", G_CALLBACK(on_clear_queue_clicked), player);
    g_signal_connect(player->repeat_queue_button, "toggled", G_CALLBACK(on_repeat_queue_toggled), player);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    player = (AudioPlayer*)g_malloc0(sizeof(AudioPlayer));
    pthread_mutex_init(&player->audio_mutex, NULL);
    
    // Initialize queue
    init_queue(&player->queue);
    
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
        // Add all command line arguments to queue
        for (int i = 1; i < argc; i++) {
            add_to_queue(&player->queue, argv[i]);
        }
        
        if (load_file_from_queue(player)) {
            printf("Loaded and auto-starting: %s\n", argv[1]);
            update_queue_display(player);
            update_gui_state(player);
            // load_file now auto-starts playback
        }
    }
    
    gtk_main();
    
    clear_queue(&player->queue);
    pthread_mutex_destroy(&player->audio_mutex);
    g_free(player);
    return 0;
}
