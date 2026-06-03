#version 450

// ─── Passe de contour post-process (détection de bords) ─────────────────────
// Lit la scène cel-shadée + le G-buffer (normales, profondeur) rendus dans le
// FBO, détecte les arêtes (silhouettes ET plis internes) et trace le contour.
//
// - Discontinuité de PROFONDEUR  → silhouettes + recouvrements (tore qui se
//   superpose, objets devant le fond).
// - Discontinuité de NORMALE     → arêtes vives internes (jonction cap/flanc
//   du cylindre) là où la profondeur varie peu mais l'orientation casse.
//
// Avantage vs l'ancienne technique 2 passes : contour d'épaisseur constante,
// uniforme sur tous les objets, et qui gère proprement les arêtes vives.

uniform sampler2D u_color;    // unité 0 : scène cel-shadée
uniform sampler2D u_normal;   // unité 1 : normales espace caméra [0,1]
uniform sampler2D u_depth;    // unité 2 : depth buffer non-linéaire

uniform vec2  u_texel;            // 1 / taille du framebuffer
uniform vec3  u_outline_color;    // couleur du trait au premier plan (près)
uniform vec3  u_outline_color_far;// couleur du trait au loin (estompée)
uniform float u_depth_thr;        // seuil profondeur (silhouettes)
uniform float u_normal_thr;       // seuil normale (plis internes)
uniform float u_thickness;        // épaisseur du trait au premier plan (px)
uniform float u_thickness_far;    // facteur d'épaisseur au loin (×, 0..1)
uniform float u_dist_near;        // distance caméra où le trait est le plus épais
uniform float u_dist_far;         // distance caméra où le trait est le plus fin
uniform vec3  u_bg_top;           // couleur fond (haut)
uniform vec3  u_bg_bottom;        // couleur fond (bas)

// near/far du frustum (main.cpp : frustum(..., 1.0, 100.0))
const float NEAR = 1.0;
const float FAR  = 100.0;

in  vec2 frag_uv;
layout(location = 0) out vec4 output_color;

// Depth buffer [0,1] non-linéaire → distance caméra linéaire
float linearize(float d) {
    float z = d * 2.0 - 1.0;                       // NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
}

vec3 read_normal(vec2 uv) { return texture(u_normal, uv).xyz * 2.0 - 1.0; }
float read_depth(vec2 uv) { return linearize(texture(u_depth, uv).r); }

void main() {
    // Fond stylisé : sur les pixels d'arrière-plan (depth = far, jamais écrits
    // par la géométrie → depth brut ≈ 1.0) on remplace la couleur par un
    // gradient vertical. Le contour reste tracé aux silhouettes (cf. plus bas).
    float raw_depth = texture(u_depth, frag_uv).r;
    bool  is_bg     = raw_depth >= 0.9999;
    vec3  gradient  = mix(u_bg_bottom, u_bg_top, frag_uv.y);
    vec3  base      = is_bg ? gradient : texture(u_color, frag_uv).rgb;

    // ── Contour variable selon la profondeur ───────────────────────────────
    // Distance caméra du pixel central → facteur 0 (près) … 1 (loin). On en
    // déduit une épaisseur (près = épais, loin = fin) et une couleur (près =
    // trait franc, loin = trait estompé) → effet « poids de trait » dessiné.
    float dc  = read_depth(frag_uv);
    float dt  = clamp((dc - u_dist_near) / (u_dist_far - u_dist_near), 0.0, 1.0);
    float thickness   = mix(u_thickness, u_thickness * u_thickness_far, dt);
    vec3  outline_col = mix(u_outline_color, u_outline_color_far, dt);

    // Écart d'échantillonnage = épaisseur du trait (en pixels)
    vec2 dx = vec2(u_texel.x * thickness, 0.0);
    vec2 dy = vec2(0.0, u_texel.y * thickness);

    // ── Bords par la profondeur (dérivée SECONDE = Laplacien) ──────────────
    // On compare la profondeur à l'interpolation linéaire de ses voisins.
    // Un plan, même vu à angle rasant, a une profondeur linéaire → ce terme ≈ 0
    // (pas de faux contour sur le sol qui s'éloigne). Une vraie silhouette est
    // une discontinuité → terme grand → contour tracé.
    float dl = read_depth(frag_uv - dx), dr = read_depth(frag_uv + dx);
    float dd = read_depth(frag_uv - dy), du = read_depth(frag_uv + dy);
    float depth_edge = abs(dl + dr - 2.0 * dc) + abs(du + dd - 2.0 * dc);

    // ── Bords par la normale (4 voisins) ───────────────────────────────────
    vec3 nc = read_normal(frag_uv);
    float normal_edge = (1.0 - dot(nc, read_normal(frag_uv + dx)))
                      + (1.0 - dot(nc, read_normal(frag_uv - dx)))
                      + (1.0 - dot(nc, read_normal(frag_uv + dy)))
                      + (1.0 - dot(nc, read_normal(frag_uv - dy)));

    float edge = max(step(u_depth_thr,  depth_edge),
                     step(u_normal_thr, normal_edge));

    output_color = vec4(mix(base, outline_col, edge), 1.0);
}
