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


// Helper functions for layout management
static void calculate_layout_config(LayoutManager *layout) {
    // Get screen info
    GdkScreen *screen = gdk_screen_get_default();
    int screen_width = gdk_screen_get_width(screen);
    int screen_height = gdk_screen_get_height(screen);
    
    // Determine if we should use compact layout
    layout->config.is_compact = (screen_width <= 1024);
    
    // Adaptive base sizes based on screen resolution category
    int base_window_width, base_window_height, base_player_width;
    int base_vis_width, base_vis_height, base_queue_width, base_queue_height;
    
    if (screen_width <= 800 || screen_height <= 600) {
        // Very small screens
        base_window_width = 750;
        base_window_height = 550;
        base_player_width = 350;
        base_vis_width = 200;
        base_vis_height = 80;
        base_queue_width = 200;
        base_queue_height = 300;
    } else if (screen_width < 1200 || screen_height < 900) {
        // Medium screens
        base_window_width = 800;
        base_window_height = 600;
        base_player_width = 400;
        base_vis_width = 260;
        base_vis_height = 120;
        base_queue_width = 250;
        base_queue_height = 350;
    } else {
        // Large screens
        base_window_width = 900;
        base_window_height = 700;
        base_player_width = 500;
        base_vis_width = 400;
        base_vis_height = 200;
        base_queue_width = 300;
        base_queue_height = 400;
    }
    
    // Use appropriate reference resolution
    int ref_width = (screen_width < 1200) ? 1024 : 1920;
    int ref_height = (screen_height < 900) ? 768 : 1080;
    
    // Calculate sizes
    layout->config.window_width = scale_size(base_window_width, screen_width, ref_width);
    layout->config.window_height = scale_size(base_window_height, screen_height, ref_height);
    layout->config.player_width = scale_size(base_player_width, screen_width, ref_width);
    layout->config.vis_width = scale_size(base_vis_width, screen_width, ref_width);
    layout->config.vis_height = scale_size(base_vis_height, screen_height, ref_height);
    layout->config.queue_width = scale_size(base_queue_width, screen_width, ref_width);
    layout->config.queue_height = scale_size(base_queue_height, screen_height, ref_height);
    layout->config.icon_size = scale_size(64, screen_width, 1920);
    
    // Apply DPI scaling if needed
    int scale = 1; // We'll get this from the window later
    if (scale > 1) {
        layout->config.window_width /= scale;
        layout->config.window_height /= scale;
        layout->config.player_width /= scale;
        layout->config.vis_width /= scale;
        layout->config.vis_height /= scale;
        layout->config.queue_width /= scale;
        layout->config.queue_height /= scale;
    }
    
    // Apply minimums
    if (screen_width <= 800) {
        layout->config.window_width = screen_width;
        layout->config.window_height = screen_height;
        layout->config.vis_width = fmax(layout->config.vis_width, 180);
        layout->config.vis_height = fmax(layout->config.vis_height, 60);
        layout->config.queue_width = fmax(layout->config.queue_width, 180);
        layout->config.queue_height = fmax(layout->config.queue_height, 250);
    } else if (screen_width <= 1024) {
        layout->config.window_width = fmax(layout->config.window_width, 800);
        layout->config.window_height = fmax(layout->config.window_height, 600);
        layout->config.player_width = fmax(layout->config.player_width, 400);
        layout->config.vis_width = fmax(layout->config.vis_width, 220);
        layout->config.vis_height = fmax(layout->config.vis_height, 100);
        layout->config.queue_width = fmax(layout->config.queue_width, 250);
        layout->config.queue_height = fmax(layout->config.queue_height, 300);
    } else {
        layout->config.window_width = fmax(layout->config.window_width, 800);
        layout->config.window_height = fmax(layout->config.window_height, 600);
        layout->config.player_width = fmax(layout->config.player_width, 400);
        layout->config.vis_width = fmax(layout->config.vis_width, 300);
        layout->config.vis_height = fmax(layout->config.vis_height, 150);
        layout->config.queue_width = fmax(layout->config.queue_width, 250);
        layout->config.queue_height = fmax(layout->config.queue_height, 300);
    }
    
    // Icon size bounds
    layout->config.icon_size = fmax(layout->config.icon_size, 32);
    layout->config.icon_size = fmin(layout->config.icon_size, 96);
}

