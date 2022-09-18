// Draw a circle
// https://www.nolife.cyou/how-to-draw-a-circle-with-glsl/

#version 450 core

layout(push_constant, std430) uniform PushConstantObject {
    float time;
    vec2 extent;
};

layout(location = 0) out vec4 color;

const float radius = 0.25f;
const float stroke = 0.01f;

vec3 get_circle(in vec2 center, in vec2 coord) {
    float dist = distance(coord, center);
    float base_radius = step(dist, radius);
    return vec3(base_radius);
}

vec3 get_ring(in vec2 center, in vec2 coord) {
    float dist = distance(coord, center);
    float base_radius = step(dist, radius);
    float little_radius = step(dist, radius - stroke);
    return vec3(base_radius - little_radius);
}

void main() {
    vec2 scale = {extent.x / extent.y, 1.f};
    vec2 coord = gl_FragCoord.xy / extent * scale; // normalized
    vec2 center = vec2(0.5f, 0.5f) * scale;
    vec2 timed_center = {center.x + 0.08f / scale.x * cos(time),
                         center.y + 0.08f / scale.y * sin(time)};

    vec3 circle = get_circle(center, coord);
    vec3 ring = get_ring(timed_center, coord);
    color = vec4(ring + circle, 1.f);
}
