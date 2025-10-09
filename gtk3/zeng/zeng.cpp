#include "zeng.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>

// ============================================================================
// ZenGDisplay Implementation
// ============================================================================

ZenGDisplay::ZenGDisplay() 
    : width(0), height(0), buffer_size(0), current_packet(0),
      border_color(0), transparent_color(0), global_alpha(255), blend_mode(0),
      current_time_ms(0.0), last_update_time(0.0), fps(60) {
    
    // Initialize header
    memset(&header, 0, sizeof(ZenGHeader));
    strncpy(header.magic, ZENG_MAGIC, 4);
    header.version = ZENG_VERSION;
    header.fps = 60;
    
    // Initialize transform to identity
    transform[0] = 1.0f; transform[1] = 0.0f; transform[2] = 0.0f;
    transform[3] = 0.0f; transform[4] = 1.0f; transform[5] = 0.0f;
    
    // Default to CDG resolution and indexed 16 color mode
    setResolution(ZENG_RES_CDG, 0, 0);
    setColorMode(ZENG_COLOR_INDEXED_16);
}

ZenGDisplay::~ZenGDisplay() {
    // Vectors auto-cleanup
}

void ZenGDisplay::setResolution(ZenGResolution res, uint16_t custom_w, uint16_t custom_h) {
    header.resolution_profile = res;
    
    switch (res) {
        case ZENG_RES_CDG:
            width = 300;
            height = 216;
            break;
        case ZENG_RES_SD:
            width = 640;
            height = 480;
            break;
        case ZENG_RES_HD_720:
            width = 1280;
            height = 720;
            break;
        case ZENG_RES_HD_1080:
            width = 1920;
            height = 1080;
            break;
        case ZENG_RES_4K:
            width = 3840;
            height = 2160;
            break;
        case ZENG_RES_CUSTOM:
            width = custom_w;
            height = custom_h;
            break;
    }
    
    header.width = width;
    header.height = height;
    initializeBuffers();
}

void ZenGDisplay::setColorMode(ZenGColorMode mode) {
    header.color_mode = mode;
    
    switch (mode) {
        case ZENG_COLOR_INDEXED_16:
            header.bits_per_pixel = 4;
            header.palette_size = 16;
            palette.resize(16);
            break;
        case ZENG_COLOR_INDEXED_256:
            header.bits_per_pixel = 8;
            header.palette_size = 256;
            palette.resize(256);
            break;
        case ZENG_COLOR_RGB565:
            header.bits_per_pixel = 16;
            header.palette_size = 0;
            break;
        case ZENG_COLOR_RGB888:
            header.bits_per_pixel = 24;
            header.palette_size = 0;
            break;
        case ZENG_COLOR_RGBA8888:
            header.bits_per_pixel = 32;
            header.palette_size = 0;
            header.flags |= ZENG_FLAG_HAS_ALPHA;
            break;
    }
    
    initializeBuffers();
}

void ZenGDisplay::setFPS(uint16_t new_fps) {
    fps = new_fps;
    header.fps = new_fps;
}

void ZenGDisplay::initializeBuffers() {
    buffer_size = width * height;
    
    // Allocate screen buffer based on color mode
    if (header.color_mode == ZENG_COLOR_RGB888) {
        screen.resize(buffer_size * 3);  // RGB
    } else if (header.color_mode == ZENG_COLOR_RGBA8888) {
        screen.resize(buffer_size * 4);  // RGBA
    } else if (header.color_mode == ZENG_COLOR_RGB565) {
        screen.resize(buffer_size * 2);  // 16-bit
    } else {
        screen.resize(buffer_size);      // Indexed
    }
    
    // Clear buffers
    std::fill(screen.begin(), screen.end(), 0);
    
    if (header.flags & ZENG_FLAG_HAS_ALPHA) {
        alpha.resize(buffer_size);
        std::fill(alpha.begin(), alpha.end(), 255);
    }
}

