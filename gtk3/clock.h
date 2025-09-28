#define MAX_SWIRL_PARTICLES 100
#define DIGIT_WIDTH 4
#define DIGIT_HEIGHT 5

typedef struct {
    double x, y;               // Position
    double angle;              // Current angle around clock center
    double radius;             // Distance from center
    double angular_velocity;   // How fast it spins
    double radial_velocity;    // How fast it moves in/out
    double life;               // Life remaining (1.0 to 0.0)
    double intensity;          // Audio intensity that created it
    double hue;                // Color hue
    double size;               // Particle size
    gboolean active;           // Is particle active
    int frequency_band;        // Which frequency band created it
} SwirlParticle;

// 4x5 dot matrix patterns for digits 0-9
// 1 = dot on, 0 = dot off
static const int digit_patterns[10][DIGIT_HEIGHT][DIGIT_WIDTH] = {
    // 0
    {{1,1,1,1},
     {1,0,0,1},
     {1,0,0,1},
     {1,0,0,1},
     {1,1,1,1}},
    // 1
    {{0,0,1,0},
     {0,1,1,0},
     {0,0,1,0},
     {0,0,1,0},
     {0,1,1,1}},
    // 2
    {{1,1,1,1},
     {0,0,0,1},
     {1,1,1,1},
     {1,0,0,0},
     {1,1,1,1}},
    // 3
    {{1,1,1,1},
     {0,0,0,1},
     {0,1,1,1},
     {0,0,0,1},
     {1,1,1,1}},
    // 4
    {{1,0,0,1},
     {1,0,0,1},
     {1,1,1,1},
     {0,0,0,1},
     {0,0,0,1}},
    // 5
    {{1,1,1,1},
     {1,0,0,0},
     {1,1,1,1},
     {0,0,0,1},
     {1,1,1,1}},
    // 6
    {{1,1,1,1},
     {1,0,0,0},
     {1,1,1,1},
     {1,0,0,1},
     {1,1,1,1}},
    // 7
    {{1,1,1,1},
     {0,0,0,1},
     {0,0,1,0},
     {0,1,0,0},
     {1,0,0,0}},
    // 8
    {{1,1,1,1},
     {1,0,0,1},
     {1,1,1,1},
     {1,0,0,1},
     {1,1,1,1}},
    // 9
    {{1,1,1,1},
     {1,0,0,1},
     {1,1,1,1},
     {0,0,0,1},
     {1,1,1,1}}
};
