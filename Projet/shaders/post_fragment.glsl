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

uniform vec2  u_texel;          // 1 / taille du framebuffer
uniform vec3  u_outline_color;  // couleur du trait (noir)
uniform float u_depth_thr;      // seuil profondeur (silhouettes)
uniform float u_normal_thr;     // seuil normale (plis internes)
uniform float u_thickness;      // épaisseur du trait (en pixels) : écarte les voisins

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
    vec3 base = texture(u_color, frag_uv).rgb;

    // Écart d'échantillonnage = épaisseur du trait (en pixels)
    vec2 dx = vec2(u_texel.x * u_thickness, 0.0);
    vec2 dy = vec2(0.0, u_texel.y * u_thickness);

    // ── Bords par la profondeur (4 voisins) ────────────────────────────────
    float dc = read_depth(frag_uv);
    float depth_edge = abs(dc - read_depth(frag_uv + dx))
                     + abs(dc - read_depth(frag_uv - dx))
                     + abs(dc - read_depth(frag_uv + dy))
                     + abs(dc - read_depth(frag_uv - dy));

    // ── Bords par la normale (4 voisins) ───────────────────────────────────
    vec3 nc = read_normal(frag_uv);
    float normal_edge = (1.0 - dot(nc, read_normal(frag_uv + dx)))
                      + (1.0 - dot(nc, read_normal(frag_uv - dx)))
                      + (1.0 - dot(nc, read_normal(frag_uv + dy)))
                      + (1.0 - dot(nc, read_normal(frag_uv - dy)));

    float edge = max(step(u_depth_thr,  depth_edge),
                     step(u_normal_thr, normal_edge));

    output_color = vec4(mix(base, u_outline_color, edge), 1.0);
}
