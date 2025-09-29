#include "cdg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

CDGDisplay* cdg_display_new(void) {
    CDGDisplay *display = calloc(1, sizeof(CDGDisplay));
    if (!display) return NULL;
    
    // Initialize with default colors (black background)
    for (int i = 0; i < CDG_COLORS; i++) {
        display->palette[i] = 0x000000;  // Black
    }
    
    display->border_color = 0;
    display->transparent_color = 0;
    display->ball_radius = 12.0;
    display->ball_y = CDG_HEIGHT / 2.0;
    display->ball_x = 50.0;
    
    return display;
}

void cdg_display_free(CDGDisplay *display) {
    if (!display) return;
    
    if (display->packets) {
        free(display->packets);
    }
    
    free(display);
}

bool cdg_load_file(CDGDisplay *display, const char *filename) {
    if (!display || !filename) return false;
    
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Failed to open CDG file: %s\n", filename);
        return false;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Each packet is 24 bytes (subchannel format)
    if (size % 24 != 0) {
        printf("Invalid CDG file size: %ld\n", size);
        fclose(f);
        return false;
    }
    
    display->packet_count = size / 24;
    display->packets = malloc(display->packet_count * sizeof(CDGPacket));
    if (!display->packets) {
        fclose(f);
        return false;
    }
    
    // Read all packets
    for (int i = 0; i < display->packet_count; i++) {
        uint8_t raw[24];
        if (fread(raw, 1, 24, f) != 24) {
            printf("Failed to read packet %d\n", i);
            free(display->packets);
            display->packets = NULL;
            fclose(f);
            return false;
        }
        
        // Extract command and instruction (mask with 0x3F)
        display->packets[i].command = raw[0] & CDG_COMMAND_MASK;
        display->packets[i].instruction = raw[1] & CDG_COMMAND_MASK;
        
        // Copy the 16 data bytes (skip 4 header bytes, skip 4 parity bytes at end)
        memcpy(display->packets[i].data, &raw[4], 16);
    }
    
    fclose(f);
    
    printf("Loaded CDG file: %d packets (%.1f seconds)\n", 
           display->packet_count, 
           display->packet_count / (double)CDG_PACKETS_PER_SECOND);
    
    // Reset display state
    cdg_reset(display);
    
    return true;
}

void cdg_reset(CDGDisplay *display) {
    if (!display) return;
    
    memset(display->screen, 0, sizeof(display->screen));
    display->current_packet = 0;
    display->last_update_time = 0.0;
    display->ball_x = 50.0;
    display->ball_y = CDG_HEIGHT / 2.0;
    display->ball_velocity_y = 0.0;
}

void cdg_update(CDGDisplay *display, double playTime) {
    if (!display || !display->packets) return;
    
    // Calculate target packet based on time
    int target_packet = (int)(playTime * CDG_PACKETS_PER_SECOND);
    
    // Clamp to valid range
    if (target_packet < 0) target_packet = 0;
    if (target_packet > display->packet_count) target_packet = display->packet_count;
    
    // Process ONE packet per call to allow rendering between packets
    // If we're behind, the timer will call us again quickly
    if (display->current_packet < target_packet) {
        cdg_process_packet(display, &display->packets[display->current_packet]);
        display->current_packet++;
    }
    
    display->last_update_time = playTime;
}

void cdg_process_packet(CDGDisplay *display, CDGPacket *packet) {
    if (!display || !packet) return;
    
    // Only process graphics commands
    if (packet->command != CDG_COMMAND_GRAPHICS) {
        return;
    }
    
    switch (packet->instruction) {
        case CDG_INST_MEMORY_PRESET: {
            // Clear screen to specified color
            uint8_t color = packet->data[0] & 0x0F;
            uint8_t repeat = packet->data[1] & 0x0F;
            
            // Fill screen with color
            for (int y = 0; y < CDG_HEIGHT; y++) {
                for (int x = 0; x < CDG_WIDTH; x++) {
                    display->screen[y][x] = color;
                }
            }
            
            display->border_color = color;
            break;
        }
        
        case CDG_INST_BORDER_PRESET: {
            // Set border color
            display->border_color = packet->data[0] & 0x0F;
            break;
        }
        
        case CDG_INST_TILE_BLOCK: {
            // Draw a 6x12 pixel tile
            uint8_t color0 = packet->data[0] & 0x0F;
            uint8_t color1 = packet->data[1] & 0x0F;
            int row = (packet->data[2] & 0x1F);      // 0-17
            int col = (packet->data[3] & 0x3F);      // 0-49
            
            cdg_draw_tile(display, col, row, color0, color1, &packet->data[4]);
            break;
        }
        
        case CDG_INST_SCROLL_PRESET: {
            // Scroll and fill with color
            uint8_t color = packet->data[0] & 0x0F;
            uint8_t h_cmd = (packet->data[1] & 0x30) >> 4;
            uint8_t v_cmd = (packet->data[2] & 0x30) >> 4;
            
            cdg_scroll_screen(display, h_cmd, v_cmd, color);
            break;
        }
        
        case CDG_INST_SCROLL_COPY: {
            // Scroll and wrap around
            uint8_t h_cmd = (packet->data[1] & 0x30) >> 4;
            uint8_t v_cmd = (packet->data[2] & 0x30) >> 4;
            
            cdg_scroll_screen(display, h_cmd, v_cmd, 0xFF);  // 0xFF = copy mode
            break;
        }
        
        case CDG_INST_DEFINE_TRANSPARENT: {
            // Set transparent color
            display->transparent_color = packet->data[0] & 0x0F;
            break;
        }
        
        case CDG_INST_LOAD_COLORMAP_LOW: {
            // Load colors 0-7
            cdg_load_colormap(display, packet->data, false);
            break;
        }
        
        case CDG_INST_LOAD_COLORMAP_HIGH: {
            // Load colors 8-15
            cdg_load_colormap(display, packet->data, true);
            break;
        }
        
        default:
            // Unknown instruction, ignore
            break;
    }
}

