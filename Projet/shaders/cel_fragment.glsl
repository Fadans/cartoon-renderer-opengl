#version 450

uniform sampler2D tex_diffuse;
uniform vec3  light_dir;       // direction VERS la lumière (soleil directionnel)
uniform vec3  light_color;
uniform vec3  eye_position;

uniform float num_bands;       // paliers de diffus (4 = cartoon, 64 = quasi-Phong)
uniform float ambient;         // lumière ambiante (0.15)
uniform float spec_threshold;  // seuil spéculaire binaire (0.5)
uniform float spec_strength;   // intensité spéculaire (0.6)
uniform bool  use_texture;     // false = couleur uniforme
uniform vec4  object_color;    // couleur si pas de texture

layout(location = 0) out vec4 output_color;
layout(location = 1) out vec4 output_normal;  // G-buffer normales (espace caméra) pour le post-process

in vec3 frag_pos;
in vec3 frag_normal;
in vec3 view_normal;
in vec2 frag_uv;

void main() {
    vec3 n = normalize(frag_normal);
    vec3 l = normalize(light_dir);                 // directionnel : constant partout
    vec3 v = normalize(eye_position - frag_pos);
    vec3 r = reflect(-l, n);

    // ── Cel diffus : discrétisation en paliers ──────────────────────────
    float diff = max(dot(n, l), 0.0);
    float cel  = floor(diff * num_bands) / num_bands;

    // ── Spéculaire binaire (step) ───────────────────────────────────────
    float spec_raw = pow(max(dot(r, v), 0.0), 32.0);
    float spec     = step(spec_threshold, spec_raw) * spec_strength;
    // Bridé au côté éclairé : pas de reflet sur une face dans l'ombre
    // (sinon le point blanc déborde sur la zone sombre vue de derrière).
    spec *= step(0.0, dot(n, l));

    // ── Couleur de base (texture ou uniforme) ────────────────────────────
    vec3 base = use_texture ? texture(tex_diffuse, frag_uv).rgb : object_color.rgb;

    // Rim light retiré : le contour post-process assure déjà la silhouette
    vec3 result = base * (ambient + cel) * light_color
                + vec3(1.0) * spec * light_color;

    output_color = vec4(clamp(result, 0.0, 1.0), 1.0);

    // Normale espace caméra encodée [-1,1] → [0,1] pour la passe de contour
    output_normal = vec4(normalize(view_normal) * 0.5 + 0.5, 1.0);
}
