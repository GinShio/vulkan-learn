// Draw a circle
// https://www.nolife.cyou/how-to-draw-a-circle-with-glsl/

#version 450 core

layout(constant_id = 1) const float radius = 0.1f;
layout(constant_id = 2) const float stroke = 0.01f;

layout(push_constant, std430) uniform PushConstantObject {
    int millisec;
    vec2 extent;
} pco;

layout(location = 0) out vec4 frag_color;

vec3 get_circle(vec2 center, vec2 coord) {
    float dist = distance(coord, center);
    float base_radius = step(dist, radius);
    return vec3(base_radius);
}

vec3 get_ring(vec2 center, vec2 coord) {
    float dist = distance(coord, center);
    float base_radius = step(dist, radius);
    float little_radius = step(dist, radius - stroke);
    return vec3(base_radius - little_radius);
}

void main() {
    vec2 scale = {pco.extent.x / pco.extent.y, 1.f};
    vec2 coord = gl_FragCoord.xy / pco.extent.xy * scale; // normalized
    vec2 center = vec2(0.5f, 0.5f) * scale;
    vec2 timed_center = {center.x + 0.08f / scale.x * cos(radians(pco.millisec / 8)),
                         center.y + 0.08f / scale.y * sin(radians(pco.millisec / 8))};

    vec3 circle = get_circle(center, coord);
    vec3 ring = get_ring(timed_center, coord);
    vec3 color = ring + circle;

    frag_color = vec4(color, 1.f);
}