void cdg_draw_tile(CDGDisplay *display, int col, int row, uint8_t color0, uint8_t color1, uint8_t *tile_data) {
    // Calculate pixel position
    int x = col * CDG_TILE_WIDTH;
    int y = row * CDG_TILE_HEIGHT;
    
    // Draw 12 rows of 6 pixels each
    for (int r = 0; r < CDG_TILE_HEIGHT; r++) {
        uint8_t byte = tile_data[r] & 0x3F;  // Only bottom 6 bits used
        
        for (int c = 0; c < CDG_TILE_WIDTH; c++) {
            // Check bit (MSB first)
            uint8_t pixel_color = (byte & (1 << (5 - c))) ? color1 : color0;
            
            int px = x + c;
            int py = y + r;
            
            // Bounds check
            if (px >= 0 && px < CDG_WIDTH && py >= 0 && py < CDG_HEIGHT) {
                display->screen[py][px] = pixel_color;
            }
        }
    }
}

void cdg_scroll_screen(CDGDisplay *display, int h_cmd, int v_cmd, uint8_t fill_color) {
    // Horizontal scroll: 0=none, 1=right 6px, 2=left 6px
    // Vertical scroll: 0=none, 1=down 12px, 2=up 12px
    
    int h_offset = 0, v_offset = 0;
    
    if (h_cmd == 1) h_offset = 6;
    else if (h_cmd == 2) h_offset = -6;
    
    if (v_cmd == 1) v_offset = 12;
    else if (v_cmd == 2) v_offset = -12;
    
    if (h_offset == 0 && v_offset == 0) return;
    
    // Create temporary buffer
    uint8_t temp[CDG_HEIGHT][CDG_WIDTH];
    memcpy(temp, display->screen, sizeof(temp));
    
    // Scroll and fill/wrap
    for (int y = 0; y < CDG_HEIGHT; y++) {
        for (int x = 0; x < CDG_WIDTH; x++) {
            int src_x = x - h_offset;
            int src_y = y - v_offset;
            
            if (fill_color == 0xFF) {
                // Wrap mode
                src_x = (src_x + CDG_WIDTH) % CDG_WIDTH;
                src_y = (src_y + CDG_HEIGHT) % CDG_HEIGHT;
                display->screen[y][x] = temp[src_y][src_x];
            } else {
                // Fill mode
                if (src_x >= 0 && src_x < CDG_WIDTH && src_y >= 0 && src_y < CDG_HEIGHT) {
                    display->screen[y][x] = temp[src_y][src_x];
                } else {
                    display->screen[y][x] = fill_color;
                }
            }
        }
    }
}

void cdg_load_colormap(CDGDisplay *display, uint8_t *data, bool high_colors) {
    int start_index = high_colors ? 8 : 0;
    
    // Each color is 2 bytes: [RGB0][0rgb]
    for (int i = 0; i < 8; i++) {
        uint8_t byte0 = data[2 * i] & 0x3F;
        uint8_t byte1 = data[2 * i + 1] & 0x3F;
        
        // Extract RGB (4 bits each, scaled to 8 bits)
        uint8_t r = ((byte0 & 0x30) >> 4) * 17;  // Scale 0-3 to 0-255
        uint8_t g = ((byte0 & 0x0C) >> 2) * 17;
        uint8_t b = ((byte0 & 0x03)) * 17;
        
        // Combine into RGB888
        display->palette[start_index + i] = (r << 16) | (g << 8) | b;
    }
}
