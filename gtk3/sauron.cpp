// eye_of_sauron.cpp

#include "visualization.h"

void init_eye_of_sauron(Visualizer *vis) {
    vis->eye_of_sauron.pupil_size = 0.3;
    vis->eye_of_sauron.iris_rotation = 0.0;
    vis->eye_of_sauron.blink_timer = 0.0;
    vis->eye_of_sauron.blink_duration = 0.0;
    vis->eye_of_sauron.is_blinking = FALSE;
    vis->eye_of_sauron.look_x = 0.0;
    vis->eye_of_sauron.look_y = 0.0;
    vis->eye_of_sauron.look_target_x = 0.0;
    vis->eye_of_sauron.look_target_y = 0.0;
    vis->eye_of_sauron.intensity_pulse = 0.0;
    vis->eye_of_sauron.fire_particle_count = 0;
}

void update_eye_of_sauron(Visualizer *vis, double dt) {
    EyeOfSauronState *eye = &vis->eye_of_sauron;
    
    // Pupil dilation based on volume
    double target_pupil = 0.2 + vis->volume_level * 0.4;
    eye->pupil_size += (target_pupil - eye->pupil_size) * dt * 5.0;
    
    // Iris rotation based on music
    eye->iris_rotation += dt * (0.5 + vis->volume_level * 2.0);
    
    // Random blinking
    eye->blink_timer += dt;
    if (eye->blink_timer > 3.0 + ((double)rand() / RAND_MAX) * 4.0) {
        eye->is_blinking = TRUE;
        eye->blink_duration = 0.0;
        eye->blink_timer = 0.0;
    }
    
    if (eye->is_blinking) {
        eye->blink_duration += dt;
        if (eye->blink_duration > 0.2) {
            eye->is_blinking = FALSE;
        }
    }
    
    // Eye "looking" around based on frequency bands
    double look_force_x = 0.0;
    double look_force_y = 0.0;
    
    for (int i = 0; i < VIS_FREQUENCY_BARS; i++) {
        double angle = (double)i / VIS_FREQUENCY_BARS * 2.0 * M_PI;
        look_force_x += cos(angle) * vis->frequency_bands[i];
        look_force_y += sin(angle) * vis->frequency_bands[i];
    }
    
    eye->look_target_x = look_force_x * 30.0;
    eye->look_target_y = look_force_y * 30.0;
    
    // Smooth eye movement
    eye->look_x += (eye->look_target_x - eye->look_x) * dt * 3.0;
    eye->look_y += (eye->look_target_y - eye->look_y) * dt * 3.0;
    
    // Intensity pulse on beats
    if (vis->volume_level > 0.3) {
        eye->intensity_pulse = 1.0;
    }
    if (eye->intensity_pulse > 0) {
        eye->intensity_pulse -= dt * 2.0;
        if (eye->intensity_pulse < 0) eye->intensity_pulse = 0;
    }
    
    // Update fire particles around the eye
    for (int i = 0; i < eye->fire_particle_count; i++) {
        eye->fire_particles[i][2] -= dt * 0.5;  // life decay
        eye->fire_particles[i][1] -= dt * 50.0;  // rise up
        eye->fire_particles[i][3] += dt * 2.0;   // rotate
    }
    
    // Spawn new fire particles on high volume
    if (vis->volume_level > 0.2 && eye->fire_particle_count < 20) {
        int idx = eye->fire_particle_count++;
        double angle = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
        eye->fire_particles[idx][0] = angle;  // store angle
        eye->fire_particles[idx][1] = 0.0;    // y offset
        eye->fire_particles[idx][2] = 1.0;    // life
        eye->fire_particles[idx][3] = 0.0;    // rotation
    }
    
    // Remove dead particles
    for (int i = 0; i < eye->fire_particle_count; i++) {
        if (eye->fire_particles[i][2] <= 0) {
            // Swap with last particle
            eye->fire_particles[i][0] = eye->fire_particles[eye->fire_particle_count - 1][0];
            eye->fire_particles[i][1] = eye->fire_particles[eye->fire_particle_count - 1][1];
            eye->fire_particles[i][2] = eye->fire_particles[eye->fire_particle_count - 1][2];
            eye->fire_particles[i][3] = eye->fire_particles[eye->fire_particle_count - 1][3];
            eye->fire_particle_count--;
            i--;
        }
    }
}

