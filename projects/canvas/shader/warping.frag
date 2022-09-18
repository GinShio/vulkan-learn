// Fractal Brownian Motion
// https://thebookofshaders.com/13

#version 450 core

layout(push_constant, std430) uniform PushConstantObject {
    float time;
    vec2 extent;
};

layout(location = 0) out vec4 color;

const int kOctaves = 6;
const vec2 shift = vec2(100.0);
const mat2 rot = {{cos(.5f), sin(.5f)}, {-sin(.5f), cos(.5f)}};

float hash(in vec2 st) {
    return fract(
        sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise(in vec2 st) {
    vec2 temp = floor(st);
    float a = hash(temp);
    float b = hash(temp + vec2(1.f, 0.f));
    float c = hash(temp + vec2(0.f, 1.f));
    float d = hash(temp + vec2(1.f, 1.f));
    vec2 u = smoothstep(0.f, 1.f, fract(st));
    return mix(a, b, u.x) +
        (c - a) * u.y * (1.f - u.x) +
        (d - b) * u.x * u.y;
}

float fbm(in vec2 st) {
    float value = 0.f;
    float amplitude = .5f;
    for (int i = 0; i < kOctaves; i++) {
        value += amplitude * noise(st);
        st = rot * st * 2.0 + shift;
        amplitude *= .5f;
    }
    return value;
}

void main() {
    vec2 scale = {extent.x / extent.y, 1.f};
    vec2 p = gl_FragCoord.xy / extent * scale;
    vec2 q = {fbm(p), fbm(p + vec2(1.f))};
    vec2 r = {fbm(p + 2.f * q + vec2(1.7f, 9.2f) + 0.15f * time),
              fbm(p + 2.f * q + vec2(8.3f, 2.8f) + 0.25f * time)};
    float f = fbm(p + 2.f * r);

    vec3 cloud = mix(vec3(0.101961, 0.619608, 0.666667),
                     vec3(0.666667, 0.666667, 0.498039),
                     clamp((f * f) * 4.f, 0.f, 1.f));
    cloud = mix(cloud,
                vec3(0.165, 0.054, 0.091),
                clamp(length(q), 0.f, 1.f));
    cloud = mix(cloud,
                vec3(0.666667, 1.f, 1.f),
                clamp(length(r.x), 0.f, 1.f));
    color = vec4(cloud * (f * f * f + .6f * f * f+ .5f * f), 1.f);
}
