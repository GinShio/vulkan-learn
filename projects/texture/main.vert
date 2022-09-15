#version 450 core

struct UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 project;
};

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 in_coord;

layout(location = 1) out vec2 out_coord;
layout(location = 2) out int is_rin;

layout(set = 0, binding = 0) uniform UBO_0_0 {
    UniformBufferObject rin;
};

layout(set = 0, binding = 1) uniform UBO_0_1 {
    UniformBufferObject len;
};

void main() {
    vec4 pos = vec4(position, 0.f, 1.f);
    out_coord = in_coord;
    is_rin = position.x > 0.f ? 1 : 0;
    if (is_rin != 0) {
        gl_Position = rin.project * rin.view * rin.model * pos;
    } else {
        gl_Position = len.project * len.view * len.model * pos;
    }

}
