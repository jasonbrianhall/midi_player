#include "cdg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

CDGDisplay* cdg_display_new(void) {
    CDGDisplay *display = calloc(1, sizeof(CDGDisplay));
    if (!display) return NULL;
    
    for (int i = 0; i < CDG_COLORS; i++) {
        display->palette[i] = 0x000000;
    }

    display->border_color = 0;
    display->transparent_color = 0;

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
}

void cdg_update(CDGDisplay *cdg, double time_seconds) {
    if (!cdg || !cdg->packets) {
        fprintf(stderr, "[CDG] cdg_update called with null cdg or packets\n");
        return;
    }
    
    // CDG runs at 300 packets per second (75 sectors/sec * 4 packets/sector)
    int target_packet = (int)(time_seconds * 300.0);
    
    // Clamp to valid range
    if (target_packet < 0) target_packet = 0;
    if (target_packet >= cdg->packet_count) target_packet = cdg->packet_count - 1;
    
    // Process all packets from current position to target
    while (cdg->current_packet < target_packet) {
        cdg_process_packet(cdg, &cdg->packets[cdg->current_packet]);
        cdg->current_packet++;
    }
}

void cdg_process_packet(CDGDisplay *cdg, CDGPacket *packet) {
    if (!cdg || !packet) return;
    
    // Check if this is a CDG command (not a CD audio packet)
    if ((packet->command & 0x3F) != 0x09) {
        return; // Not a graphics command
    }
    
    uint8_t instruction = packet->instruction & 0x3F;
    
    switch (instruction) {
        case 1: // Memory Preset
        {
            uint8_t color = packet->data[0] & 0x0F;
            uint8_t repeat = packet->data[1] & 0x0F;
            
            // Fill screen with color
            for (int y = 0; y < CDG_HEIGHT; y++) {
                for (int x = 0; x < CDG_WIDTH; x++) {
                    cdg->screen[y][x] = color;
                }
            }
            break;
        }
        
        case 6: // Tile Block (normal)
        case 38: // Tile Block XOR
        {
            bool xor_mode = (instruction == 38);
            uint8_t color0 = packet->data[0] & 0x0F;
            uint8_t color1 = packet->data[1] & 0x0F;
            uint8_t row = (packet->data[2] & 0x1F);
            uint8_t column = (packet->data[3] & 0x3F);
            
            // Each tile is 6x12 pixels
            int pixel_row = row * 12;
            int pixel_col = column * 6;
            
            // Draw tile using the 12 bytes of pixel data
            for (int y = 0; y < 12; y++) {
                uint8_t byte = packet->data[4 + y];
                for (int x = 0; x < 6; x++) {
                    int px = pixel_col + x;
                    int py = pixel_row + y;
                    
                    if (px >= 0 && px < CDG_WIDTH && py >= 0 && py < CDG_HEIGHT) {
                        uint8_t bit = (byte >> (5 - x)) & 1;
                        uint8_t color = bit ? color1 : color0;
                        
                        if (xor_mode) {
                            cdg->screen[py][px] ^= color;
                        } else {
                            cdg->screen[py][px] = color;
                        }
                    }
                }
            }
            break;
        }
        
        case 30: // Load Color Table (Low)
        case 31: // Load Color Table (High)
        {
            int offset = (instruction == 30) ? 0 : 8;
            
            for (int i = 0; i < 8; i++) {
                uint16_t entry = (packet->data[2 * i] << 8) | packet->data[2 * i + 1];
                
                // Extract RGB components (each is 4 bits)
                uint8_t r = ((entry >> 8) & 0x0F) * 17; // Scale 0-15 to 0-255
                uint8_t g = ((entry >> 4) & 0x0F) * 17;
                uint8_t b = (entry & 0x0F) * 17;
                
                cdg->palette[offset + i] = (r << 16) | (g << 8) | b;
            }
            break;
        }
        
        case 20: // Scroll Preset
        case 24: // Scroll Copy
        {
            // Not implementing scrolling for now - these are complex
            // Most karaoke files work without them
            break;
        }
        
        default:
            // Unknown instruction
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
    
    for (int i = 0; i < 8; i++) {
        uint8_t byte0 = data[2 * i] & 0x3F;
        uint8_t byte1 = data[2 * i + 1] & 0x3F;
        
        // Combine high 2 bits (byte0) with low 2 bits (byte1) for each channel
        uint8_t r = ((byte0 & 0x30) >> 2) | ((byte1 & 0x30) >> 4);
        uint8_t g = (byte0 & 0x0C) | ((byte1 & 0x0C) >> 2);
        uint8_t b = ((byte0 & 0x03) << 2) | (byte1 & 0x03);
        
        // Scale 4-bit to 8-bit
        r = (r << 4) | r;  // More accurate than * 17
        g = (g << 4) | g;
        b = (b << 4) | b;
        
        display->palette[start_index + i] = (r << 16) | (g << 8) | b;
    }
}
