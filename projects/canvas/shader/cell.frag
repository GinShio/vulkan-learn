// Cellular Noise
// https://thebookofshaders.com/12

#version 450 core

layout(push_constant, std430) uniform PushConstantObject {
    int millisec;
    vec2 extent;
    vec2 mouse;
};

layout(location = 0) out vec4 color;

const int kPointNum = 6;

void main() {
    vec2 scale = {extent.x / extent.y, 1.f};
    vec2 st = gl_FragCoord.xy / extent * scale;

    // Cell positions
    vec2 point[kPointNum];
    point[0] = vec2(0.92, 0.75);
    point[1] = vec2(0.74, 0.37);
    point[2] = vec2(0.28, 0.64);
    point[3] = vec2(0.31, 0.26);
    point[4] = vec2(1.2f, 0.25f);
    point[5] = mouse / extent * scale;

    float m_dist = 1.;  // minimum distance
    // Iterate through the points positions
    for (int i = 0; i < kPointNum; i++) {
        float dist = distance(st, point[i]);
        m_dist = min(m_dist, dist); // Keep the closer distance
    }

    // Draw the min distance (distance field)
    color = vec4(vec3(m_dist), 1.f);
}