bool ZenGDisplay::loadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        printf("Failed to open ZenG file: %s\n", filename.c_str());
        return false;
    }
    
    // Read header
    file.read(reinterpret_cast<char*>(&header), sizeof(ZenGHeader));
    
    // Verify magic
    if (strncmp(header.magic, ZENG_MAGIC, 4) != 0) {
        printf("Invalid ZenG file magic\n");
        return false;
    }
    
    // Set dimensions from header
    width = header.width;
    height = header.height;
    fps = header.fps;
    
    // Initialize buffers
    setColorMode(static_cast<ZenGColorMode>(header.color_mode));
    
    // Read palette if present
    if (header.palette_offset > 0 && header.palette_size > 0) {
        file.seekg(header.palette_offset);
        palette.resize(header.palette_size);
        file.read(reinterpret_cast<char*>(palette.data()), 
                  header.palette_size * sizeof(uint32_t));
    }
    
    // Read packets
    if (header.packet_offset > 0 && header.packet_count > 0) {
        file.seekg(header.packet_offset);
        packets.resize(header.packet_count);
        
        for (uint32_t i = 0; i < header.packet_count; i++) {
            file.read(reinterpret_cast<char*>(&packets[i].timestamp_ms), 4);
            file.read(reinterpret_cast<char*>(&packets[i].command), 1);
            file.read(reinterpret_cast<char*>(&packets[i].instruction), 1);
            file.read(reinterpret_cast<char*>(&packets[i].data_size), 2);
            
            if (packets[i].data_size > 0) {
                packets[i].data.resize(packets[i].data_size);
                file.read(reinterpret_cast<char*>(packets[i].data.data()), 
                         packets[i].data_size);
            }
        }
    }
    
    // Read metadata if present
    if (header.metadata_offset > 0 && header.metadata_size > 0) {
        file.seekg(header.metadata_offset);
        std::vector<char> meta_buffer(header.metadata_size);
        file.read(meta_buffer.data(), header.metadata_size);
        metadata = std::string(meta_buffer.begin(), meta_buffer.end());
    }
    
    file.close();
    reset();
    
    printf("Loaded ZenG file: %dx%d, %d packets, %.1f seconds\n",
           width, height, header.packet_count, 
           header.duration_ms / 1000.0);
    
    return true;
}

bool ZenGDisplay::saveFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        printf("Failed to create ZenG file: %s\n", filename.c_str());
        return false;
    }
    
    // Update header with current state
    header.packet_count = packets.size();
    
    // Calculate offsets
    uint32_t current_offset = sizeof(ZenGHeader);
    
    if (palette.size() > 0) {
        header.palette_offset = current_offset;
        current_offset += palette.size() * sizeof(uint32_t);
    }
    
    header.packet_offset = current_offset;
    // Calculate packet data size
    for (const auto& packet : packets) {
        current_offset += 8 + packet.data_size;  // header + data
    }
    
    if (!metadata.empty()) {
        header.metadata_offset = current_offset;
        header.metadata_size = metadata.size();
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(ZenGHeader));
    
    // Write palette
    if (palette.size() > 0) {
        file.write(reinterpret_cast<const char*>(palette.data()), 
                   palette.size() * sizeof(uint32_t));
    }
    
    // Write packets
    for (const auto& packet : packets) {
        file.write(reinterpret_cast<const char*>(&packet.timestamp_ms), 4);
        file.write(reinterpret_cast<const char*>(&packet.command), 1);
        file.write(reinterpret_cast<const char*>(&packet.instruction), 1);
        file.write(reinterpret_cast<const char*>(&packet.data_size), 2);
        if (packet.data_size > 0) {
            file.write(reinterpret_cast<const char*>(packet.data.data()), 
                      packet.data_size);
        }
    }
    
    // Write metadata
    if (!metadata.empty()) {
        file.write(metadata.c_str(), metadata.size());
    }
    
    file.close();
    printf("Saved ZenG file: %s\n", filename.c_str());
    return true;
}

void ZenGDisplay::reset() {
    std::fill(screen.begin(), screen.end(), 0);
    current_packet = 0;
    current_time_ms = 0.0;
    last_update_time = 0.0;
}

void ZenGDisplay::update(double time_ms) {
    current_time_ms = time_ms;
    
    // Calculate target packet based on time
    uint32_t target_packet = current_packet;
    
    // Find the last packet that should be displayed at this time
    while (target_packet < packets.size() && 
           packets[target_packet].timestamp_ms <= time_ms) {
        target_packet++;
    }
    
    // If seeking backward, reset and replay
    if (time_ms < last_update_time) {
        reset();
        current_time_ms = time_ms;
    }
    
    // Process packets up to target
    while (current_packet < target_packet && current_packet < packets.size()) {
        processPacket(packets[current_packet]);
        current_packet++;
    }
    
    last_update_time = time_ms;
}

