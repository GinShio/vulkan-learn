#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 in_coord;

layout(location = 1) out vec2 out_coord;

void main() {
    gl_Position = vec4(position, 0.f, 1.f);
    out_coord = in_coord;
}
