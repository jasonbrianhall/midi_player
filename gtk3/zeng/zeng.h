#ifndef ZENG_H
#define ZENG_H

#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>
#include <memory>

// ZenG - Enhanced karaoke graphics format (superset of CDG)
// Design goals:
// - Higher resolution support (scalable)
// - More colors (8-bit indexed or 24-bit RGB)
// - Higher frame rates (60/120 fps capable)
// - Backward compatible concepts with CDG
// - Efficient for modern hardware

// ============================================================================
// RESOLUTION PROFILES
// ============================================================================
// ZenG supports multiple resolution profiles for different use cases

typedef enum {
    ZENG_RES_CDG,      // 300x216 (original CDG compatibility)
    ZENG_RES_SD,       // 640x480 (SD)
    ZENG_RES_HD_720,   // 1280x720 (HD)
    ZENG_RES_HD_1080,  // 1920x1080 (Full HD)
    ZENG_RES_4K,       // 3840x2160 (4K)
    ZENG_RES_CUSTOM    // Custom resolution (stored in header)
} ZenGResolution;

// ============================================================================
// COLOR MODES
// ============================================================================

typedef enum {
    ZENG_COLOR_INDEXED_16,   // 16 colors (CDG compatible)
    ZENG_COLOR_INDEXED_256,  // 256 color palette
    ZENG_COLOR_RGB565,       // 16-bit RGB (5-6-5)
    ZENG_COLOR_RGB888,       // 24-bit RGB (8-8-8)
    ZENG_COLOR_RGBA8888      // 32-bit RGBA (8-8-8-8 with alpha)
} ZenGColorMode;

// ============================================================================
// FILE FORMAT
// ============================================================================

#define ZENG_MAGIC "ZENG"
#define ZENG_VERSION 1

// File header (64 bytes, padded for alignment)
typedef struct {
    char magic[4];              // "ZENG"
    uint16_t version;           // Format version (1)
    uint16_t flags;             // Feature flags
    
    // Resolution
    uint8_t resolution_profile; // ZenGResolution enum
    uint8_t reserved1;
    uint16_t width;             // Actual width (if CUSTOM)
    uint16_t height;            // Actual height (if CUSTOM)
    
    // Color configuration
    uint8_t color_mode;         // ZenGColorMode enum
    uint8_t bits_per_pixel;     // Actual BPP
    uint16_t palette_size;      // Number of palette entries (0 if direct color)
    
    // Timing
    uint16_t fps;               // Frames per second (typically 60 or 75)
    uint16_t packet_rate;       // Packets per second (for compatibility)
    
    // Data offsets
    uint32_t palette_offset;    // Offset to palette data (0 if none)
    uint32_t packet_offset;     // Offset to packet data
    uint32_t packet_count;      // Total number of packets
    
    // Metadata
    uint32_t duration_ms;       // Duration in milliseconds
    uint32_t metadata_offset;   // Offset to metadata (song title, artist, etc.)
    uint32_t metadata_size;     // Size of metadata block
    
    uint8_t reserved[16];       // Reserved for future use
} ZenGHeader;

// Feature flags
#define ZENG_FLAG_HAS_ALPHA      0x0001  // Contains alpha channel
#define ZENG_FLAG_HAS_AUDIO_SYNC 0x0002  // Has audio synchronization data
#define ZENG_FLAG_COMPRESSED     0x0004  // Packets are compressed
#define ZENG_FLAG_DELTA_ENCODED  0x0008  // Uses delta encoding
#define ZENG_FLAG_HAS_EFFECTS    0x0010  // Contains effect packets
#define ZENG_FLAG_CDG_COMPAT     0x0020  // Fully CDG backward compatible

// ============================================================================
// PACKET FORMAT
// ============================================================================

// Enhanced packet structure (variable size)
class ZenGPacket {
public:
    uint32_t timestamp_ms;      // Timestamp in milliseconds
    uint8_t command;            // Command byte
    uint8_t instruction;        // Instruction byte
    uint16_t data_size;         // Size of data payload
    std::vector<uint8_t> data;  // Variable-size data payload
    
    ZenGPacket(uint32_t timestamp = 0, uint8_t cmd = 0, uint8_t inst = 0)
        : timestamp_ms(timestamp), command(cmd), instruction(inst), data_size(0) {}
    
    void setData(const uint8_t* src, uint16_t size) {
        data.resize(size);
        std::copy(src, src + size, data.begin());
        data_size = size;
    }
};

// Command types (0x00-0x0F reserved for CDG compatibility)
#define ZENG_CMD_CDG_COMPAT      0x09  // CDG graphics command (compatible)

// Extended ZenG commands (0x10+)
#define ZENG_CMD_GRAPHICS_EXT    0x10  // Extended graphics
#define ZENG_CMD_PALETTE         0x11  // Palette operations
#define ZENG_CMD_EFFECTS         0x12  // Visual effects
#define ZENG_CMD_SYNC            0x13  // Synchronization/timing
#define ZENG_CMD_METADATA        0x14  // Metadata/text
#define ZENG_CMD_COMPRESSION     0x15  // Compression control

// ============================================================================
// GRAPHICS INSTRUCTIONS
// ============================================================================