void ZenGDisplay::processPacket(const ZenGPacket& packet) {
    // Check command type
    if (packet.command == ZENG_CMD_CDG_COMPAT) {
        processCDGCompatPacket(packet);
    } else {
        processExtendedPacket(packet);
    }
}

void ZenGDisplay::processCDGCompatPacket(const ZenGPacket& packet) {
    // Process CDG-compatible instructions
    switch (packet.instruction) {
        case ZENG_INST_MEMORY_PRESET: {
            if (packet.data.size() < 1) break;
            uint8_t color = packet.data[0] & 0x0F;
            std::fill(screen.begin(), screen.end(), color);
            break;
        }
        
        case ZENG_INST_BORDER_PRESET: {
            if (packet.data.size() < 1) break;
            border_color = packet.data[0] & 0x0F;
            break;
        }
        
        case ZENG_INST_TILE_BLOCK: {
            if (packet.data.size() < 16) break;
            uint8_t color0 = packet.data[0] & 0x0F;
            uint8_t color1 = packet.data[1] & 0x0F;
            uint8_t row = packet.data[2] & 0x1F;
            uint8_t col = packet.data[3] & 0x3F;
            
            int pixel_row = row * 12;
            int pixel_col = col * 6;
            
            for (int y = 0; y < 12; y++) {
                uint8_t tile_byte = packet.data[4 + y];
                for (int x = 0; x < 6; x++) {
                    int px = pixel_col + x;
                    int py = pixel_row + y;
                    
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        uint8_t bit = (tile_byte >> (5 - x)) & 1;
                        uint8_t color = bit ? color1 : color0;
                        screen[py * width + px] = color;
                    }
                }
            }
            break;
        }
        
        case ZENG_INST_LOAD_CLUT_LOW:
        case ZENG_INST_LOAD_CLUT_HIGH: {
            if (packet.data.size() < 16) break;
            int offset = (packet.instruction == ZENG_INST_LOAD_CLUT_LOW) ? 0 : 8;
            
            for (int i = 0; i < 8; i++) {
                uint8_t byte0 = packet.data[2 * i] & 0x3F;
                uint8_t byte1 = packet.data[2 * i + 1] & 0x3F;
                
                uint8_t r = (byte0 >> 2) & 0x0F;
                uint8_t g = ((byte0 & 0x03) << 2) | ((byte1 >> 4) & 0x03);
                uint8_t b = (byte1 & 0x0F);
                
                uint8_t r8 = r * 17;
                uint8_t g8 = g * 17;
                uint8_t b8 = b * 17;
                
                palette[offset + i] = (r8 << 16) | (g8 << 8) | b8;
            }
            break;
        }
        
        case ZENG_INST_TRANSPARENT: {
            if (packet.data.size() < 1) break;
            transparent_color = packet.data[0] & 0x0F;
            break;
        }
    }
}

void ZenGDisplay::processExtendedPacket(const ZenGPacket& packet) {
    switch (packet.instruction) {
        case ZENG_INST_CLEAR_SCREEN: {
            if (packet.data.size() < 4) break;
            uint32_t color = *reinterpret_cast<const uint32_t*>(packet.data.data());
            clear(color);
            break;
        }
        
        case ZENG_INST_DRAW_RECT: {
            if (packet.data.size() < 12) break;
            int16_t x = *reinterpret_cast<const int16_t*>(&packet.data[0]);
            int16_t y = *reinterpret_cast<const int16_t*>(&packet.data[2]);
            int16_t w = *reinterpret_cast<const int16_t*>(&packet.data[4]);
            int16_t h = *reinterpret_cast<const int16_t*>(&packet.data[6]);
            uint32_t color = *reinterpret_cast<const uint32_t*>(&packet.data[8]);
            drawRect(x, y, w, h, color);
            break;
        }
        
        case ZENG_INST_SET_ALPHA: {
            if (packet.data.size() < 1) break;
            global_alpha = packet.data[0];
            break;
        }
        
        case ZENG_INST_LOAD_PALETTE_FULL: {
            uint32_t count = packet.data.size() / 4;
            loadPalette(reinterpret_cast<const uint32_t*>(packet.data.data()), count);
            break;
        }
    }
}

