#version 450

uniform sampler2D tex_diffuse;
uniform vec3  light_position;
uniform vec3  light_color;
uniform vec3  eye_position;

uniform float num_bands;       // paliers de diffus (4 = cartoon, 64 = quasi-Phong)
uniform float ambient;         // lumière ambiante (0.15)
uniform float spec_threshold;  // seuil spéculaire binaire (0.5)
uniform float spec_strength;   // intensité spéculaire (0.6)
uniform float rim_power;       // exposant rim (3.0)
uniform float rim_threshold;   // seuil rim (0.3)
uniform vec3  rim_color;       // couleur du rim light
uniform bool  use_texture;     // false = couleur uniforme
uniform vec4  object_color;    // couleur si pas de texture

layout(location = 0) out vec4 output_color;

in vec3 frag_pos;
in vec3 frag_normal;
in vec2 frag_uv;

void main() {
    vec3 n = normalize(frag_normal);
    vec3 l = normalize(light_position - frag_pos);
    vec3 v = normalize(eye_position   - frag_pos);
    vec3 r = reflect(-l, n);

    // ── Cel diffus : discrétisation en paliers ──────────────────────────
    float diff = max(dot(n, l), 0.0);
    float cel  = floor(diff * num_bands) / num_bands;

    // ── Spéculaire binaire (step) ───────────────────────────────────────
    float spec_raw = pow(max(dot(r, v), 0.0), 32.0);
    float spec     = step(spec_threshold, spec_raw) * spec_strength;

    // ── Rim lighting (halo sur les bords) ───────────────────────────────
    float rim_raw  = 1.0 - max(dot(v, n), 0.0);
    float rim      = step(rim_threshold, pow(rim_raw, rim_power)) * 0.8;

    // ── Couleur de base (texture ou uniforme) ────────────────────────────
    vec3 base = use_texture ? texture(tex_diffuse, frag_uv).rgb : object_color.rgb;

    vec3 result = base * (ambient + cel) * light_color
                + vec3(1.0) * spec * light_color
                + rim_color * rim;

    output_color = vec4(clamp(result, 0.0, 1.0), 1.0);
}
