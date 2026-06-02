#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model_view;
uniform mat4 projection;
uniform float outline_width;

void main() {
    vec4 pos_view  = model_view * vec4(position, 1.0);
    vec3 norm_view = normalize(mat3(model_view) * normal);

    // 1. Projeter la position normalement
    gl_Position = projection * pos_view;

    // 2. Projeter la normale en espace clip (w=0 = vecteur direction, pas un point)
    //    → donne la direction de la normale en 2D écran
    vec2 screen_normal = normalize((projection * vec4(norm_view, 0.0)).xy);

    // 3. Déplacer en XY clip space uniquement
    //    × gl_Position.w annule la division perspective → largeur constante à l'écran
    gl_Position.xy += screen_normal * outline_width * gl_Position.w;
}
