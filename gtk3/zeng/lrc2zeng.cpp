#include "zeng.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <memory>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

// Cairo for high-quality text rendering
#include <cairo/cairo.h>

// STB Image for loading background images
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Structures for lyrics
struct WordTiming {
    std::string word;
    double start;
    double end;
};

struct LyricLine {
    double timestamp;
    std::string text;
};

struct Line {
    std::vector<WordTiming> words;
    std::string text;
    double start;
    double end;
};

// Parse LRC timestamp [mm:ss.xx] or [mm:ss.xxx]
double parse_lrc_timestamp(const std::string& timestamp) {
    std::regex time_regex(R"(\[(\d+):(\d+)\.(\d+)\])");
    std::smatch match;
    
    if (std::regex_search(timestamp, match, time_regex)) {
        int minutes = std::stoi(match[1]);
        int seconds = std::stoi(match[2]);
        std::string fraction_str = match[3];
        double fraction = std::stod("0." + fraction_str);
        return minutes * 60.0 + seconds + fraction;
    }
    return 0.0;
}

// Parse LRC file
std::vector<LyricLine> parse_lrc_file(const std::string& filename) {
    std::vector<LyricLine> lines;
    std::ifstream file(filename);
    
    if (!file) {
        std::cerr << "ERROR: Could not open LRC file: " << filename << std::endl;
        return lines;
    }
    
    std::string line;
    std::regex timestamp_regex(R"(\[(\d+:\d+\.\d+)\])");
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] != '[') continue;
        
        // Skip metadata lines
        if (line.length() > 2 && line[1] >= 'a' && line[1] <= 'z' && line[2] == ':') {
            continue;
        }
        
        std::smatch match;
        if (std::regex_search(line, match, timestamp_regex)) {
            double timestamp = parse_lrc_timestamp(match[0]);
            std::string text = std::regex_replace(line, timestamp_regex, "");
            
            // Trim whitespace
            text.erase(0, text.find_first_not_of(" \t\n\r"));
            text.erase(text.find_last_not_of(" \t\n\r") + 1);
            
            if (!text.empty()) {
                lines.push_back({timestamp, text});
            }
        }
    }
    
    std::sort(lines.begin(), lines.end(), 
              [](const LyricLine& a, const LyricLine& b) { return a.timestamp < b.timestamp; });
    
    return lines;
}

// Convert LRC lines to word timings
std::vector<WordTiming> lrc_to_word_timings(const std::vector<LyricLine>& lrc_lines) {
    std::vector<WordTiming> word_timings;
    
    for (size_t i = 0; i < lrc_lines.size(); i++) {
        const auto& line = lrc_lines[i];
        
        double line_duration;
        if (i + 1 < lrc_lines.size()) {
            line_duration = lrc_lines[i + 1].timestamp - line.timestamp;
        } else {
            line_duration = 3.0;
        }
        
        std::istringstream iss(line.text);
        std::vector<std::string> words;
        std::string word;
        while (iss >> word) {
            words.push_back(word);
        }
        
        if (words.empty()) continue;
        
        double time_per_word = line_duration / words.size();
        
        for (size_t j = 0; j < words.size(); j++) {
            double word_start = line.timestamp + j * time_per_word;
            double word_end = word_start + time_per_word;
            word_timings.push_back({words[j], word_start, word_end});
        }
    }
    
    return word_timings;
}

// Group words into lines for display
std::vector<Line> group_words_into_lines(const std::vector<WordTiming>& transcript,
                                         int max_chars_per_line = 50,
                                         int max_words_per_line = 8) {
    std::vector<Line> lines;
    std::vector<WordTiming> current_line_words;
    std::string current_line_text;
    
    for (const auto& entry : transcript) {
        std::string word = entry.word;
        word.erase(0, word.find_first_not_of(" \t\n\r"));
        word.erase(word.find_last_not_of(" \t\n\r") + 1);
        
        if (word.empty()) continue;
        
        std::string test_text = current_line_text + (current_line_text.empty() ? "" : " ") + word;
        
        if (!current_line_words.empty() && 
            (test_text.length() > static_cast<size_t>(max_chars_per_line) || 
             current_line_words.size() >= static_cast<size_t>(max_words_per_line))) {
            
            Line line;
            line.words = current_line_words;
            line.text = current_line_text;
            line.start = current_line_words[0].start;
            line.end = current_line_words.back().end;
            lines.push_back(line);
            
            current_line_words.clear();
            current_line_text = "";
            test_text = word;
        }
        
        current_line_words.push_back({word, entry.start, entry.end});
        current_line_text = test_text;
    }
    
    if (!current_line_words.empty()) {
        Line line;
        line.words = current_line_words;
        line.text = current_line_text;
        line.start = current_line_words[0].start;
        line.end = current_line_words.back().end;
        lines.push_back(line);
    }
    
    return lines;
}

