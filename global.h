#ifndef DBSDR_GLOBAL_H
#define DBSDR_GLOBAL_H

#include "mouse.h"
#include "linmath.h"

typedef struct mouse_state {
    double x_pos;
    double y_pos;
    double scroll_x_offset;
    double scroll_y_offset;
} mouse_state_t;

typedef struct window_state {
    float aspect;
    float scale;
    float zoom;
    int width;
    int height;
    int update_aspect;
    int resizing;
    mat4x4 mvp;
} window_state_t;

typedef struct game_timer {
    double time;
    double previous_time;
    double start_time;
    double end_time;
    double delta;
    int frame_count;
    int fps;
} game_timer_t;

typedef struct game_state {
    int should_close;
    GLFWcursor *cursor;
    window_state_t *window_state;
    mouse_state_t *mouse_state;
} game_state_t;


extern game_state_t *game_state;

#endif // DBSDR_GLOBAL_H