// main.cpp
// Main program for SDL-based MIDI player

#include "midiplayer.h"
#include "audio.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>

void printHelp() {
    std::cout << "MIDI Player with OPL3 Synthesis via SDL\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Controls:\n";
    std::cout << "  Space - Pause/Resume\n";
    std::cout << "  +/-   - Increase/Decrease Volume\n";
    std::cout << "  N     - Toggle Volume Normalization\n";
    std::cout << "  ESC/Q - Quit\n";
    std::cout << "------------------------------------------\n";
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Error: No MIDI file specified\n";
        std::cerr << "Usage: " << argv[0] << " <midi_file>\n";
        return 1;
    }
    
    std::string filename = argv[1];
    std::cout << "Loading MIDI file: " << filename << std::endl;
    
    // Initialize SDL for audio and event handling
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create a small window for keyboard input
    SDL_Window* window = SDL_CreateWindow(
        "MIDI Player", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        640, 
        240, 
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Initialize audio system
    if (!Audio::init()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize MIDI player
    MidiPlayer player;
    if (!player.init()) {
        std::cerr << "Failed to initialize MIDI player" << std::endl;
        Audio::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Load the MIDI file
    if (!player.loadFile(filename)) {
        std::cerr << "Failed to load MIDI file: " << filename << std::endl;
        Audio::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Display help information
    printHelp();
    
    // Start playing
    player.play();
    
    // Main loop
    bool quit = false;
    SDL_Event e;
    
    while (!quit && player.isPlaying()) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        quit = true;
                        break;
                    
                    case SDLK_SPACE:
                        player.togglePause();
                        break;
                    
                    case SDLK_PLUS:
                    case SDLK_EQUALS:
                        player.increaseVolume();
                        break;
                    
                    case SDLK_MINUS:
                        player.decreaseVolume();
                        break;
                    
                    case SDLK_n:
                        player.toggleNormalization();
                        break;
                }
            }
        }
        
        // Update player state
        player.update();
        
        // Simple status display in the console
        if (player.isPlaying()) {
            static int counter = 0;
            if (counter++ % 100 == 0) {
                if (player.isPaused()) {
                    std::cout << "\rPaused | Volume: " << player.getVolume() << "%   " << std::flush;
                } else {
                    std::cout << "\rPlaying | Volume: " << player.getVolume() << "%   " << std::flush;
                }
            }
        }
        
        // Don't hog the CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Stop playback
    player.stop();
    
    std::cout << "\nPlayback finished!" << std::endl;
    
    // Clean up
    Audio::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
