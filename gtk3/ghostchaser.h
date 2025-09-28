#define GHOST_CHASER_MAZE_WIDTH 25
#define GHOST_CHASER_MAZE_HEIGHT 15
#define MAX_GHOST_CHASER_PELLETS 300
#define MAX_GHOST_CHASER_GHOSTS 6
#define GHOST_CHASER_GHOST_COLORS 4

typedef enum {
    CHASER_WALL = 1,
    CHASER_PELLET = 2,
    CHASER_POWER_PELLET = 3,
    CHASER_EMPTY = 0
} ChaserCellType;

typedef enum {
    CHASER_UP = 0,
    CHASER_DOWN = 1,
    CHASER_LEFT = 2,
    CHASER_RIGHT = 3
} ChaserDirection;

typedef struct {
    double x, y;              // Smooth position (can be fractional)
    int grid_x, grid_y;       // Grid position
    ChaserDirection direction; // Current direction
    ChaserDirection next_direction; // Queued direction change
    double mouth_angle;       // For animation
    double size_multiplier;   // Audio-reactive size
    gboolean moving;          // Is currently moving
    double speed;             // Movement speed
    double beat_pulse;        // Pulse effect on beats
} ChaserPlayer;

typedef struct {
    double x, y;              // Smooth position
    int grid_x, grid_y;       // Grid position
    ChaserDirection direction; // Current direction
    int color_index;          // 0=red, 1=pink, 2=cyan, 3=orange
    double hue;               // Current color hue
    double target_hue;        // Target color based on audio
    double size_multiplier;   // Audio-reactive size
    double speed;             // Movement speed
    double scared_timer;      // How long scared state lasts
    gboolean scared;          // Is in scared state
    gboolean visible;         // Is currently visible
    double blink_timer;       // For blinking effect
    int frequency_band;       // Which frequency band controls this ghost
    double audio_intensity;   // Current audio level affecting this ghost
} ChaserGhost;

typedef struct {
    int grid_x, grid_y;       // Grid position
    gboolean active;          // Is pellet still there
    double pulse_phase;       // For pulsing animation
    double size_multiplier;   // Audio-reactive size
    gboolean is_power_pellet; // Is this a power pellet
    double hue;               // Color hue
    int frequency_band;       // Which frequency affects this pellet
} ChaserPellet;

// Simplified maze layout (1=wall, 0=empty, 2=pellet, 3=power pellet)
static const int ghost_chaser_maze_template[GHOST_CHASER_MAZE_HEIGHT][GHOST_CHASER_MAZE_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,3,1,1,1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,2,1,1,1,3,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,2,1},
    {1,2,2,2,2,2,1,2,2,2,2,2,1,2,2,2,2,2,1,2,2,2,2,2,1},
    {1,1,1,1,1,2,1,1,1,1,1,0,1,0,1,1,1,1,1,2,1,1,1,1,1},
    {0,0,0,0,1,2,1,0,0,0,0,0,0,0,0,0,0,0,1,2,1,0,0,0,0},
    {1,1,1,1,1,2,1,1,1,1,1,0,1,0,1,1,1,1,1,2,1,1,1,1,1},
    {1,2,2,2,2,2,1,2,2,2,2,2,1,2,2,2,2,2,1,2,2,2,2,2,1},
    {1,2,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,3,1,1,1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,2,1,1,1,3,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

