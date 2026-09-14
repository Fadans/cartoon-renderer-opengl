#version 450

uniform sampler2D u_color;
uniform sampler2D u_normal;
uniform sampler2D u_depth;

uniform vec2  u_texel;
uniform vec3  u_outline_color;
uniform vec3  u_outline_color_far;
uniform float u_depth_thr;
uniform float u_normal_thr;
uniform float u_thickness;
uniform float u_thickness_far;
uniform float u_dist_near;
uniform float u_dist_far;
uniform vec3  u_bg_top;
uniform vec3  u_bg_bottom;

const float NEAR = 1.0;
const float FAR  = 150.0;

in  vec2 frag_uv;
layout(location = 0) out vec4 output_color;

float linearize(float d) {
    float z = d * 2.0 - 1.0;
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
}

vec3 read_normal(vec2 uv) { return texture(u_normal, uv).xyz * 2.0 - 1.0; }
float read_depth(vec2 uv) { return linearize(texture(u_depth, uv).r); }

void main() {

    float raw_depth = texture(u_depth, frag_uv).r;
    bool  is_bg     = raw_depth >= 0.9999;
    vec3  gradient  = mix(u_bg_bottom, u_bg_top, frag_uv.y);
    vec3  base      = is_bg ? gradient : texture(u_color, frag_uv).rgb;

    float dc  = read_depth(frag_uv);
    float dt  = clamp((dc - u_dist_near) / (u_dist_far - u_dist_near), 0.0, 1.0);
    float thickness   = mix(u_thickness, u_thickness * u_thickness_far, dt);
    vec3  outline_col = mix(u_outline_color, u_outline_color_far, dt);

    vec2 dx = vec2(u_texel.x * thickness, 0.0);
    vec2 dy = vec2(0.0, u_texel.y * thickness);

    float dl = read_depth(frag_uv - dx), dr = read_depth(frag_uv + dx);
    float dd = read_depth(frag_uv - dy), du = read_depth(frag_uv + dy);
    float depth_edge = abs(dr - dl) + abs(du - dd);

    vec3 nc = read_normal(frag_uv);
    float normal_edge = (1.0 - dot(nc, read_normal(frag_uv + dx)))
                      + (1.0 - dot(nc, read_normal(frag_uv - dx)))
                      + (1.0 - dot(nc, read_normal(frag_uv + dy)))
                      + (1.0 - dot(nc, read_normal(frag_uv - dy)));

    float edge = max(step(u_depth_thr,  depth_edge),
                     step(u_normal_thr, normal_edge));

    output_color = vec4(mix(base, outline_col, edge), 1.0);
}
