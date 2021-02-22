#version 330 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec4 in_Color;
layout (location = 2) in vec2 in_TexCoords;

uniform mat4 mvp;

out vec4 f_Color;
out vec2 f_TexCoords;

void main() {
    gl_Position = mvp * vec4(in_Position, 1.0);
    f_Color = in_Color;
    f_TexCoords = in_TexCoords;
}