static void create_menu_bar(AudioPlayer *player) {
    GtkWidget *menubar = gtk_menu_bar_new();
    
    // File menu
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
    
    GtkWidget *open_item = gtk_menu_item_new_with_label("Open One File (Clears Queue)");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
    g_signal_connect(open_item, "activate", G_CALLBACK(on_menu_open), player);

    GtkWidget *playlist_separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), playlist_separator);

    GtkWidget *load_playlist_item = gtk_menu_item_new_with_label("Load Playlist...");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), load_playlist_item);
    g_signal_connect(load_playlist_item, "activate", G_CALLBACK(on_menu_load_playlist), player);

    GtkWidget *save_playlist_item = gtk_menu_item_new_with_label("Save Playlist...");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_playlist_item);
    g_signal_connect(save_playlist_item, "activate", G_CALLBACK(on_menu_save_playlist), player);

    GtkWidget *add_to_queue_playlist_item = gtk_menu_item_new_with_label("Add to Queue...");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), add_to_queue_playlist_item);
    g_signal_connect(add_to_queue_playlist_item, "activate", G_CALLBACK(on_add_to_queue_clicked), player);

    GtkWidget *clear_queue_playlist_item = gtk_menu_item_new_with_label("Clear Queue...");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), clear_queue_playlist_item);
    g_signal_connect(clear_queue_playlist_item, "activate", G_CALLBACK(on_clear_queue_clicked), player);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_menu_quit), player);
    
    // Help menu
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);

    add_keyboard_shortcuts_menu(player, help_menu);
    
    GtkWidget *about_item = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);
    g_signal_connect(about_item, "activate", G_CALLBACK(on_menu_about), player);
    
    gtk_box_pack_start(GTK_BOX(player->layout.player_vbox), menubar, FALSE, FALSE, 0);
}

static void create_visualization_section(AudioPlayer *player) {
    // Initialize visualizer
    player->visualizer = visualizer_new();
    
    // Visualization section
    GtkWidget *vis_frame = gtk_frame_new("Visualization");
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), vis_frame, FALSE, FALSE, 0);
    
    GtkWidget *vis_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(vis_frame), vis_vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vis_vbox), 5);
    
    // Set visualizer size based on layout config
    gtk_widget_set_size_request(player->visualizer->drawing_area, 
                                player->layout.config.vis_width, 
                                player->layout.config.vis_height);
    
    // Add visualization drawing area
    gtk_box_pack_start(GTK_BOX(vis_vbox), player->visualizer->drawing_area, TRUE, TRUE, 0);
    
    // Add visualization controls
    player->vis_controls = create_visualization_controls(player->visualizer);
    gtk_box_pack_start(GTK_BOX(vis_vbox), player->vis_controls, FALSE, FALSE, 0);
}

static void create_player_controls(AudioPlayer *player) {
    // File label
    player->file_label = gtk_label_new("No file loaded");
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->file_label, FALSE, FALSE, 0);
    
    // Progress scale
    player->progress_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 0.1);
    gtk_scale_set_draw_value(GTK_SCALE(player->progress_scale), FALSE);
    gtk_widget_set_sensitive(player->progress_scale, FALSE);
    g_signal_connect(player->progress_scale, "value-changed", G_CALLBACK(on_progress_scale_value_changed), player);
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->progress_scale, FALSE, FALSE, 0);
    
    player->time_label = gtk_label_new("00:00 / 00:00");
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->time_label, FALSE, FALSE, 0);
    
    // Navigation buttons
    player->layout.nav_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(player->layout.nav_button_box), TRUE);
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->layout.nav_button_box, FALSE, FALSE, 0);
    
    player->prev_button = gtk_button_new_with_label("|◄");
    player->rewind_button = gtk_button_new_with_label("◄◄ 5s");
    player->play_button = gtk_button_new_with_label("▶");
    player->pause_button = gtk_button_new_with_label("⏸");
    player->stop_button = gtk_button_new_with_label("⏹");
    player->fast_forward_button = gtk_button_new_with_label("5s ▶▶");
    player->next_button = gtk_button_new_with_label("▶|");
    
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->prev_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->rewind_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->play_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->pause_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->stop_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->fast_forward_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.nav_button_box), player->next_button, TRUE, TRUE, 0);
    
    // Volume controls
    player->layout.volume_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->layout.volume_box, FALSE, FALSE, 0);
    
    GtkWidget *volume_label = gtk_label_new("Volume:");
    player->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
    gtk_range_set_value(GTK_RANGE(player->volume_scale), (double)globalVolume / 100.0);
    
    gtk_box_pack_start(GTK_BOX(player->layout.volume_box), volume_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.volume_box), player->volume_scale, TRUE, TRUE, 0);
}

