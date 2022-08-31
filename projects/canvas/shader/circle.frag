// Draw a circle
// https://www.nolife.cyou/how-to-draw-a-circle-with-glsl/

#version 450 core

layout(push_constant) uniform PushConstantObject {
    int millisec;
    vec2 extent;
} pco;

layout(location = 0) out vec4 frag_color;

const float radius = 0.25f;
const float stroke = 0.01f;

vec3 get_circle(vec2 center, vec2 coord) {
    float dist = distance(coord, center);
    float base_radius = step(radius, dist);
    return vec3(base_radius);
}

vec3 get_ring(vec2 center, vec2 coord) {
    float dist = distance(coord, center);
    float base_radius = step(radius, dist);
    float little_radius = step(radius - stroke, dist);
    return vec3(1.f - (little_radius - base_radius));
}

void main() {
    vec2 scale = {pco.extent.x / pco.extent.y, 1.f};
    vec2 coord = gl_FragCoord.xy / pco.extent.xy * scale; // normalized
    vec2 center = vec2(0.5f, 0.5f) * scale;
    vec2 timed_center = {center.x + 0.05f / scale.x * cos(radians(pco.millisec)),
                         center.y + 0.05f / scale.y * sin(radians(pco.millisec))};

    // vec3 color = get_ring(center, coord);
    vec3 color = get_ring(timed_center, coord);

    frag_color = vec4(color, 1.f);
}