// CDG-compatible instructions (0x00-0x1F)
#define ZENG_INST_MEMORY_PRESET      1   // Fill screen
#define ZENG_INST_BORDER_PRESET      2   // Set border
#define ZENG_INST_TILE_BLOCK         6   // Draw tile
#define ZENG_INST_SCROLL_PRESET      20  // Scroll with fill
#define ZENG_INST_SCROLL_COPY        24  // Scroll with wrap
#define ZENG_INST_TRANSPARENT        28  // Set transparent color
#define ZENG_INST_LOAD_CLUT_LOW      30  // Load palette 0-7
#define ZENG_INST_LOAD_CLUT_HIGH     31  // Load palette 8-15

// Extended instructions (0x20+)
#define ZENG_INST_CLEAR_SCREEN       0x20  // Clear to color
#define ZENG_INST_DRAW_RECT          0x21  // Draw filled rectangle
#define ZENG_INST_DRAW_LINE          0x22  // Draw line
#define ZENG_INST_DRAW_SPRITE        0x23  // Draw sprite/image
#define ZENG_INST_DRAW_TEXT          0x24  // Draw text with font
#define ZENG_INST_BLIT               0x25  // Blit image data
#define ZENG_INST_GRADIENT           0x26  // Draw gradient
#define ZENG_INST_TILE_BLOCK_EXT     0x27  // Extended tile (variable size)

// Alpha/blending operations
#define ZENG_INST_SET_ALPHA          0x30  // Set global alpha
#define ZENG_INST_SET_BLEND_MODE     0x31  // Set blend mode
#define ZENG_INST_ALPHA_BLIT         0x32  // Blit with alpha

// Transform operations
#define ZENG_INST_SET_TRANSFORM      0x40  // Set transform matrix
#define ZENG_INST_ROTATE             0x41  // Rotate region
#define ZENG_INST_SCALE              0x42  // Scale region
#define ZENG_INST_FLIP               0x43  // Flip horizontal/vertical

// Effect instructions
#define ZENG_INST_FADE               0x50  // Fade in/out
#define ZENG_INST_WIPE               0x51  // Wipe transition
#define ZENG_INST_BLUR               0x52  // Blur effect
#define ZENG_INST_GLOW               0x53  // Glow effect
#define ZENG_INST_PARTICLE           0x54  // Particle effect

// Palette operations
#define ZENG_INST_LOAD_PALETTE_FULL  0x60  // Load full palette
#define ZENG_INST_LOAD_PALETTE_RANGE 0x61  // Load palette range
#define ZENG_INST_PALETTE_FADE       0x62  // Fade palette
#define ZENG_INST_PALETTE_CYCLE      0x63  // Cycle palette colors

// ============================================================================
// DISPLAY CONTEXT
// ============================================================================

class ZenGDisplay {
public:
    // Header information
    ZenGHeader header;
    
    // Display buffers
    std::vector<uint8_t> screen;      // Main screen buffer (indexed or RGB)
    std::vector<uint32_t> palette;    // Palette (if indexed mode)
    std::vector<uint8_t> alpha;       // Alpha channel (if RGBA mode)
    
    // Dimensions
    uint16_t width;
    uint16_t height;
    uint32_t buffer_size;
    
    // Packet storage
    std::vector<ZenGPacket> packets;
    uint32_t current_packet;
    
    // Rendering state
    uint8_t border_color;
    uint8_t transparent_color;
    uint8_t global_alpha;
    uint8_t blend_mode;
    
    // Transform state
    float transform[6];         // 2D affine transform matrix
    
    // Timing
    double current_time_ms;
    double last_update_time;
    uint16_t fps;
    
    // Metadata
    std::string title;
    std::string artist;
    std::string metadata;
    
    // Constructor
    ZenGDisplay();
    ~ZenGDisplay();
    
    // Configuration
    void setResolution(ZenGResolution res, uint16_t custom_w = 0, uint16_t custom_h = 0);
    void setColorMode(ZenGColorMode mode);
    void setFPS(uint16_t fps);
    
    // File operations
    bool loadFile(const std::string& filename);
    bool saveFile(const std::string& filename);
    
    // Playback
    void update(double time_ms);
    void reset();
    void processPacket(const ZenGPacket& packet);
    
    // CDG conversion
    bool fromCDG(const std::string& cdg_filename);
    bool toCDG(const std::string& cdg_filename);
    
    // Drawing operations
    void clear(uint32_t color);
    void drawRect(int x, int y, int w, int h, uint32_t color);
    void drawSprite(int x, int y, const uint8_t* sprite_data, int w, int h);
    void drawText(int x, int y, const std::string& text, uint32_t color);
    
    // Effects
    void applyFade(float factor);
    void applyBlur(int radius);
    
    // Palette operations
    void loadPalette(const uint32_t* colors, int count);
    void paletteFade(float factor);
    
    // Utility
    uint32_t getPixel(int x, int y) const;
    void setPixel(int x, int y, uint32_t color);
    
private:
    void initializeBuffers();
    void processCDGCompatPacket(const ZenGPacket& packet);
    void processExtendedPacket(const ZenGPacket& packet);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

namespace ZenG {
    // Get resolution dimensions for predefined profiles
    void getResolutionDimensions(ZenGResolution res, uint16_t* width, uint16_t* height);
    
    // Packet creation helper
    ZenGPacket createPacket(uint32_t timestamp_ms, uint8_t cmd, uint8_t inst, 
                           const uint8_t* data = nullptr, uint16_t data_size = 0);
}

#endif // ZENG_H