static void create_queue_controls_compact(AudioPlayer *player) {
    printf("Creating compact queue controls layout\n");
    
    // Create horizontal box for bottom controls (only queue controls, no equalizer)
    player->layout.compact.bottom_controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->layout.compact.bottom_controls_hbox, FALSE, FALSE, 0);
    
    // Queue controls in horizontal layout for compact
    player->add_to_queue_button = gtk_button_new_with_label("Add");
    player->clear_queue_button = gtk_button_new_with_label("Clear");
    player->repeat_queue_button = gtk_check_button_new_with_label("Repeat");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(player->repeat_queue_button), TRUE);
    
    // Set smaller button sizes for compact layout
    gtk_widget_set_size_request(player->add_to_queue_button, 80, 30);
    gtk_widget_set_size_request(player->clear_queue_button, 80, 30);
    
    gtk_box_pack_start(GTK_BOX(player->layout.compact.bottom_controls_hbox), player->add_to_queue_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.compact.bottom_controls_hbox), player->clear_queue_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.compact.bottom_controls_hbox), player->repeat_queue_button, TRUE, TRUE, 0);
}


static void create_queue_controls_regular(AudioPlayer *player) {
    printf("Creating regular queue controls layout\n");
    
    // Standard layout - horizontal queue controls (no equalizer here)
    player->layout.regular.queue_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), player->layout.regular.queue_button_box, FALSE, FALSE, 0);
    
    player->add_to_queue_button = gtk_button_new_with_label("Add to Queue");
    player->clear_queue_button = gtk_button_new_with_label("Clear Queue");
    player->repeat_queue_button = gtk_check_button_new_with_label("Repeat Queue");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(player->repeat_queue_button), TRUE);
    
    gtk_box_pack_start(GTK_BOX(player->layout.regular.queue_button_box), player->add_to_queue_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.regular.queue_button_box), player->clear_queue_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(player->layout.regular.queue_button_box), player->repeat_queue_button, TRUE, TRUE, 0);
}

static void create_icon_section(AudioPlayer *player) {
    player->layout.bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_end(GTK_BOX(player->layout.content_vbox), player->layout.bottom_box, FALSE, FALSE, 0);

    GdkPixbuf *small_icon = load_icon_from_base64();
    if (small_icon) {
        GdkPixbuf *scaled_icon = gdk_pixbuf_scale_simple(small_icon, 
                                                        player->layout.config.icon_size, 
                                                        player->layout.config.icon_size, 
                                                        GDK_INTERP_BILINEAR);
        if (scaled_icon) {
            GtkWidget *icon_image = gtk_image_new_from_pixbuf(scaled_icon);
            GtkWidget *icon_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_box_pack_start(GTK_BOX(icon_box), icon_image, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(player->layout.content_vbox), icon_box, FALSE, FALSE, 0);
            g_object_unref(scaled_icon);
        }
        g_object_unref(small_icon);
    }

    // Add spacing
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(player->layout.bottom_box), spacer, TRUE, TRUE, 0);
}

static void create_queue_display(AudioPlayer *player) {
    // Queue display - now on the right side but arranged vertically
    player->layout.queue_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(player->layout.main_hbox), player->layout.queue_vbox, TRUE, TRUE, 0);

    GtkWidget *queue_label = gtk_label_new("Queue:");
    gtk_widget_set_halign(queue_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(player->layout.queue_vbox), queue_label, FALSE, FALSE, 0);

    // Set size based on layout config - adjust height to leave room for equalizer
    player->queue_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(player->queue_scrolled_window), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // Reduce queue height to make room for equalizer below
    int adjusted_queue_height = player->layout.config.queue_height - 150; // Reserve space for equalizer
    gtk_widget_set_size_request(player->queue_scrolled_window, 
                               player->layout.config.queue_width, 
                               adjusted_queue_height);

    player->queue_listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(player->queue_scrolled_window), player->queue_listbox);
    gtk_box_pack_start(GTK_BOX(player->layout.queue_vbox), player->queue_scrolled_window, TRUE, TRUE, 0);
    
    // Add equalizer at the bottom of the right side
    gtk_box_pack_end(GTK_BOX(player->layout.queue_vbox), player->layout.shared_equalizer, FALSE, FALSE, 0);
}

