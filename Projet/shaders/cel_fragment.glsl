#version 450

uniform sampler2D tex_diffuse;
uniform vec3  light_dir;
uniform vec3  light_color;
uniform vec3  eye_position;

uniform float num_bands;
uniform float ambient;
uniform float spec_threshold;
uniform float spec_strength;
uniform bool  use_texture;
uniform vec4  object_color;
uniform bool  use_hatch;
uniform float hatch_thr;
uniform bool  two_sided;

layout(location = 0) out vec4 output_color;
layout(location = 1) out vec4 output_normal;

in vec3 frag_pos;
in vec3 frag_normal;
in vec3 view_normal;
in vec2 frag_uv;

float hatch_layer(vec2 fc, vec2 dir, float spacing, float thick) {
    float v = mod(dot(fc, dir), spacing);
    return 1.0 - step(thick, v);
}

float hatching(float tone) {
    vec2 fc = gl_FragCoord.xy;
    vec2 d1 = vec2(0.7071,  0.7071);
    vec2 d2 = vec2(0.7071, -0.7071);
    float h = 0.0;
    if (tone < 0.80 * hatch_thr) h = max(h, hatch_layer(fc, d1, 9.0, 1.6));
    if (tone < 0.55 * hatch_thr) h = max(h, hatch_layer(fc, d2, 9.0, 1.6));
    if (tone < 0.30 * hatch_thr) h = max(h, hatch_layer(fc, d1, 4.5, 1.6));
    if (tone < 0.15 * hatch_thr) h = max(h, hatch_layer(fc, d2, 4.5, 1.6));
    return h;
}

void main() {
    vec3 n = normalize(frag_normal);
    vec3 v = normalize(eye_position - frag_pos);
    if (two_sided && dot(n, v) < 0.0)
        n = -n;
    vec3 l = normalize(light_dir);
    vec3 r = reflect(-l, n);

    float diff = max(dot(n, l), 0.0);
    bool  smooth_shade = num_bands >= 64.0;
    float cel  = smooth_shade ? diff : floor(diff * num_bands) / num_bands;

    float spec_raw = pow(max(dot(r, v), 0.0), 32.0);
    float spec     = (smooth_shade ? spec_raw : step(spec_threshold, spec_raw)) * spec_strength;

    spec *= step(0.0, dot(n, l));

    vec3 base = use_texture ? texture(tex_diffuse, frag_uv).rgb : object_color.rgb;

    vec3 result = base * (ambient + cel) * light_color
                + vec3(1.0) * spec * light_color;

    if (use_hatch)
        result *= mix(1.0, 0.35, hatching(diff));

    output_color = vec4(clamp(result, 0.0, 1.0), 1.0);

    output_normal = vec4(normalize(view_normal) * 0.5 + 0.5, 1.0);
}