void ZenGDisplay::clear(uint32_t color) {
    if (header.color_mode == ZENG_COLOR_INDEXED_16 || 
        header.color_mode == ZENG_COLOR_INDEXED_256) {
        std::fill(screen.begin(), screen.end(), static_cast<uint8_t>(color));
    } else {
        // Direct color modes - would need to convert color to bytes
        // Simplified implementation
        std::fill(screen.begin(), screen.end(), 0);
    }
}

void ZenGDisplay::drawRect(int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h && py < height; py++) {
        if (py < 0) continue;
        for (int px = x; px < x + w && px < width; px++) {
            if (px < 0) continue;
            setPixel(px, py, color);
        }
    }
}

void ZenGDisplay::drawSprite(int x, int y, const uint8_t* sprite_data, int w, int h) {
    for (int py = 0; py < h && (y + py) < height; py++) {
        if ((y + py) < 0) continue;
        for (int px = 0; px < w && (x + px) < width; px++) {
            if ((x + px) < 0) continue;
            uint8_t pixel = sprite_data[py * w + px];
            screen[(y + py) * width + (x + px)] = pixel;
        }
    }
}

void ZenGDisplay::drawText(int x, int y, const std::string& text, uint32_t color) {
    // Placeholder - would need font rendering implementation
    printf("Text rendering not yet implemented: %s\n", text.c_str());
}

void ZenGDisplay::applyFade(float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    
    if (header.color_mode == ZENG_COLOR_INDEXED_16 || 
        header.color_mode == ZENG_COLOR_INDEXED_256) {
        // Fade palette
        for (auto& color : palette) {
            uint8_t r = ((color >> 16) & 0xFF) * factor;
            uint8_t g = ((color >> 8) & 0xFF) * factor;
            uint8_t b = (color & 0xFF) * factor;
            color = (r << 16) | (g << 8) | b;
        }
    }
}

void ZenGDisplay::applyBlur(int radius) {
    // Simplified box blur - production would use better algorithm
    printf("Blur effect not yet fully implemented\n");
}

void ZenGDisplay::loadPalette(const uint32_t* colors, int count) {
    count = std::min(count, static_cast<int>(palette.size()));
    std::copy(colors, colors + count, palette.begin());
}

void ZenGDisplay::paletteFade(float factor) {
    applyFade(factor);
}

uint32_t ZenGDisplay::getPixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return 0;
    
    if (header.color_mode == ZENG_COLOR_INDEXED_16 || 
        header.color_mode == ZENG_COLOR_INDEXED_256) {
        uint8_t index = screen[y * width + x];
        return palette[index];
    }
    
    return 0;  // Direct color modes would need different handling
}

void ZenGDisplay::setPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    
    int offset = y * width + x;
    
    if (header.color_mode == ZENG_COLOR_INDEXED_16 || 
        header.color_mode == ZENG_COLOR_INDEXED_256) {
        screen[offset] = static_cast<uint8_t>(color);
    } else if (header.color_mode == ZENG_COLOR_RGB888) {
        screen[offset * 3 + 0] = (color >> 16) & 0xFF;  // R
        screen[offset * 3 + 1] = (color >> 8) & 0xFF;   // G
        screen[offset * 3 + 2] = color & 0xFF;          // B
    } else if (header.color_mode == ZENG_COLOR_RGBA8888) {
        screen[offset * 4 + 0] = (color >> 16) & 0xFF;  // R
        screen[offset * 4 + 1] = (color >> 8) & 0xFF;   // G
        screen[offset * 4 + 2] = color & 0xFF;          // B
        screen[offset * 4 + 3] = (color >> 24) & 0xFF;  // A
    } else if (header.color_mode == ZENG_COLOR_RGB565) {
        uint16_t rgb565 = ((color >> 8) & 0xF800) |    // R
                          ((color >> 5) & 0x07E0) |     // G
                          ((color >> 3) & 0x001F);      // B
        screen[offset * 2 + 0] = rgb565 & 0xFF;
        screen[offset * 2 + 1] = (rgb565 >> 8) & 0xFF;
    }
}

