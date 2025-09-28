#define MAX_FIREWORKS 20
#define MAX_PARTICLES_PER_FIREWORK 50
#define MAX_TOTAL_PARTICLES 1000

typedef struct {
    double x, y;           // Position
    double vx, vy;         // Velocity
    double ax, ay;         // Acceleration (gravity)
    double life;           // Life remaining (1.0 to 0.0)
    double max_life;       // Initial life span
    double size;           // Particle size
    double r, g, b;        // Color
    double brightness;     // Brightness multiplier
    gboolean active;       // Is particle alive
    int trail_length;      // Length of particle trail
    double trail_x[10], trail_y[10]; // Trail positions
} FireworkParticle;

typedef struct {
    double x, y;           // Launch position
    double target_x, target_y; // Explosion point
    double vx, vy;         // Velocity
    double life;           // Life until explosion
    double explosion_size; // Size of explosion
    double hue;            // Base color hue
    int particle_count;    // Number of particles to spawn
    gboolean exploded;     // Has it exploded yet
    gboolean active;       // Is firework active
    int frequency_band;    // Which frequency band triggered it
} Firework;
