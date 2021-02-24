//
// Created by dbrent on 2/19/21.
//

#include "mouse.h"
#include "global.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void hide_cursor(GLFWwindow *window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void show_cursor(GLFWwindow *window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

GLFWcursor *custom_cursor(GLFWwindow *window) {
    unsigned char cursor_pixels[16 * 16 * 4];
    memset(cursor_pixels, 0xff, sizeof(cursor_pixels));
    GLFWimage cursor_image;
    cursor_image.width = 16;
    cursor_image.height = 16;
    cursor_image.pixels = cursor_pixels;

    GLFWcursor *cursor = glfwCreateCursor(&cursor_image, 0, 0);
    if (cursor == NULL) {
        fprintf(stderr, "Could not create cursor\n");
        exit(-1);
    }
    glfwSetCursor(window, cursor);
    return cursor;
}

void cursor_position_callback(GLFWwindow *window, double x_pos, double y_pos) {
    if (x_pos < 0) {
        x_pos = 0;
    }
    if (x_pos > game_state->window_state->width) {
        x_pos = game_state->window_state->width;
    }
    if (y_pos < 0) {
        y_pos = 0;
    }
    if (x_pos > game_state->window_state->height) {
        x_pos = game_state->window_state->height;
    }
    game_state->mouse_state->x_pos = x_pos;
    game_state->mouse_state->y_pos = y_pos;
}

void scroll_callback(GLFWwindow *window, double x_offset, double y_offset) {
    game_state->mouse_state->scroll_x_offset = x_offset;
    game_state->mouse_state->scroll_y_offset = y_offset;
    if (y_offset == -1.0f) {
        game_state->sdr_state->frequency += 1000000;
//        if (game_state->window_state->zoom < 30.0f) {
//        game_state->window_state->zoom += 1.0f;
//        }
    } else {
        game_state->sdr_state->frequency -= 1000000;
//        if (game_state->window_state->zoom > 1.0f) {
//        game_state->window_state->zoom -= 1.0f;
//        }
    }
    game_state->window_state->update_aspect = 1;
}