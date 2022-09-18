#version 450 core

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 out_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 project;
};

layout(push_constant) uniform PushConstantObject {
    float c;
};

void main() {
    gl_Position = project * view * model * vec4(in_pos, 0.0, 1.0);
    out_color = c * in_color;
}
