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
#include "audio_player.h"
#include "vfs.h"
#include "icon.h"

extern double playTime;
extern bool isPlaying;
extern bool paused;
extern int globalVolume;
extern void processEvents(void);
extern double playwait;

AudioPlayer *player = NULL;

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
    ofn.lpstrFilter = "All Supported\0*.mid;*.midi;*.wav;*.mp3;*.ogg;*.flac\0"
                      "MIDI Files\0*.mid;*.midi\0"
                      "WAV Files\0*.wav\0"
                      "MP3 Files\0*.mp3\0"
                      "OGG Files\0*.ogg\0"
                      "FLAC Files\0*.flac\0"
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
        
        // Add remove button
        GtkWidget *remove_button = gtk_button_new_with_label("✗");
        gtk_widget_set_size_request(remove_button, 30, 30);
        
        // Store the index in the button data
        g_object_set_data(G_OBJECT(remove_button), "queue_index", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(remove_button), "player", player);
        
        g_signal_connect(remove_button, "clicked", G_CALLBACK(on_remove_from_queue_clicked), NULL);
        
        gtk_box_pack_end(GTK_BOX(box), remove_button, FALSE, FALSE, 0);
        
        gtk_container_add(GTK_CONTAINER(player->queue_listbox), row);
        
        g_free(basename);
    }
    
    // IMPORTANT: Connect the row-activated signal to handle clicks
    // Disconnect any existing signal first to avoid multiple connections
    g_signal_handlers_disconnect_by_func(player->queue_listbox, 
                                         G_CALLBACK(on_queue_item_clicked), 
                                         player);
    
    // Connect the signal for row activation (when user clicks on a row)
    g_signal_connect(player->queue_listbox, "row-activated", 
                     G_CALLBACK(on_queue_item_clicked), player);
    
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

bool init_audio(AudioPlayer *player) {
#ifdef _WIN32
// Try different audio drivers in order of preference
const char* drivers[] = {"wasapi", "directsound", "winmm", NULL};
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
SDL_Init(SDL_INIT_AUDIO);
#endif


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


bool convert_ogg_to_wav(AudioPlayer *player, const char* filename) {
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
    snprintf(virtual_filename, sizeof(virtual_filename), "virtual_ogg_%d.wav", virtual_counter++);
    
    strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting OGG to virtual WAV: %s -> %s\n", filename, virtual_filename);
    
    // Read OGG file into memory
    FILE* ogg_file = fopen(filename, "rb");
    if (!ogg_file) {
        printf("Cannot open OGG file: %s\n", filename);
        return false;
    }
    
    fseek(ogg_file, 0, SEEK_END);
    long ogg_size = ftell(ogg_file);
    fseek(ogg_file, 0, SEEK_SET);
    
    std::vector<uint8_t> ogg_data(ogg_size);
    if (fread(ogg_data.data(), 1, ogg_size, ogg_file) != (size_t)ogg_size) {
        printf("Failed to read OGG file\n");
        fclose(ogg_file);
        return false;
    }
    fclose(ogg_file);
    
    // Convert OGG to WAV in memory
    std::vector<uint8_t> wav_data;
    if (!convertOggToWavInMemory(ogg_data, wav_data)) {
        printf("OGG to WAV conversion failed\n");
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
    
    printf("OGG conversion to virtual file complete\n");
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
                    strcmp(ext_lower, ".ogg") == 0 ||
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
                char full_path[1024];
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
    gtk_file_filter_add_pattern(all_filter, "*.ogg");
    gtk_file_filter_add_pattern(all_filter, "*.flac");
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
    char filename[1024];
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


void on_menu_quit(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    (void)user_data;
    gtk_main_quit();
}

void on_menu_about(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    GtkWidget *about_dialog = gtk_about_dialog_new();
    
    // Set window properties
    gtk_window_set_transient_for(GTK_WINDOW(about_dialog), GTK_WINDOW(player->window));
    gtk_window_set_modal(GTK_WINDOW(about_dialog), TRUE);
    
    // ADD THIS to set the about dialog icon
    GdkPixbuf *logo = load_icon_from_base64();
    if (logo) {
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about_dialog), logo);
        g_object_unref(logo);
    }
    
    // Set about dialog properties
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Music Player");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), "1.0");
    
    // Beautiful description
    const char *description = 
        "A multi-format music player\n\n"
        "Supports MIDI (.mid, .midi) with OPL3 synthesis\n"
        "Supports WAV (.wav) files\n"
        "Supports MP3 (.mp3) files\n"
        "Supports OGG (.ogg) files\n"
        "Supports FLAC (.flac) files\n\n"
        "Features:\n"
        "• Playlist queue with repeat mode\n"
        "• Drag progress slider to seek\n"
        "• << and >> buttons for 5-second rewind/fast-forward\n"
        "• |< and >| buttons for previous/next song\n"
        "• Volume control\n"
        "• GTK interface";
    
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), description);
    
    // Author information
    const char *authors[] = {
        "Jason Hall",
        NULL
    };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
    
    // License information
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_MIT_X11);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), 
                                   "© 2025 Jason Hall\nReleased under MIT License");
    
    // Website
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), 
                                 "https://github.com/jasonbrianhall/midi_msdos");
    gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog), 
                                       "Visit GitHub Repository");
        
    // Show the dialog
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
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
    
    // This will be called after delete-event, so just quit
    gtk_main_quit();
}

void create_main_window(AudioPlayer *player) {
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), "Music Player");
    gtk_window_set_default_size(GTK_WINDOW(player->window), 800, 600);
    gtk_container_set_border_width(GTK_CONTAINER(player->window), 10);
    
    set_window_icon_from_base64(GTK_WINDOW(player->window));
    
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
    player->pause_button = gtk_button_new_with_label("⸸");
    player->stop_button = gtk_button_new_with_label("■");
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
    g_signal_connect(player->window, "delete-event", G_CALLBACK(on_window_delete_event), player);
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

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Initialize virtual filesystem
    init_virtual_filesystem();
    
    player = (AudioPlayer*)g_malloc0(sizeof(AudioPlayer));
    pthread_mutex_init(&player->audio_mutex, NULL);
    
    // Initialize queue and conversion cache
    init_queue(&player->queue);
    init_conversion_cache(&player->conversion_cache);
    
    if (!init_audio(player)) {
        printf("Audio initialization failed\n");
        cleanup_conversion_cache(&player->conversion_cache);
        cleanup_virtual_filesystem();
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
    cleanup_conversion_cache(&player->conversion_cache);
    cleanup_virtual_filesystem();
    pthread_mutex_destroy(&player->audio_mutex);
    g_free(player);
    return 0;
}

