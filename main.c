#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "global.h"
#include "linmath.h"
#include "device.h"
#include "queue.h"
#include "fft.h"
#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <libhackrf/hackrf.h>
#include <stb/stb_image.h>
#include <time.h>

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 640
#define DEFAULT_SAMPLE_RATE 20000000
#define DEFAULT_FREQUENCY 860721500
#define DEFAULT_LNA_GAIN 24
#define DEFAULT_VGA_GAIN 24
#define FFT_SIZE 8192 // max BYTES_PER_TRANSFER

#define TW (8192)

game_state_t *game_state;
queue_t mag_line_queue;

void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

void receive_callback(void *samples, size_t n_samples,
        size_t bytes_per_sample) {
    int8_t *buff = (int8_t *) samples;
    size_t ffts = n_samples / FFT_SIZE;

    // TODO store in a buffer for fft size larger than n samples

    for (size_t i = 0; i < ffts; i++) {
        float *line = fft(buff + i * FFT_SIZE * bytes_per_sample);
        queue_append(&mag_line_queue, line);
    }
}

void set_aspect(int width, int height) {
    float aspect = (float) width / (float) height;
    glViewport(0, 0, width, height);
    gluOrtho2D(0.0f, (float) width, (float) height, 0.0f);
    float zoom = game_state->window_state->zoom;

    mat4x4 m, p;
    mat4x4_identity(m);
    mat4x4_ortho(p, -aspect * zoom, aspect * zoom, -zoom, zoom, 1.0f, -1.0f);
    mat4x4_mul(game_state->window_state->mvp, p, m);

    game_state->window_state->width = width;
    game_state->window_state->height = height;
    game_state->window_state->aspect = aspect;
    game_state->window_state->update_aspect = 1;
}

void resize_callback(GLFWwindow *window, int width, int height) {
    set_aspect(width, height);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
        int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        game_state->should_close = 1;
    }
}

double interpolate(double val, double y0, double x0, double y1, double x1) {
    return (val - x0) * (y1 - y0) / (x1 - x0) + y0;
}

uint32_t color_intensity(uint8_t intensity) {
    return 255 << 24 | intensity << 16 | intensity << 8 | intensity;
}