void draw_eye_of_sauron(Visualizer *vis, cairo_t *cr) {
    double center_x = vis->width / 2.0;
    double center_y = vis->height / 2.0;
    double eye_width = fmin(vis->width, vis->height) * 0.4;
    double eye_height = eye_width * 0.6;
    
    EyeOfSauronState *eye = &vis->eye_of_sauron;
    
    // Draw fire particles around eye
    for (int i = 0; i < eye->fire_particle_count; i++) {
        double angle = eye->fire_particles[i][0];
        double y_offset = eye->fire_particles[i][1];
        double life = eye->fire_particles[i][2];
        
        double radius = eye_width * 0.6;
        double px = center_x + cos(angle) * radius;
        double py = center_y + sin(angle) * (radius * 0.5) + y_offset;
        
        // Fire colors: yellow to red to black
        double r = 1.0;
        double g = life * 0.5;
        double b = 0.0;
        
        cairo_set_source_rgba(cr, r, g, b, life * 0.6);
        cairo_arc(cr, px, py, 3.0 + life * 4.0, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // Outer glow (orange/red)
    double glow_intensity = 0.3 + eye->intensity_pulse * 0.4 + vis->volume_level * 0.3;
    cairo_pattern_t *glow = cairo_pattern_create_radial(
        center_x + eye->look_x, center_y + eye->look_y, eye_height * 0.3,
        center_x + eye->look_x, center_y + eye->look_y, eye_height * 1.2);
    
    cairo_pattern_add_color_stop_rgba(glow, 0, 1.0, 0.3, 0.0, glow_intensity);
    cairo_pattern_add_color_stop_rgba(glow, 0.5, 0.8, 0.1, 0.0, glow_intensity * 0.5);
    cairo_pattern_add_color_stop_rgba(glow, 1, 0.2, 0.0, 0.0, 0);
    
    cairo_set_source(cr, glow);
    cairo_arc(cr, center_x + eye->look_x, center_y + eye->look_y, 
              eye_height * 1.2, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(glow);
    
    // Eye white/sclera (yellowish, bloodshot)
    if (!eye->is_blinking) {
        cairo_save(cr);
        cairo_translate(cr, center_x + eye->look_x, center_y + eye->look_y);
        cairo_scale(cr, 1.0, 0.6);
        
        cairo_pattern_t *sclera = cairo_pattern_create_radial(0, 0, 0, 0, 0, eye_width);
        cairo_pattern_add_color_stop_rgba(sclera, 0, 0.9, 0.8, 0.6, 0.95);
        cairo_pattern_add_color_stop_rgba(sclera, 1, 0.6, 0.5, 0.3, 0.9);
        cairo_set_source(cr, sclera);
        cairo_arc(cr, 0, 0, eye_width, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(sclera);
        
        cairo_restore(cr);
        
        // Iris (fiery orange with rotating patterns)
        cairo_save(cr);
        cairo_translate(cr, center_x + eye->look_x, center_y + eye->look_y);
        
        double iris_radius = eye_height * 0.8;
        
        // Rotating iris pattern
        for (int i = 0; i < 12; i++) {
            double angle = eye->iris_rotation + (i / 12.0) * 2.0 * M_PI;
            double x1 = cos(angle) * iris_radius * 0.3;
            double y1 = sin(angle) * iris_radius * 0.3;
            double x2 = cos(angle) * iris_radius;
            double y2 = sin(angle) * iris_radius;
            
            cairo_pattern_t *ray = cairo_pattern_create_linear(x1, y1, x2, y2);
            cairo_pattern_add_color_stop_rgba(ray, 0, 1.0, 0.5, 0.0, 0.8);
            cairo_pattern_add_color_stop_rgba(ray, 1, 0.6, 0.2, 0.0, 0.3);
            
            cairo_set_source(cr, ray);
            cairo_set_line_width(cr, iris_radius * 0.15);
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
            cairo_stroke(cr);
            cairo_pattern_destroy(ray);
        }
        
        // Iris base color
        cairo_pattern_t *iris = cairo_pattern_create_radial(0, 0, 0, 0, 0, iris_radius);
        cairo_pattern_add_color_stop_rgba(iris, 0, 1.0, 0.4, 0.0, 0.9);
        cairo_pattern_add_color_stop_rgba(iris, 0.7, 0.8, 0.2, 0.0, 0.8);
        cairo_pattern_add_color_stop_rgba(iris, 1, 0.3, 0.1, 0.0, 0.9);
        cairo_set_source(cr, iris);
        cairo_arc(cr, 0, 0, iris_radius, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(iris);
        
        // Pupil (black void with red glow)
        double pupil_radius = eye_height * eye->pupil_size;
        
        cairo_pattern_t *pupil = cairo_pattern_create_radial(0, 0, 0, 0, 0, pupil_radius * 1.5);
        cairo_pattern_add_color_stop_rgba(pupil, 0, 0.0, 0.0, 0.0, 1.0);
        cairo_pattern_add_color_stop_rgba(pupil, 0.7, 0.0, 0.0, 0.0, 1.0);
        cairo_pattern_add_color_stop_rgba(pupil, 1, 0.3, 0.0, 0.0, 0.0);
        cairo_set_source(cr, pupil);
        cairo_arc(cr, 0, 0, pupil_radius * 1.5, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(pupil);
        
        // Inner pupil shine/reflection
        cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 0.3);
        cairo_arc(cr, -pupil_radius * 0.3, -pupil_radius * 0.3, pupil_radius * 0.2, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_restore(cr);
    } else {
        // Blinking - draw closed eyelid
        double blink_progress = eye->blink_duration / 0.2;
        if (blink_progress > 0.5) blink_progress = 1.0 - blink_progress;
        blink_progress *= 2.0;  // 0 to 1 and back
        
        cairo_save(cr);
        cairo_translate(cr, center_x + eye->look_x, center_y + eye->look_y);
        cairo_scale(cr, 1.0, 1.0 - blink_progress * 0.95);
        
        cairo_set_source_rgba(cr, 0.3, 0.2, 0.1, 0.9);
        cairo_arc(cr, 0, 0, eye_width, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_restore(cr);
    }
    
    // Eye outline/lids
    cairo_save(cr);
    cairo_translate(cr, center_x + eye->look_x, center_y + eye->look_y);
    cairo_scale(cr, 1.0, 0.6);
    
    cairo_set_source_rgba(cr, 0.2, 0.1, 0.0, 0.9);
    cairo_set_line_width(cr, 4.0);
    cairo_arc(cr, 0, 0, eye_width, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    cairo_restore(cr);
}
