#version 450

// Ombre = aplat sombre. On écrit AUSSI la normale du sol dans le G-buffer :
// ainsi le post-process ne voit ni saut de normale ni saut de profondeur à la
// frontière sol/ombre → pas de contour noir autour de l'ombre (juste une tache).
uniform vec3  shadow_color;       // couleur de l'ombre
uniform vec3  ground_view_normal; // normale du sol en espace caméra [-1,1]
uniform float ground_half;        // demi-côté du sol : on coupe l'ombre au-delà

layout(location = 0) out vec4 output_color;
layout(location = 1) out vec4 output_normal;

in vec2 ground_xz;

void main() {
    // L'ombre ne doit pas déborder du terrain (sinon elle flotte sur le fond).
    // Clip seulement si ground_half > 0 → rétro-compatible avec une app qui ne
    // définit pas cet uniform (ex. le binaire Zelda) : pas de clip dans ce cas.
    if (ground_half > 0.0 && (abs(ground_xz.x) > ground_half || abs(ground_xz.y) > ground_half))
        discard;

    output_color  = vec4(shadow_color, 1.0);
    output_normal = vec4(normalize(ground_view_normal) * 0.5 + 0.5, 1.0);
}