// Render text to high-resolution surface
struct RenderedText {
    std::vector<uint8_t> alpha_data;
    int width;
    int height;
    int text_width;  // Actual text width
};

RenderedText render_text_hd(const std::string& text, int font_size = 48, 
                             int max_width = 1800) {
    RenderedText result;
    
    // Create high-res surface
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_A8, max_width, font_size * 2);
    cairo_t* cr = cairo_create(surface);
    
    // Setup font
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_size);
    
    // Get text dimensions
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text.c_str(), &extents);
    result.text_width = static_cast<int>(extents.width);
    
    // Center text
    double x_pos = (max_width - extents.width) / 2;
    double y_pos = font_size * 1.5;
    cairo_move_to(cr, x_pos, y_pos);
    cairo_show_text(cr, text.c_str());
    cairo_surface_flush(surface);
    
    // Copy data
    unsigned char* data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    result.width = max_width;
    result.height = font_size * 2;
    
    result.alpha_data.resize(result.width * result.height);
    for (int y = 0; y < result.height; y++) {
        for (int x = 0; x < result.width; x++) {
            result.alpha_data[y * result.width + x] = data[y * stride + x];
        }
    }
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    
    return result;
}

// Load and process background image
bool load_background_image(ZenGDisplay& display, const std::string& image_path) {
    int width, height, channels;
    unsigned char* img_data = stbi_load(image_path.c_str(), &width, &height, &channels, 3);
    
    if (!img_data) {
        std::cerr << "WARNING: Could not load image: " << image_path << std::endl;
        return false;
    }
    
    std::cout << "Loaded background image: " << width << "x" << height << std::endl;
    
    // Generate palette from image (k-means or median cut would be better)
    // For now, use a simple quantization
    std::vector<uint32_t> palette(256);
    for (int i = 0; i < 256; i++) {
        // Generate a reasonable palette
        int r = (i & 0xE0);           // 3 bits red
        int g = (i & 0x1C) << 3;      // 3 bits green  
        int b = (i & 0x03) << 6;      // 2 bits blue
        palette[i] = (r << 16) | (g << 8) | b;
    }
    
    // Reserve last few colors for text
    palette[254] = 0xFFFFFF;  // White text
    palette[255] = 0x6AAEFF;  // Blue highlight
    
    display.loadPalette(palette.data(), 256);
    
    // Scale and quantize image to display
    for (int y = 0; y < display.height; y++) {
        for (int x = 0; x < display.width; x++) {
            int src_x = (x * width) / display.width;
            int src_y = (y * height) / display.height;
            int idx = (src_y * width + src_x) * 3;
            
            uint8_t r = img_data[idx];
            uint8_t g = img_data[idx + 1];
            uint8_t b = img_data[idx + 2];
            
            // Simple quantization
            uint8_t color_idx = ((r & 0xE0)) | ((g & 0xE0) >> 3) | ((b & 0xC0) >> 6);
            display.screen[y * display.width + x] = color_idx;
        }
    }
    
    stbi_image_free(img_data);
    return true;
}

// Draw text with alpha blending onto display
void draw_text_with_alpha(ZenGDisplay& display, const RenderedText& text, 
                          int x, int y, uint8_t color_idx, uint8_t threshold = 128) {
    for (int ty = 0; ty < text.height && (y + ty) < display.height; ty++) {
        if ((y + ty) < 0) continue;
        for (int tx = 0; tx < text.width && (x + tx) < display.width; tx++) {
            if ((x + tx) < 0) continue;
            
            uint8_t alpha = text.alpha_data[ty * text.width + tx];
            if (alpha > threshold) {
                display.screen[(y + ty) * display.width + (x + tx)] = color_idx;
            }
        }
    }
}