bool ZenGDisplay::fromCDG(const std::string& cdg_filename) {
    // Open CDG file
    std::ifstream file(cdg_filename, std::ios::binary);
    if (!file.is_open()) {
        printf("Failed to open CDG file: %s\n", cdg_filename.c_str());
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size % 24 != 0) {
        printf("Invalid CDG file size: %zu\n", size);
        return false;
    }
    
    uint32_t cdg_packet_count = size / 24;
    
    // Set up ZenG display for CDG compatibility
    setResolution(ZENG_RES_CDG, 0, 0);
    setColorMode(ZENG_COLOR_INDEXED_16);
    header.flags |= ZENG_FLAG_CDG_COMPAT;
    header.packet_rate = 300;  // CDG runs at 300 packets/sec
    
    // Convert CDG packets to ZenG packets
    packets.clear();
    packets.reserve(cdg_packet_count);
    
    for (uint32_t i = 0; i < cdg_packet_count; i++) {
        uint8_t raw[24];
        file.read(reinterpret_cast<char*>(raw), 24);
        
        // Extract command and instruction
        uint8_t command = raw[0] & 0x3F;
        uint8_t instruction = raw[1] & 0x3F;
        
        // Only process graphics commands
        if (command == 0x09) {
            // Calculate timestamp (CDG runs at 300 packets/sec)
            uint32_t timestamp_ms = (i * 1000) / 300;
            
            // Create ZenG packet
            ZenGPacket packet(timestamp_ms, ZENG_CMD_CDG_COMPAT, instruction);
            
            // Copy data bytes (skip 4 header bytes, skip 4 parity bytes at end)
            packet.setData(&raw[4], 16);
            
            packets.push_back(packet);
        }
    }
    
    file.close();
    
    // Calculate duration
    if (!packets.empty()) {
        header.duration_ms = packets.back().timestamp_ms + 1000;
    }
    
    header.packet_count = packets.size();
    
    printf("Converted CDG to ZenG: %d packets\n", header.packet_count);
    reset();
    
    return true;
}

bool ZenGDisplay::toCDG(const std::string& cdg_filename) {
    // Only works if display is in CDG-compatible mode
    if (!(header.flags & ZENG_FLAG_CDG_COMPAT) || 
        header.resolution_profile != ZENG_RES_CDG ||
        header.color_mode != ZENG_COLOR_INDEXED_16) {
        printf("Display is not in CDG-compatible mode\n");
        return false;
    }
    
    std::ofstream file(cdg_filename, std::ios::binary);
    if (!file.is_open()) {
        printf("Failed to create CDG file: %s\n", cdg_filename.c_str());
        return false;
    }
    
    // CDG runs at 300 packets/sec (75 sectors/sec * 4 packets/sector)
    // We need to output packets at the correct timing
    uint32_t last_timestamp = 0;
    uint32_t cdg_packet_index = 0;
    
    for (const auto& packet : packets) {
        // Only process CDG-compatible packets
        if (packet.command != ZENG_CMD_CDG_COMPAT) continue;
        
        // Calculate how many CDG packets should have elapsed
        uint32_t target_cdg_packets = (packet.timestamp_ms * 300) / 1000;
        
        // Fill in empty packets if needed (timing gaps)
        while (cdg_packet_index < target_cdg_packets) {
            uint8_t raw[24] = {0};
            file.write(reinterpret_cast<const char*>(raw), 24);
            cdg_packet_index++;
        }
        
        // Write the actual packet
        uint8_t raw[24] = {0};
        raw[0] = ZENG_CMD_CDG_COMPAT;  // CDG graphics command
        raw[1] = packet.instruction;
        
        // Copy data (16 bytes starting at offset 4)
        if (packet.data_size >= 16) {
            memcpy(&raw[4], packet.data.data(), 16);
        }
        
        file.write(reinterpret_cast<const char*>(raw), 24);
        cdg_packet_index++;
    }
    
    file.close();
    printf("Converted ZenG to CDG: %d packets\n", cdg_packet_index);
    
    return true;
}

// ============================================================================
// Utility Functions
// ============================================================================

namespace ZenG {
    void getResolutionDimensions(ZenGResolution res, uint16_t* width, uint16_t* height) {
        switch (res) {
            case ZENG_RES_CDG:
                *width = 300;
                *height = 216;
                break;
            case ZENG_RES_SD:
                *width = 640;
                *height = 480;
                break;
            case ZENG_RES_HD_720:
                *width = 1280;
                *height = 720;
                break;
            case ZENG_RES_HD_1080:
                *width = 1920;
                *height = 1080;
                break;
            case ZENG_RES_4K:
                *width = 3840;
                *height = 2160;
                break;
            default:
                *width = 0;
                *height = 0;
                break;
        }
    }
    
