#ifndef PARROT_H
#define PARROT_H

typedef struct {
    double mouth_open;        // 0.0 to 1.0
    double blink_timer;       // Timer for blinking
    gboolean eye_closed;      // Is eye currently closed
} ParrotState;

#endif
