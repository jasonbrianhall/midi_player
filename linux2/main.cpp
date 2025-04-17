#include "midiplayer.h"
#include "audio.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <signal.h>

// Global flag for handling termination
bool quit = false;

// Handle termination signals
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    quit = true;
}

int main(int argc, char* argv[]) {
    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Error: No MIDI file specified\n";
        std::cerr << "Usage: " << argv[0] << " <midi_file>\n";
        return 1;
    }
    
    std::string filename = argv[1];
    std::cout << "Loading MIDI file: " << filename << std::endl;
    
    try {
        // Initialize audio system
        if (!Audio::init()) {
            std::cerr << "Failed to initialize audio system" << std::endl;
            return 1;
        }
        
        // Initialize MIDI player
        MidiPlayer player;
        if (!player.init()) {
            std::cerr << "Failed to initialize MIDI player" << std::endl;
            Audio::shutdown();
            return 1;
        }
        
        // Load the MIDI file
        if (!player.loadFile(filename)) {
            std::cerr << "Failed to load MIDI file: " << filename << std::endl;
            Audio::shutdown();
            return 1;
        }
        
        // Start playing
        player.play();
        
        std::cout << "Starting playback..." << std::endl;
        
        // Main loop with error handling
        while (!quit && player.isPlaying()) {
            try {
                // Update player state
                player.update();
                
                // Simple status display
                static int counter = 0;
                if (counter++ % 100 == 0) {
                    std::cout << "\rPlaying | Volume: " << player.getVolume() << "%   " << std::flush;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "\nException during playback: " << e.what() << std::endl;
                break;
            }
            catch (...) {
                std::cerr << "\nUnknown exception during playback" << std::endl;
                break;
            }
            
            // Don't hog the CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Stop playback
        player.stop();
        
        std::cout << "\nPlayback finished!" << std::endl;
        
        // Clean up
        Audio::shutdown();
    }
    catch (const std::exception& e) {
        std::cerr << "Critical error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown critical error" << std::endl;
        return 1;
    }
    
    return 0;
}
