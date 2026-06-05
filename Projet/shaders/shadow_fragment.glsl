#version 450

uniform vec3  shadow_color;
uniform vec3  ground_view_normal;
uniform float ground_half;

layout(location = 0) out vec4 output_color;
layout(location = 1) out vec4 output_normal;

in vec2 ground_xz;

void main() {

    if (ground_half > 0.0 && (abs(ground_xz.x) > ground_half || abs(ground_xz.y) > ground_half))
        discard;

    output_color  = vec4(shadow_color, 1.0);
    output_normal = vec4(normalize(ground_view_normal) * 0.5 + 0.5, 1.0);
}
