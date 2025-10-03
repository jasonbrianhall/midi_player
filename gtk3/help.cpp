#ifndef HELPMENU_H
#define HELPMENU_H

#include "audio_player.h"
#include "icon.h"

void on_menu_about(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem;
    AudioPlayer *player = (AudioPlayer*)user_data;
    
    // Get screen resolution to adapt dialog size
    GdkScreen *screen = gtk_widget_get_screen(player->window);
    int screen_width = gdk_screen_get_width(screen);
    int screen_height = gdk_screen_get_height(screen);
    bool use_compact_dialog = (screen_width <= 1024 || screen_height <= 700);
    
    // Create main dialog window
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "About Zenamp",
        GTK_WINDOW(player->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    
    // Set dialog properties based on screen size
    if (use_compact_dialog) {
        gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
        gtk_widget_set_size_request(dialog, 480, 450); // Fit in 800x600
    } else {
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
        gtk_widget_set_size_request(dialog, 600, 500);
    }
    
    // Create notebook for tabs
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), use_compact_dialog ? 5 : 10);
    
    // Get content area and add notebook
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), notebook);
    
    // === ABOUT TAB ===
    // Create scrolled window for the About tab content
    GtkWidget *about_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(about_scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget *about_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, use_compact_dialog ? 8 : 10);
    gtk_container_set_border_width(GTK_CONTAINER(about_vbox), use_compact_dialog ? 10 : 20);
    
    // Logo - smaller for compact layout
    GdkPixbuf *logo = load_icon_from_base64();
    if (logo) {
        if (use_compact_dialog) {
            // Scale down logo for small screens
            GdkPixbuf *small_logo = gdk_pixbuf_scale_simple(logo, 48, 48, GDK_INTERP_BILINEAR);
            GtkWidget *logo_image = gtk_image_new_from_pixbuf(small_logo);
            gtk_box_pack_start(GTK_BOX(about_vbox), logo_image, FALSE, FALSE, 0);
            g_object_unref(small_logo);
        } else {
            GtkWidget *logo_image = gtk_image_new_from_pixbuf(logo);
            gtk_box_pack_start(GTK_BOX(about_vbox), logo_image, FALSE, FALSE, 0);
        }
        g_object_unref(logo);
    }
    
    // Program name and version - smaller text for compact layout
    GtkWidget *title_label = gtk_label_new(NULL);
    if (use_compact_dialog) {
        gtk_label_set_markup(GTK_LABEL(title_label), 
            "<span size='x-large' weight='bold'>Zenamp</span>\n"
            "<span size='medium'>Version 1.0</span>");
    } else {
        gtk_label_set_markup(GTK_LABEL(title_label), 
            "<span size='xx-large' weight='bold'>Zenamp</span>\n"
            "<span size='large'>Version 1.0</span>");
    }
    gtk_label_set_justify(GTK_LABEL(title_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(about_vbox), title_label, FALSE, FALSE, 0);
    
    // Description - more compact for small screens
    const char *description = use_compact_dialog ?
        "Multi-format music player with GTK interface\n\n"
        "Supported Formats:\n"
        "• MIDI (.mid, .midi) - OPL3 synthesis\n"
        "• WAV (.wav) - Uncompressed audio\n"
        "• MP3 (.mp3) - Classic compressed format\n"
        "• OGG (.ogg) - Open-source alternative\n"
        "• FLAC (.flac) - Lossless compression\n"
        "• OPUS (.opus) - Modern codec\n"
        "• AIFF (.aiff) - Apple audio format\n\n"
        "• CDG (.zip) - Karaoke Files\n\n"      
        "Other formats"      

        "Features:\n"
        "• Playlist queue with repeat mode\n"
        "• Audio visualizer\n"
        "• 3-band equalizer\n"
        "• Progress seeking\n"
        "• 5-second skip controls\n"
        "• Previous/next navigation\n"
        "• Volume control\n"
        "• GTK interface"
        :
        "A multi-format music player with GTK interface\n\n"
        "Supported Formats:\n"
        "• MIDI (.mid, .midi) - Nostalgic bleeps and bloops with OPL3 synthesis\n"
        "• WAV (.wav) - Uncompressed audio goodness, chunky but delicious\n"
        "• MP3 (.mp3) - The classic compressed format that conquered the world\n"
        "• OGG (.ogg) - Open-source alternative that sounds great\n"
        "• FLAC (.flac) - Lossless perfection for the audiophiles\n"
        "• OPUS (.opus) - Modern codec that's small but mighty\n"
        "• AIFF (.aiff) - Apple's answer to WAV, crisp and clean\n\n"
        "• CDG (.zip) - Karaoke Files\n\n"      
        "Other formats supported by either FFMPEG (Linux) or Windows Native"      
        "Features:\n"
        "• Playlist queue with repeat mode\n"
        "• Audio visualizer for eye candy while you listen\n"
        "• Equalizer to fine-tune your sound\n"
        "• Drag progress slider to seek\n"
        "• << and >> buttons for 5-second rewind/fast-forward\n"
        "• |< and >| buttons for previous/next song\n"
        "• Volume control\n"
        "• GTK interface\n";
    
    GtkWidget *desc_label = gtk_label_new(description);
    gtk_label_set_justify(GTK_LABEL(desc_label), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap(GTK_LABEL(desc_label), TRUE);
    gtk_box_pack_start(GTK_BOX(about_vbox), desc_label, TRUE, TRUE, 0);
    
    // Author and website
    GtkWidget *author_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(author_label), 
        "<b>Author:</b> Jason Hall\n"
        "<b>Website:</b> <a href=\"https://github.com/jasonbrianhall/midi_player/tree/main/gtk3\">"
        "GitHub Repository</a>");
    gtk_label_set_justify(GTK_LABEL(author_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(about_vbox), author_label, FALSE, FALSE, 0);
    
    // Add the vbox to the scrolled window
    if (use_compact_dialog) {
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(about_scrolled), about_vbox);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_scrolled, 
                                gtk_label_new("About"));
    } else {
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_vbox, 
                                gtk_label_new("About"));
    }
    
    // === LICENSE TAB ===
    GtkWidget *license_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(license_scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    if (use_compact_dialog) {
        gtk_widget_set_size_request(license_scrolled, 450, 350);
    } else {
        gtk_widget_set_size_request(license_scrolled, 550, 400);
    }
    
    GtkWidget *license_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(license_textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(license_textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(license_textview), GTK_WRAP_WORD);
    
    GtkTextBuffer *license_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(license_textview));
    const char *mit_license = 
        "MIT License\n\n"
        "Copyright (c) 2025 Jason Hall\n\n"
        "Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files (the \"Software\"), to deal "
        "in the Software without restriction, including without limitation the rights "
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
        "copies of the Software, and to permit persons to whom the Software is "
        "furnished to do so, subject to the following conditions:\n\n"
        "The above copyright notice and this permission notice shall be included in all "
        "copies or substantial portions of the Software.\n\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
        "SOFTWARE.";
    
    gtk_text_buffer_set_text(license_buffer, mit_license, -1);
    gtk_container_add(GTK_CONTAINER(license_scrolled), license_textview);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), license_scrolled, 
                            gtk_label_new("License"));
    
    // === PRIVACY POLICY TAB ===
    GtkWidget *privacy_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(privacy_scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    if (use_compact_dialog) {
        gtk_widget_set_size_request(privacy_scrolled, 450, 350);
    } else {
        gtk_widget_set_size_request(privacy_scrolled, 550, 400);
    }
    
    GtkWidget *privacy_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(privacy_textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(privacy_textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(privacy_textview), GTK_WRAP_WORD);
    
    GtkTextBuffer *privacy_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(privacy_textview));
    const char *privacy_policy = 
        "Privacy Policy for Zenamp\n\n"
        "Last updated: September 11, 2025\n\n"
        "Information We Collect:\n"
        "Zenamp is a local music player that does not collect, store, or transmit any "
        "personal information to external servers.\n\n"
        "Local File Access:\n"
        "- The application only accesses audio files that you explicitly choose to open\n"
        "- No file information is shared with third parties\n"
        "- All processing happens locally on your device\n\n"
        "Data Storage:\n"
        "- The application may store user preferences (volume settings, equalizer settings) "
        "locally on your device\n"
        "- No personal data is transmitted over the internet\n\n"
        "Contact:\n"
        "For questions about this privacy policy, contact jasonbrianhall@yahoo.com";
    
    gtk_text_buffer_set_text(privacy_buffer, privacy_policy, -1);
    gtk_container_add(GTK_CONTAINER(privacy_scrolled), privacy_textview);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privacy_scrolled, 
                            gtk_label_new("Privacy Policy"));
    
    // Show all widgets
    gtk_widget_show_all(dialog);
    
    // Run dialog and clean up
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

#endif
