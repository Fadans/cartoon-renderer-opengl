#version 450

// Ombre = aplat sombre. On écrit AUSSI la normale du sol dans le G-buffer :
// ainsi le post-process ne voit ni saut de normale ni saut de profondeur à la
// frontière sol/ombre → pas de contour noir autour de l'ombre (juste une tache).
uniform vec3 shadow_color;      // couleur de l'ombre
uniform vec3 ground_view_normal; // normale du sol en espace caméra [-1,1]

layout(location = 0) out vec4 output_color;
layout(location = 1) out vec4 output_normal;

void main() {
    output_color  = vec4(shadow_color, 1.0);
    output_normal = vec4(normalize(ground_view_normal) * 0.5 + 0.5, 1.0);
}
