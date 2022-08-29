// Draw a circle
// https://www.nolife.cyou/how-to-draw-a-circle-with-glsl/

#version 450 core

layout(push_constant) uniform PushConstantObject {
    vec2 extent;
} pco;

layout(location = 0) out vec4 frag_color;

void main() {
    // normalized
    vec2 scale = {pco.extent.x / pco.extent.y, 1.f};
    vec2 coord = gl_FragCoord.xy / pco.extent.xy * scale;
    vec2 offset = vec2(0.5f, 0.5f) * scale;
    // draw
    frag_color = vec4(vec3(step(distance(coord, offset), 0.25f)), 1.f);
}
