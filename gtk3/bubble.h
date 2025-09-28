#define MAX_BUBBLES 100
#define MAX_POP_EFFECTS 50

typedef struct {
    double x, y;           // Position
    double radius;         // Current radius
    double max_radius;     // Maximum radius before popping
    double velocity_x, velocity_y;  // Movement
    double life;           // Life remaining (0.0 - 1.0)
    double birth_time;     // When bubble was created
    double intensity;      // Audio intensity that created it
    gboolean active;       // Is this bubble alive?
} Bubble;

typedef struct {
    double x, y;           // Position where pop occurred
    double radius;         // Expanding ring radius
    double max_radius;     // Final ring radius
    double life;           // Effect life remaining
    double intensity;      // Original bubble intensity
    gboolean active;       // Is effect active?
} PopEffect;

