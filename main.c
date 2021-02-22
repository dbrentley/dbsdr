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

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 640
#define DEFAULT_SAMPLE_RATE 20000000
#define DEFAULT_FREQUENCY 860721500
#define DEFAULT_LNA_GAIN 24
#define DEFAULT_VGA_GAIN 24
#define FFT_SIZE 8192 // max BYTES_PER_TRANSFER

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

    mat4x4 m, p;
    mat4x4_identity(m);
    mat4x4_ortho(p, -aspect, aspect, -1.0f, 1.0f, 1.0f, -1.0f);
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
    Timer timer;
    timer.frame_count = 0;

    if (!glfwInit()) {
        return -1;
    }

    queue_init(&mag_line_queue);
    fft_init(FFT_SIZE);

    device_init();
    device_set_sample_rate(DEFAULT_SAMPLE_RATE);
    device_set_frequency(DEFAULT_FREQUENCY);
    device_set_lna_gain(DEFAULT_LNA_GAIN);
    device_set_vga_gain(DEFAULT_VGA_GAIN);
    device_rx(receive_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // glfwWindowHint(GLFW_SAMPLES, 4); // anti-alias

    window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "TradesKill", NULL,
            NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

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

    //game_state->cursor = custom_cursor(window);
    game_state->should_close = 0;
    game_state->window_state->resizing = 0;
    game_state->window_state->width = DEFAULT_WIDTH;
    game_state->window_state->height = DEFAULT_HEIGHT;
    game_state->window_state->aspect =
            (float) DEFAULT_WIDTH / (float) DEFAULT_HEIGHT;
    game_state->window_state->aspect = 1.0f;

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
            // position       color                   uv
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // lr 0
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, // ul 1
            1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, // ur 2
            -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f  // ll 3
    };
    int elements[] = {
            2, 1, 0,
            0, 1, 3
    };

    int position_size = 3;
    int color_size = 4;
    int uv_size = 2;

    GLuint vbo, vao, vertex_shader, fragment_shader, shader_program;
    GLuint tex, ebo;
    int IsCompiled_VS, IsCompiled_FS;
    int maxLength;
    char *vertexInfoLog;
    char *fragmentInfoLog;

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

    // position
    glVertexAttribPointer(0, position_size, GL_FLOAT, GL_FALSE,
            9 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    // color
    glVertexAttribPointer(1, color_size, GL_FLOAT, GL_FALSE,
            9 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // uv
    glVertexAttribPointer(2, uv_size, GL_FLOAT, GL_FALSE,
            9 * sizeof(float), (void *) (7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // load texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int tex_width, tex_height, tex_channels;
    unsigned char *data = stbi_load("assets/img/test.png", &tex_width,
            &tex_height, &tex_channels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load texture.\n");
        exit(-1);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    // shader
    const GLchar *vertex_shader_source = file_to_buffer(
            "assets/shaders/default.v.shader");
    // fragment
    const GLchar *fragment_shader_source = file_to_buffer(
            "assets/shaders/default.f.shader");

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &IsCompiled_VS);
    if (IsCompiled_VS == GL_FALSE) {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);

        /* The maxLength includes the NULL character */
        vertexInfoLog = (char *) malloc(maxLength);

        glGetShaderInfoLog(vertex_shader, maxLength, &maxLength, vertexInfoLog);

        /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
        /* In this simple program, we'll just leave */
        fprintf(stderr, "Vertex: %s\n", vertexInfoLog);
        free(vertexInfoLog);
        exit(-1);
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &IsCompiled_FS);
    if (IsCompiled_FS == GL_FALSE) {
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &maxLength);

        /* The maxLength includes the NULL character */
        fragmentInfoLog = (char *) malloc(maxLength);

        glGetShaderInfoLog(fragment_shader, maxLength, &maxLength,
                fragmentInfoLog);

        /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
        /* In this simple program, we'll just leave */
        fprintf(stderr, "Fragment: %s\n", fragmentInfoLog);
        free(fragmentInfoLog);
        exit(-1);
    }

    shader_program = glCreateProgram();

    glAttachShader(shader_program, fragment_shader);
    glAttachShader(shader_program, vertex_shader);
    // bind attribute index 0 (coordinates) to in_Position
    glBindAttribLocation(shader_program, 0, "in_Position");
    // bind attribute index 1 (colors) to in_Color
    glBindAttribLocation(shader_program, 1, "in_Color");
    // link the program. attribute binding must happen before this
    glLinkProgram(shader_program);
    free((char *) vertex_shader_source);
    free((char *) fragment_shader_source);

    GLint mvp_uniform = glGetUniformLocation(shader_program, "mvp");

    set_aspect(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    timer.previous_time = glfwGetTime();
    while (!game_state->should_close && !glfwWindowShouldClose(window) &&
            device_is_alive()) {
        timer.time = glfwGetTime();
        timer.start_time = timer.time;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw

        // bind shader
        glUseProgram(shader_program);
        if (game_state->window_state->update_aspect) {
            glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE,
                    (const GLfloat *) game_state->window_state->mvp);
            game_state->window_state->update_aspect = 0;
        }

        // upload texture to shader
        glBindTexture(GL_TEXTURE_2D, tex);

        // bind vao
        glBindVertexArray(vao);

        // enable vertex attribute pointers
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        // draw the elements
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);



        // update
        for (int i = 0; i < timer.fps; i++) {
            float *line = queue_pop(&mag_line_queue);
            if (line) {
                unsigned int points_per_pixel =
                        FFT_SIZE / game_state->window_state->width;
                for (unsigned int j = 0; j < game_state->window_state->width;
                        j++) {
                    float sum = 0;
                    for (unsigned int k = 0; k < points_per_pixel; k++) {
                        sum += line[j * points_per_pixel + k];
                    }

                    sum /= (float) points_per_pixel;
                    uint8_t gray = interpolate(sum, 0.0, -128, 1.0, 128) * 255;
                    uint8_t intensity = color_intensity(gray);
                    //fprintf(stdout, "%hhu\n", intensity);
                }
                //waterfall_newline(&waterfall, line, FFT_SIZE);
                free(line);
            }
        }


        // unbind
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glBindVertexArray(0);
        glUseProgram(0);

        // swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
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

    glfwDestroyCursor(game_state->cursor);
    glfwDestroyWindow(window);
    glfwTerminate();

    free(game_state->mouse_state);
    game_state->mouse_state = NULL;
    free(game_state->window_state);
    game_state->window_state = NULL;
    free(game_state);
    game_state = NULL;

    device_destroy();
    queue_destroy(&mag_line_queue);

    return 0;
}