// Generate ZenG packets from lyrics
bool generate_zeng_from_lrc(const std::string& lrc_path, 
                            const std::string& output_path,
                            const std::string& image_path = "",
                            double song_duration = 0.0,
                            ZenGResolution resolution = ZENG_RES_HD_1080) {
    
    std::cout << "Parsing LRC file: " << lrc_path << std::endl;
    auto lrc_lines = parse_lrc_file(lrc_path);
    
    if (lrc_lines.empty()) {
        std::cerr << "ERROR: No lyrics found in LRC file" << std::endl;
        return false;
    }
    
    std::cout << "Found " << lrc_lines.size() << " lyric lines" << std::endl;
    
    // Calculate duration
    if (song_duration == 0.0) {
        song_duration = lrc_lines.back().timestamp + 5.0;
    }
    
    std::cout << "Song duration: " << std::fixed << std::setprecision(2) 
              << song_duration << "s" << std::endl;
    
    // Create ZenG display
    ZenGDisplay display;
    display.setResolution(resolution, 0, 0);
    display.setColorMode(ZENG_COLOR_INDEXED_256);
    display.setFPS(60);  // High frame rate for smooth timing
    
    display.header.duration_ms = static_cast<uint32_t>(song_duration * 1000);
    
    // Load background if provided
    bool has_background = false;
    if (!image_path.empty()) {
        has_background = load_background_image(display, image_path);
    }
    
    // If no background, create a gradient
    if (!has_background) {
        std::vector<uint32_t> palette(256);
        for (int i = 0; i < 256; i++) {
            // Create a nice gradient palette
            int r = std::min(255, i * 2);
            int g = std::min(255, i + 50);
            int b = std::min(255, 255 - i);
            palette[i] = (r << 16) | (g << 8) | b;
        }
        palette[254] = 0xFFFFFF;  // White
        palette[255] = 0x6AAEFF;  // Highlight blue
        
        display.loadPalette(palette.data(), 256);
        
        // Fill with gradient
        for (int y = 0; y < display.height; y++) {
            uint8_t color = (y * 255) / display.height;
            for (int x = 0; x < display.width; x++) {
                display.screen[y * display.width + x] = color / 4;  // Dark gradient
            }
        }
    }
    
    // Save initial state as first packet
    ZenGPacket init_packet(0, ZENG_CMD_GRAPHICS_EXT, ZENG_INST_LOAD_PALETTE_FULL);
    init_packet.setData(reinterpret_cast<uint8_t*>(display.palette.data()), 
                        display.palette.size() * 4);
    display.packets.push_back(init_packet);
    
    // Convert to word timings
    auto word_timings = lrc_to_word_timings(lrc_lines);
    std::cout << "Generated " << word_timings.size() << " word timings" << std::endl;
    
    // Group into lines
    auto lines = group_words_into_lines(word_timings, 50, 8);
    std::cout << "Created " << lines.size() << " display lines" << std::endl;
    
    // Calculate font size based on resolution
    int font_size = display.height / 15;  // Adjust for resolution
    int line_height = display.height / 6;
    
    // Process each line
    for (size_t line_idx = 0; line_idx < lines.size(); line_idx++) {
        const auto& line = lines[line_idx];
        
        std::cout << "[" << std::setw(6) << std::setprecision(2) << line.start 
                  << "s - " << std::setw(6) << line.end << "s] Line " 
                  << std::setw(3) << (line_idx + 1) << ": " << line.text << std::endl;
        
        // Render text at high resolution
        RenderedText rendered = render_text_hd(line.text, font_size, display.width - 100);
        
        // Calculate position
        int y_pos = (display.height / 2) - font_size + (line_idx % 4) * line_height;
        int x_pos = (display.width - rendered.text_width) / 2;
        
        // Create packet to draw white text at line start time
        uint32_t line_start_ms = static_cast<uint32_t>(line.start * 1000);
        
        // For high precision, we could store the rendered text data
        // For now, we'll create a simplified draw packet
        ZenGPacket text_packet(line_start_ms, ZENG_CMD_GRAPHICS_EXT, ZENG_INST_DRAW_TEXT);
        
        // Store position and text
        std::vector<uint8_t> text_data;
        text_data.push_back((x_pos >> 8) & 0xFF);
        text_data.push_back(x_pos & 0xFF);
        text_data.push_back((y_pos >> 8) & 0xFF);
        text_data.push_back(y_pos & 0xFF);
        text_data.push_back(254);  // White color
        text_data.push_back(font_size);
        
        // Add text
        for (char c : line.text) {
            text_data.push_back(static_cast<uint8_t>(c));
        }
        
        text_packet.setData(text_data.data(), text_data.size());
        display.packets.push_back(text_packet);
        
        // Highlight first word
        if (!line.words.empty()) {
            uint32_t highlight_ms = static_cast<uint32_t>(line.words[0].start * 1000);
            
            ZenGPacket highlight_packet(highlight_ms, ZENG_CMD_GRAPHICS_EXT, 
                                       ZENG_INST_DRAW_TEXT);
            
            std::vector<uint8_t> highlight_data;
            highlight_data.push_back((x_pos >> 8) & 0xFF);
            highlight_data.push_back(x_pos & 0xFF);
            highlight_data.push_back((y_pos >> 8) & 0xFF);
            highlight_data.push_back(y_pos & 0xFF);
            highlight_data.push_back(255);  // Highlight color
            highlight_data.push_back(font_size);
            
            for (char c : line.text) {
                highlight_data.push_back(static_cast<uint8_t>(c));
            }
            
            highlight_packet.setData(highlight_data.data(), highlight_data.size());
            display.packets.push_back(highlight_packet);
            
            // Restore to white after 2 seconds
            uint32_t restore_ms = highlight_ms + 2000;
            ZenGPacket restore_packet = text_packet;
            restore_packet.timestamp_ms = restore_ms;
            display.packets.push_back(restore_packet);
        }
    }
    
    // Sort packets by timestamp
    std::sort(display.packets.begin(), display.packets.end(),
              [](const ZenGPacket& a, const ZenGPacket& b) {
                  return a.timestamp_ms < b.timestamp_ms;
              });
    
    // Save ZenG file
    std::cout << "Writing ZenG file: " << output_path << std::endl;
    if (!display.saveFile(output_path)) {
        std::cerr << "ERROR: Failed to write ZenG file" << std::endl;
        return false;
    }
    
    std::cout << "Successfully created ZenG file with " << display.packets.size() 
              << " packets" << std::endl;
    std::cout << "Resolution: " << display.width << "x" << display.height << std::endl;
    std::cout << "Color mode: 256 indexed colors" << std::endl;
    std::cout << "Frame rate: " << display.fps << " fps" << std::endl;
    
    return true;
}

