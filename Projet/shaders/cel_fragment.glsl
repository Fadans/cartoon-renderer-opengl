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
uniform bool  use_hatch;       // hachures de croquis dans les zones sombres

layout(location = 0) out vec4 output_color;
layout(location = 1) out vec4 output_normal;  // G-buffer normales (espace caméra) pour le post-process

in vec3 frag_pos;
in vec3 frag_normal;
in vec3 view_normal;
in vec2 frag_uv;

// ── Hachures (croquis) ──────────────────────────────────────────────────────
// Une couche de lignes diagonales en espace écran : renvoie 1.0 si le pixel
// tombe sur un trait, 0.0 sinon. `dir` = orientation, `spacing`/`thick` en px.
float hatch_layer(vec2 fc, vec2 dir, float spacing, float thick) {
    float v = mod(dot(fc, dir), spacing);
    return 1.0 - step(thick, v);
}

// Couverture de hachures selon le niveau d'éclairement `tone` (0 sombre → 1 clair).
// Plus c'est sombre, plus on empile de couches croisées (densité ↑).
float hatching(float tone) {
    vec2 fc = gl_FragCoord.xy;
    vec2 d1 = vec2(0.7071,  0.7071);   // diagonale ↗
    vec2 d2 = vec2(0.7071, -0.7071);   // diagonale ↘ (croisement)
    float h = 0.0;
    if (tone < 0.80) h = max(h, hatch_layer(fc, d1, 9.0, 1.6));
    if (tone < 0.55) h = max(h, hatch_layer(fc, d2, 9.0, 1.6));
    if (tone < 0.30) h = max(h, hatch_layer(fc, d1, 4.5, 1.6));
    if (tone < 0.15) h = max(h, hatch_layer(fc, d2, 4.5, 1.6));
    return h;   // 0 = pas de trait, 1 = sur un trait
}

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

    // Hachures : assombrit le pixel sur les traits, densité ∝ obscurité (effet croquis).
    // `diff` (éclairement continu, non quantifié) pilote la densité des couches.
    if (use_hatch)
        result *= mix(1.0, 0.35, hatching(diff));

    output_color = vec4(clamp(result, 0.0, 1.0), 1.0);

    // Normale espace caméra encodée [-1,1] → [0,1] pour la passe de contour
    output_normal = vec4(normalize(view_normal) * 0.5 + 0.5, 1.0);
}
