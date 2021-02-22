#version 330 core

in vec4 f_Color;
in vec2 f_TexCoords;

uniform sampler2D tex;

out vec4 color;

void main() {
    color = texture(tex, f_TexCoords);
    //color = vec4(f_Color);
}
