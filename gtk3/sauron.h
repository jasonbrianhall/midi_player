#ifndef SAURON_H
#define SAURON_H

typedef struct {
    double pupil_size;
    double iris_rotation;
    double blink_timer;
    double blink_duration;
    gboolean is_blinking;
    double look_x, look_y;  // Where the eye is looking
    double look_target_x, look_target_y;
    double intensity_pulse;
    double fire_particles[20][4];  // x, y, life, angle for flame particles
    int fire_particle_count;
} EyeOfSauronState;

#endif
