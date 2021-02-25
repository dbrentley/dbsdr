#ifndef DBSDR_GLOBAL_H
#define DBSDR_GLOBAL_H

#include "linmath.h"
#include "mouse.h"
#include "queue.h"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 640
#define DEFAULT_SAMPLE_RATE 20000000
#define DEFAULT_FREQUENCY 106120000 // 860721500 //
#define DEFAULT_LNA_GAIN 24
#define DEFAULT_VGA_GAIN 24
#define FFT_SIZE 8192 // max BYTES_PER_TRANSFER
#define WATERFALL_WIDTH 1280
#define WATERFALL_HEIGHT 640

typedef struct sdr_state {
    int64_t frequency;
    float *last_line;
} sdr_state_t;

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
    sdr_state_t *sdr_state;
} game_state_t;

extern game_state_t *game_state;
extern queue_t mag_line_queue;

#endif // DBSDR_GLOBAL_H