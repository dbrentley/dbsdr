#version 330 core

in vec2 f_TexCoords;

uniform sampler2D tex;

out vec4 color;

void main() {
    color = texture(tex, f_TexCoords);
}
