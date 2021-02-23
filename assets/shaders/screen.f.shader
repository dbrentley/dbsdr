#version 330 core

in vec2 f_TexCoords;

uniform sampler2D tex;

out vec4 color;

void main() {
    color = texture(tex, f_TexCoords);

    // invert colors
    //color = vec4(vec3(1.0 - texture(tex, f_TexCoords)), 1.0);
}