int main() {
    GLFWwindow *window;
    game_timer_t timer;
    timer.frame_count = 0;

    if (!glfwInit()) {
        return -1;
    }

    queue_init(&mag_line_queue);
    fft_init(FFT_SIZE);

    game_state = malloc(sizeof(game_state_t));
    if (game_state == NULL) {
        fprintf(stderr, "Could not create game state\n");
        exit(-1);
    }
    game_state->window_state = malloc(sizeof(window_state_t));
    if (game_state->window_state == NULL) {
        fprintf(stderr, "Could not create game state - window state\n");
        exit(-1);
    }
    game_state->mouse_state = malloc(sizeof(mouse_state_t));
    if (game_state->mouse_state == NULL) {
        fprintf(stderr, "Could not create game state - mouse position\n");
        exit(-1);
    }
    game_state->sdr_state = malloc(sizeof(sdr_state_t));
    if (game_state->sdr_state == NULL) {
        fprintf(stderr, "Could not create game state - sdr state\n");
        exit(-1);
    }

    //game_state->cursor = custom_cursor(window);
    game_state->should_close = 0;
    game_state->window_state->resizing = 0;
    game_state->window_state->width = DEFAULT_WIDTH;
    game_state->window_state->height = DEFAULT_HEIGHT;
    game_state->window_state->aspect =
            (float) DEFAULT_WIDTH / (float) DEFAULT_HEIGHT;
    game_state->window_state->aspect = 1.0f;
    game_state->window_state->zoom = 500.0f;
    game_state->sdr_state->frequency = DEFAULT_FREQUENCY;

    device_init();
    device_set_sample_rate(DEFAULT_SAMPLE_RATE);
    device_set_frequency(game_state->sdr_state->frequency);
    device_set_lna_gain(DEFAULT_LNA_GAIN);
    device_set_vga_gain(DEFAULT_VGA_GAIN);
    device_rx(receive_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    //glfwWindowHint(GLFW_SAMPLES, 4); // anti-alias

    window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "TradesKill", NULL,
            NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetErrorCallback(error_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // v-sync

    glewExperimental = GL_TRUE;
    glewInit();
    const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte *version = glGetString(GL_VERSION); // version as a string
    fprintf(stderr, "Renderer: %s\n", renderer);
    fprintf(stderr, "OpenGL version supported %s\n", version);

    // enable depth testing
    glEnable(GL_DEPTH_TEST);
    // depth-testing interprets a smaller value as "closer"
    glDepthFunc(GL_LESS);

    // https://www.khronos.org/opengl/wiki/Tutorial2:_VAOs,_VBOs,_Vertex_and_Fragment_Shaders_(C_/_SDL)
    float vertex_buffer[] = {
            // position  uv
            500.0f, -500.0f, 1.0f, 1.0f, // lr 0
            -500.0f, 500.0f, 0.0f, 0.0f, // ul 1
            500.0f, 500.0f, 1.0f, 0.0f,  // ur 2
            -500.0f, -500.0f, 0.0f, 1.0f // ll 3
    };
    int elements[] = {
            2, 1, 0,
            0, 1, 3
    };

    int position_size = 2;
    int uv_size = 2;

    GLuint vbo, vao, tex, ebo, pbo;

    // voa
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // vbo
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer), vertex_buffer,
            GL_STATIC_DRAW);

    // ebo
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
            GL_STATIC_DRAW);

    // pbo ---------------------------------------------------------------------
    //float pixels[] = { [0 ... (4 * (TW * 1)) - 1] = 0.0f };
    //pixels[0] = 1.0f;
    float *pixels = calloc(TW * 4, sizeof(float));
    pixels[0] = 1.0f;

    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glBufferData(GL_PIXEL_UNPACK_BUFFER, TW * 4 * sizeof(float), 0,
            GL_STREAM_DRAW);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TW, 1, 0,
            GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    void *mapped_buffer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);
    if (mapped_buffer == NULL) {
        fprintf(stderr, "Could not create mapped buffer\n");
        exit(-1);
    }
    memcpy(mapped_buffer, pixels, TW * 4 * sizeof(float));
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    // -------------------------------------------------------------------------

    // position
    glVertexAttribPointer(0, position_size, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    // uv
    glVertexAttribPointer(1, uv_size, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
            (void *) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint default_program;
    default_program = shader_program_create("assets/shaders/default.v.shader",
            "assets/shaders/default.f.shader");
    shader_program_bind_attribute_location(default_program, 0, "in_Position");
    shader_program_bind_attribute_location(default_program, 1, "in_Color");
    shader_program_link(default_program);
    GLint mvp_uniform = shader_program_get_uniform_location(default_program,
            "mvp");


    set_aspect(game_state->window_state->width,
            game_state->window_state->height);
    timer.previous_time = glfwGetTime();
    while (!game_state->should_close && !glfwWindowShouldClose(window) &&
            device_is_alive()) {
        timer.time = glfwGetTime();
        timer.start_time = timer.time;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // process input

        // bind shader
        glUseProgram(default_program);
        if (game_state->window_state->update_aspect) {
            glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE,
                    (const GLfloat *) game_state->window_state->mvp);
            set_aspect(game_state->window_state->width,
                    game_state->window_state->height);
            device_set_frequency(game_state->sdr_state->frequency);
            fprintf(stderr, "Frequency: %ld\n",
                    game_state->sdr_state->frequency);
            game_state->window_state->update_aspect = 0;
        }
        glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, tex);
        // update
        for (int i = 0; i < timer.fps; i++) {
            float *line = queue_pop(&mag_line_queue);
            if (line != NULL) {
                //printf("here %d\n", timer.frame_count);
                unsigned int points_per_pixel =
                        FFT_SIZE / TW;
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                memset(pixels, 0, TW * 4 * sizeof(float));
                int cnt = 0;
                for (unsigned int j = 0; j < TW; j++) {
                    float sum = 0;
                    for (unsigned int k = 0; k < points_per_pixel; k++) {
                        sum += line[j * points_per_pixel + k];
                    }

                    sum /= (float) points_per_pixel;
                    uint8_t gray = interpolate(sum, 0.0, -128, 1.0, 0) * 255;
                    uint8_t intensity = color_intensity(gray);
                    if (cnt <= TW * 4) {
                        pixels[cnt] = (float) intensity / 255.0f;
                        pixels[cnt + 1] = (float) intensity / 255.0f;
                        pixels[cnt + 2] = (float) intensity / 255.0f;
                        pixels[cnt + 3] = 1.0f;
                    }
                    cnt += 4;
                }
                memcpy(mapped_buffer, pixels, TW * 4 * sizeof(float));
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                //waterfall_newline(&waterfall, line, FFT_SIZE);
                free(line);
            }
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        // poll for events
        glfwPollEvents();

        // swap buffers
        glfwSwapBuffers(window);

        timer.end_time = timer.time;
        timer.delta = timer.end_time - timer.start_time;
        timer.frame_count++;
        if (timer.time - timer.previous_time >= 1.0) {
            fprintf(stderr, "FPS: %i, Queue size: %zu\n", timer.fps,
                    queue_size(&mag_line_queue));
            timer.fps = timer.frame_count;
            timer.frame_count = 0;
            timer.previous_time = timer.time;
        }
    }

    device_destroy();
    queue_destroy(&mag_line_queue);

    glfwDestroyCursor(game_state->cursor);
    glfwDestroyWindow(window);
    glfwTerminate();

    free(game_state->mouse_state);
    game_state->mouse_state = NULL;
    free(game_state->window_state);
    game_state->window_state = NULL;
    free(game_state->sdr_state);
    game_state->sdr_state = NULL;
    free(game_state);
    game_state = NULL;


    return 0;
}