static void connect_widget_signals(AudioPlayer *player) {
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
    setup_keyboard_shortcuts(player);
}

static void hide_unused_layout(AudioPlayer *player) {
    if (player->layout.config.is_compact) {
        // Hide regular layout widgets
        if (player->layout.regular.queue_button_box) {
            gtk_widget_hide(player->layout.regular.queue_button_box);
        }
        // No need to hide eq_below_controls since equalizer is now in queue_vbox
    } else {
        // Hide compact layout widgets
        if (player->layout.compact.bottom_controls_hbox) {
            gtk_widget_hide(player->layout.compact.bottom_controls_hbox);
        }
    }
}

// Function to switch layouts at runtime (for future use)
void switch_layout(AudioPlayer *player, bool to_compact) {
    if (player->layout.config.is_compact == to_compact) {
        return; // Already in the desired layout
    }
    
    player->layout.config.is_compact = to_compact;
    
    if (to_compact) {
        // Switch to compact
        gtk_widget_hide(player->layout.regular.queue_button_box);
        gtk_widget_show_all(player->layout.compact.bottom_controls_hbox);
    } else {
        // Switch to regular
        gtk_widget_hide(player->layout.compact.bottom_controls_hbox);
        gtk_widget_show_all(player->layout.regular.queue_button_box);
    }
    // Equalizer stays visible in queue_vbox for both layouts
}

void create_main_window(AudioPlayer *player) {
    // Calculate layout configuration first
    calculate_layout_config(&player->layout);
    
    // Create main window
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), "Zenamp");
    gtk_window_set_default_size(GTK_WINDOW(player->window), 
                               player->layout.config.window_width, 
                               player->layout.config.window_height);
    gtk_container_set_border_width(GTK_CONTAINER(player->window), 10);
    
    set_window_icon_from_base64(GTK_WINDOW(player->window));
    
    // Connect realize signal to handle DPI scaling
    g_signal_connect(player->window, "realize", G_CALLBACK(on_window_realize), player);
    g_signal_connect(player->window, "configure-event", G_CALLBACK(on_window_resize), player);

    // Main layout structure
    player->layout.main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(player->window), player->layout.main_hbox);
    
    // Player controls vbox (left side)
    player->layout.player_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(player->layout.player_vbox, player->layout.config.player_width, -1);
    gtk_box_pack_start(GTK_BOX(player->layout.main_hbox), player->layout.player_vbox, FALSE, FALSE, 0);
    
    // Create menu bar
    create_menu_bar(player);
    
    // Content area for left side
    player->layout.content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(player->layout.content_vbox), 10);
    gtk_box_pack_start(GTK_BOX(player->layout.player_vbox), player->layout.content_vbox, TRUE, TRUE, 0);
    
    // Create sections in new order:
    // Left side: visualization at top, controls below
    create_visualization_section(player);
    create_player_controls(player);
    
    // Create shared equalizer widget FIRST (before queue display)
    create_shared_equalizer(player);
    
    // Create both layout variants for queue controls (but only show the active one)
    //create_queue_controls_compact(player);
    create_queue_controls_regular(player);
    
    create_icon_section(player);
    
    // Right side: queue at top, equalizer at bottom (handled in create_queue_display)
    create_queue_display(player);
    
    // Hide the layout that's not being used
    //hide_unused_layout(player);
    
    // Connect all signals
    connect_widget_signals(player);
    
    printf("Created main window with %s layout (screen-based decision)\n", 
           player->layout.config.is_compact ? "compact" : "regular");
}
void create_shared_equalizer(AudioPlayer *player) {
    if (!player->layout.shared_equalizer) {
        printf("Creating shared equalizer widget\n");
        player->layout.shared_equalizer = create_equalizer_controls(player);
    }
}
