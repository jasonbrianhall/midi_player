// main.cpp
// Main program for SDL-based MIDI player (CLI version)

#include "midiplayer.h"
#include "audio.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

// Global flag for handling termination
bool quit = false;

// Handle termination signals
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    quit = true;
}

void printHelp() {
    std::cout << "MIDI Player with OPL3 Synthesis via SDL\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Controls:\n";
    std::cout << "  +/-   - Increase/Decrease Volume\n";
    std::cout << "  n     - Toggle Volume Normalization\n";
    std::cout << "  p     - Pause/Resume\n";
    std::cout << "  q     - Quit\n";
    std::cout << "------------------------------------------\n";
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    
    return 0;
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
    
    // Initialize SDL for audio only
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Initialize audio system
    if (!Audio::init()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Initialize MIDI player
    MidiPlayer player;
    if (!player.init()) {
        std::cerr << "Failed to initialize MIDI player" << std::endl;
        Audio::shutdown();
        SDL_Quit();
        return 1;
    }
    
    // Load the MIDI file
    if (!player.loadFile(filename)) {
        std::cerr << "Failed to load MIDI file: " << filename << std::endl;
        Audio::shutdown();
        SDL_Quit();
        return 1;
    }
    
    // Display help information
    printHelp();
    
    // Start playing
    player.play();
    
    // Set up terminal for non-blocking input
    system("stty -echo raw");
    
    // Main loop
    while (!quit && player.isPlaying()) {
        // Check for keyboard input (non-blocking)
        if (kbhit()) {
            int ch = getchar();
            switch (ch) {
                case 'q':
                case 'Q':
                    quit = true;
                    break;
                
                case 'p':
                case 'P':
                    player.togglePause();
                    break;
                
                case '+':
                case '=':
                    player.increaseVolume();
                    break;
                
                case '-':
                case '_':
                    player.decreaseVolume();
                    break;
                
                case 'n':
                case 'N':
                    player.toggleNormalization();
                    break;
            }
        }
        
        // Update player state
        player.update();
        
        // Simple status display in the console
        static int counter = 0;
        if (counter++ % 100 == 0) {
            if (player.isPaused()) {
                std::cout << "\rPaused | Volume: " << player.getVolume() << "%   " << std::flush;
            } else {
                std::cout << "\rPlaying | Volume: " << player.getVolume() << "%   " << std::flush;
            }
        }
        
        // Don't hog the CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Restore terminal settings
    system("stty echo -raw");
    
    // Stop playback
    player.stop();
    
    std::cout << "\nPlayback finished!" << std::endl;
    
    // Clean up
    Audio::shutdown();
    SDL_Quit();
    
    return 0;
}