// Main function
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " input.lrc output.zeng [OPTIONS]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --duration SECONDS    Song duration in seconds" << std::endl;
        std::cout << "  --image FILE          Background image" << std::endl;
        std::cout << "  --resolution RES      Resolution: cdg, sd, 720, 1080, 4k (default: 1080)" << std::endl;
        return 1;
    }
    
    std::string lrc_path = argv[1];
    std::string output_path = argv[2];
    std::string image_path;
    double song_duration = 0.0;
    ZenGResolution resolution = ZENG_RES_HD_1080;
    
    // Parse arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--image" && i + 1 < argc) {
            image_path = argv[i + 1];
            i++;
        } else if (arg == "--duration" && i + 1 < argc) {
            song_duration = std::stod(argv[i + 1]);
            i++;
        } else if (arg == "--resolution" && i + 1 < argc) {
            std::string res = argv[i + 1];
            if (res == "cdg") resolution = ZENG_RES_CDG;
            else if (res == "sd") resolution = ZENG_RES_SD;
            else if (res == "720") resolution = ZENG_RES_HD_720;
            else if (res == "1080") resolution = ZENG_RES_HD_1080;
            else if (res == "4k") resolution = ZENG_RES_4K;
            i++;
        }
    }
    
    bool success = generate_zeng_from_lrc(lrc_path, output_path, image_path, 
                                          song_duration, resolution);
    
    return success ? 0 : 1;
}
