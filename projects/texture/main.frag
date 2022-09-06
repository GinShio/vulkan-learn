#version 450 core

layout(location = 1) in vec2 coord;

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D rin;
layout(binding = 1) uniform sampler2D len;

void main() {
    color = texture(len, coord);
}
