//
// Created by dbrent on 2/19/21.
//

#ifndef DBSDR_MOUSE_H
#define DBSDR_MOUSE_H

#include <GLFW/glfw3.h>

void show_cursor(GLFWwindow *window);

void hide_cursor(GLFWwindow *window);

GLFWcursor *custom_cursor(GLFWwindow *window);

void cursor_position_callback(GLFWwindow *window, double x_pos, double y_pos);

void scroll_callback(GLFWwindow *window, double x_offset, double y_offset);

#endif //DBSDR_MOUSE_H
