// https://www.nolife.cyou/how-to-draw-a-circle-with-glsl/

#version 430 core

layout(constant_id = 0) const float kPI = 3.14f;
layout(constant_id = 1) const float kRadius = 0.1f;

layout(push_constant, std430) uniform PushConstantObject {
    int millisec;
    vec2 extent;
} pco;

layout(location = 0) out vec4 color;

float get_circle(vec2 offset, float radius) {
    float dist = length(offset);
    return step(dist, radius);
}

float get_arc(vec2 center, vec2 coord, float start, float end) {
    vec2 offset = coord - center;
    float angle = atan(offset.y, offset.x) / kPI;
    return step(start, angle) * step(angle, end) * get_circle(offset, kRadius);
}

void main() {
    vec2 scale = {pco.extent.x / pco.extent.y, 1.f};
    vec2 coord = gl_FragCoord.xy / pco.extent.xy * scale; // normalized
    vec2 center = vec2(.5f, .5f) * scale;
    const float r = radians(pco.millisec / 5);

    vec2 pacman_center = vec2(.75f, .5f) * scale;
    const float pacman_angle = .8f;
    const float pacman_pos = cos(r) * .5f + .5f;
    const float pacman_start = mix(-pacman_angle, -1.f, pacman_pos);
    const float pacman_end = mix(pacman_angle, 1.f, pacman_pos);
    float pacman = get_arc(pacman_center, coord, pacman_start, pacman_end);

    const float food_radius = .05f;
    const float food_start = .2f;
    const float food_end = .64f;
    const float PI_1o2 = .5f * kPI;
    const float PI_2 = 2.f * kPI;
    const float food_pos = smoothstep(PI_1o2, PI_2, mod(r, PI_2)) * (food_end - food_start) + food_start;
    vec2 food_center = vec2(food_pos, .5f) * scale;
    float food = get_circle(coord - food_center, food_radius) * step(PI_1o2, mod(r, PI_2));

    color = vec4(vec3(clamp(pacman + food, .0f, 1.f)), 1.f);
}