    ZenGPacket createPacket(uint32_t timestamp_ms, uint8_t cmd, uint8_t inst,
                           const uint8_t* data, uint16_t data_size) {
        ZenGPacket packet(timestamp_ms, cmd, inst);
        if (data && data_size > 0) {
            packet.setData(data, data_size);
        }
        return packet;
    }
}

// ============================================================================
// Example Usage and Conversion Utilities
// ============================================================================

// Helper function to create a high-resolution ZenG from scratch
ZenGDisplay* createHDKaraokeDisplay() {
    ZenGDisplay* display = new ZenGDisplay();
    
    // Set to HD 1080p, 256 colors, 60fps
    display->setResolution(ZENG_RES_HD_1080, 0, 0);
    display->setColorMode(ZENG_COLOR_INDEXED_256);
    display->setFPS(60);
    
    // Set metadata
    display->title = "High Definition Karaoke";
    display->artist = "Various Artists";
    
    return display;
}

// Helper to upscale CDG to HD while maintaining visual style
bool upscaleCDGToHD(const std::string& cdg_file, const std::string& output_file,
                    ZenGResolution target_res) {
    // Load CDG as ZenG
    ZenGDisplay cdg_display;
    if (!cdg_display.fromCDG(cdg_file)) {
        return false;
    }
    
    // Create HD display
    ZenGDisplay hd_display;
    uint16_t target_w, target_h;
    ZenG::getResolutionDimensions(target_res, &target_w, &target_h);
    
    hd_display.setResolution(target_res, target_w, target_h);
    hd_display.setColorMode(ZENG_COLOR_INDEXED_256);
    hd_display.setFPS(60);
    
    // Copy palette with extended colors
    for (int i = 0; i < 16; i++) {
        hd_display.palette[i] = cdg_display.palette[i];
    }
    
    // Calculate scale factors
    float scale_x = static_cast<float>(target_w) / 300.0f;
    float scale_y = static_cast<float>(target_h) / 216.0f;
    
    // Convert packets - this is simplified, production code would
    // need to properly scale tile positions and sizes
    for (const auto& cdg_packet : cdg_display.packets) {
        ZenGPacket hd_packet = cdg_packet;
        
        // For tile blocks, scale the position
        if (cdg_packet.instruction == ZENG_INST_TILE_BLOCK) {
            // Would need more sophisticated scaling logic here
            // This is just a placeholder
        }
        
        hd_display.packets.push_back(hd_packet);
    }
    
    // Save HD version
    return hd_display.saveFile(output_file);
}

// Example: Create a simple ZenG animation from scratch
void createTestAnimation(const std::string& output_file) {
    ZenGDisplay display;
    display.setResolution(ZENG_RES_HD_720, 0, 0);
    display.setColorMode(ZENG_COLOR_INDEXED_256);
    display.setFPS(60);
    
    // Set up a colorful palette
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i * 7) & 0xFF;
        uint8_t g = (i * 13) & 0xFF;
        uint8_t b = (i * 19) & 0xFF;
        display.palette[i] = (r << 16) | (g << 8) | b;
    }
    
    // Create animation packets
    // Clear screen at start
    ZenGPacket clear_packet(0, ZENG_CMD_GRAPHICS_EXT, ZENG_INST_CLEAR_SCREEN);
    uint32_t black = 0x000000;
    clear_packet.setData(reinterpret_cast<uint8_t*>(&black), 4);
    display.packets.push_back(clear_packet);
    
    // Draw some rectangles over time
    for (int t = 0; t < 5000; t += 100) {  // 5 seconds, 10fps for simplicity
        ZenGPacket rect_packet(t, ZENG_CMD_GRAPHICS_EXT, ZENG_INST_DRAW_RECT);
        
        uint8_t rect_data[12];
        int16_t x = 100 + (t / 10) % 1000;
        int16_t y = 360;
        int16_t w = 80;
        int16_t h = 80;
        uint32_t color = 0xFF0000 | ((t / 20) & 0xFF);  // Animated red
        
        memcpy(&rect_data[0], &x, 2);
        memcpy(&rect_data[2], &y, 2);
        memcpy(&rect_data[4], &w, 2);
        memcpy(&rect_data[6], &h, 2);
        memcpy(&rect_data[8], &color, 4);
        
        rect_packet.setData(rect_data, 12);
        display.packets.push_back(rect_packet);
    }
    
    display.header.duration_ms = 5000;
    display.title = "ZenG Test Animation";
    
    display.saveFile(output_file);
    printf("Created test animation: %s\n", output_file.c_str());
}
