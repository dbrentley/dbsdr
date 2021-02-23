//
// Created by dbrent on 2/20/21.
//

#ifndef DBSDR_SHADER_H
#define DBSDR_SHADER_H

#include <GL/glew.h>

char *file_to_buffer(char *file);

GLuint shader_create(GLenum type, char *file);

GLuint shader_program_create(char *vertex_shader_file,
        char *fragment_shader_file);

void shader_program_bind_attribute_location(GLuint program, GLuint index,
        const GLchar *name);

GLint shader_program_get_uniform_location(GLuint program, const GLchar *name);

void shader_program_link(GLuint program);

#endif //DBSDR_SHADER_H
