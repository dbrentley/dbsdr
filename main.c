#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "device.h"
#include "fft.h"
#include "game_state.h"
#include "global.h"
#include "linmath.h"
#include "queue.h"
#include "shader.h"

#include <libhackrf/hackrf.h>
#include <pthread.h>
#include <stb/stb_image.h>
#include <stdio.h>
#include <stdlib.h>

game_state_t *game_state;
queue_t mag_line_queue;

void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

void receive_callback(void *samples, size_t n_samples,
                      size_t bytes_per_sample) {
    int8_t *buff = (int8_t *)samples;
    size_t ffts = n_samples / FFT_SIZE;

    // TODO store in a buffer for fft size larger than n samples

    for (size_t i = 0; i < ffts; i++) {
        float *line = fft(buff + i * FFT_SIZE * bytes_per_sample);
        queue_append(&mag_line_queue, line);
    }
}

void set_aspect(int width, int height) {
    float aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    gluOrtho2D(0.0f, (float)width, (float)height, 0.0f);
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

float float_rand(float min, float max) {
    float scale = rand() / (float)RAND_MAX; /* [0, 1.0] */
    return min + scale * (max - min);       /* [min, max] */
}

void set_pixel(float *location, unsigned int offset, float sum) {
    float value = DEFAULT_LNA_GAIN / fabsf(sum);

    location[offset] = value;
    location[offset + 1] = value;
    location[offset + 2] = value;
    location[offset + 3] = 1.0f;
}

void *queue_processor() {
    while (!game_state->should_close && device_is_alive()) {
        float *line = queue_pop(&mag_line_queue);
        if (line != NULL) {
            game_state->sdr_state->last_line = line;
        }
        free(line);
        // this shouldn't happen but just in case
        if (queue_size(&mag_line_queue) > 25000) {
            queue_pop(&mag_line_queue);
        }
    }
}

int main() {
    GLFWwindow *window;
    game_timer_t timer;
    timer.frame_count = 0;

    if (!glfwInit()) {
        return -1;
    }

    queue_init(&mag_line_queue);
    game_state = game_state_init();
    fft_init(FFT_SIZE);

    device_init();
    device_set_sample_rate(DEFAULT_SAMPLE_RATE);
    device_set_frequency(game_state->sdr_state->frequency);
    device_set_lna_gain(DEFAULT_LNA_GAIN);
    device_set_vga_gain(DEFAULT_VGA_GAIN);
    device_rx(receive_callback);

    pthread_t queue_processing_thread;
    int qpt_ret = pthread_create(&queue_processing_thread, NULL,
                                 queue_processor, NULL);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // glfwWindowHint(GLFW_SAMPLES, 4); // anti-alias

    window = glfwCreateWindow(game_state->window_state->width,
                              game_state->window_state->height, "dbsdr", NULL,
                              NULL);
    if (!window) {
        glfwTerminate();
        fprintf(stderr, "Could not create window\n");
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
    const GLubyte *version = glGetString(GL_VERSION);   // version as a string
    fprintf(stderr, "Renderer: %s\n", renderer);
    fprintf(stderr, "OpenGL version supported %s\n", version);

    // enable depth testing
    glEnable(GL_DEPTH_TEST);
    // depth-testing interprets a smaller value as "closer"
    glDepthFunc(GL_LESS);

    // https://www.khronos.org/opengl/wiki/Tutorial2:_VAOs,_VBOs,_Vertex_and_Fragment_Shaders_(C_/_SDL)
    float vertex_buffer[] = {
            // position  uv
            1.0f,  -1.0f, 1.0f, 1.0f, // lr 0
            -1.0f, 1.0f,  0.0f, 0.0f, // ul 1
            1.0f,  1.0f,  1.0f, 0.0f, // ur 2
            -1.0f, -1.0f, 0.0f, 1.0f  // ll 3
    };
    int elements[] = {2, 1, 0, 0, 1, 3};

    unsigned int position_size = 2;
    unsigned int uv_size = 2;

    GLuint vbo, vao, tex, ebo;

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

    // texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float *pixels =
            malloc(WATERFALL_WIDTH * WATERFALL_HEIGHT * 4 * sizeof(float));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WATERFALL_WIDTH, WATERFALL_HEIGHT,
                 0, GL_RGBA, GL_FLOAT, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    // position attribute pointer
    glVertexAttribPointer(0, position_size, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    // uv attribute pointer
    glVertexAttribPointer(1, uv_size, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(position_size * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint default_program =
            shader_program_create("assets/shaders/default.v.shader",
                                  "assets/shaders/default.f.shader");
    shader_program_bind_attribute_location(default_program, 0, "in_Position");
    shader_program_bind_attribute_location(default_program, 1, "in_Color");
    shader_program_link(default_program);
    GLint mvp_uniform =
            shader_program_get_uniform_location(default_program, "mvp");

    // game loop
    set_aspect(game_state->window_state->width,
               game_state->window_state->height);
    timer.previous_time = glfwGetTime();
    unsigned int one_line = (int)(WATERFALL_WIDTH * 4 * sizeof(float));
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
                               (const GLfloat *)game_state->window_state->mvp);
            set_aspect(game_state->window_state->width,
                       game_state->window_state->height);
            device_set_frequency(game_state->sdr_state->frequency);
            fprintf(stderr, "Frequency: %ld\n",
                    game_state->sdr_state->frequency);
            game_state->window_state->update_aspect = 0;
            one_line = (int)(WATERFALL_WIDTH * 4 * sizeof(float));
        }

        // update pixels
        memmove(pixels + (one_line), pixels, (WATERFALL_HEIGHT - 1) * one_line);
        float *line = game_state->sdr_state->last_line;
        unsigned int points_per_pixel = FFT_SIZE / WATERFALL_WIDTH;
        // one line/row of pixels
        for (unsigned int j = 0; j < WATERFALL_WIDTH; j++) {
            float sum = 0;
            for (unsigned int k = 0; k < points_per_pixel; k++) {
                sum += line[j * points_per_pixel + k];
            }
            sum /= (float)points_per_pixel;
            set_pixel(pixels, j * 4, sum);
        }
        glBindVertexArray(vao);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WATERFALL_WIDTH,
                        WATERFALL_HEIGHT, GL_RGBA, GL_FLOAT, pixels);
        glBindTexture(GL_TEXTURE_2D, tex);
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

    // cleanup
    pthread_join(queue_processing_thread, NULL);
    fprintf(stderr, "Queue processor thread exited with status %d\n", qpt_ret);
    device_destroy();
    queue_destroy(&mag_line_queue);
    glfwDestroyWindow(window);
    game_state_destroy(game_state);
    free(pixels);

    return 0;
}
