#include "icon.h"

GdkPixbuf* load_icon_from_base64(void) {
    GError *error = NULL;
    GdkPixbuf *pixbuf = NULL;
    
    // Decode base64 data
    gsize decoded_len;
    guchar *decoded_data = g_base64_decode(icon_base64_data, &decoded_len);
    
    if (!decoded_data) {
        g_warning("Failed to decode base64 icon data");
        return NULL;
    }
    
    // Create GdkPixbuf from decoded PNG data
    GInputStream *stream = g_memory_input_stream_new_from_data(decoded_data, decoded_len, g_free);
    
    if (stream) {
        pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, &error);
        g_object_unref(stream);
        
        if (error) {
            g_warning("Failed to create pixbuf from icon data: %s", error->message);
            g_error_free(error);
            return NULL;
        }
    } else {
        g_free(decoded_data);
        g_warning("Failed to create memory stream for icon data");
        return NULL;
    }
    
    return pixbuf;
}

void set_window_icon_from_base64(GtkWindow *window) {
    GdkPixbuf *icon = load_icon_from_base64();
    if (icon) {
        gtk_window_set_icon(window, icon);
        g_object_unref(icon);
    }
}
