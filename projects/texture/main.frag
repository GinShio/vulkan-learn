#version 450 core

layout(location = 1) in vec2 coord;
layout(location = 2) flat in int is_rin;

layout(location = 0) out vec4 color;

layout(set = 1, binding = 0) uniform sampler2D rin;
layout(set = 1, binding = 1) uniform sampler2D len;

void main() {
    if (is_rin != 0) {
        color = texture(rin, coord);
    } else {
        color = texture(len, coord);
    }
}
