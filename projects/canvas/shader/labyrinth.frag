#version 450 core

layout(push_constant, std430) uniform PushConstantObject {
    float time;
    vec2 extent;
};

layout(location = 0) out vec4 color;

float hash(in vec2 st) {
    return fract(
        sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec2 truchet_pattern(in vec2 st, in float index) {
    index = fract((index - 0.5f) * 2.f);
    if (index > 0.75f) {
        st = vec2(1.f) - st;
    } else if (index > 0.5f) {
        st = vec2(1.f - st.x, st.y);
    } else if (index > 0.25f) {
        st = 1.f - vec2(1.f - st.x, st.y);
    }
    return st;
}

void main() {
    vec2 st = gl_FragCoord.xy / extent * 10.f;
    vec2 tile = truchet_pattern(fract(st), hash(floor(st)));
    float maze =
        smoothstep(tile.x - 0.3, tile.x, tile.y) -
        smoothstep(tile.x, tile.x + 0.3, tile.y);
    color = vec4(vec3(maze), 1.0);
}
