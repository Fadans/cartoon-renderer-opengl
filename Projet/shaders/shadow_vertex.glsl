#version 450

// Ombre projetée planaire : la position objet est d'abord passée en monde
// (model), aplatie sur le plan du sol (shadow), puis projetée caméra.
// `mvp` = tout replié pour le rendu ; `world` = shadow*model seul, pour récupérer
// la position MONDE (sur le sol) et clipper l'ombre aux bornes du terrain.
layout(location = 0) in vec3 position;

uniform mat4 mvp;     // projection * view * shadow * model
uniform mat4 world;   // shadow * model (position aplatie en espace monde)

out vec2 ground_xz;   // position de l'ombre sur le plan du sol (x,z)

void main() {
    vec4 w = world * vec4(position, 1.0);
    ground_xz = w.xz / w.w;
    gl_Position = mvp * vec4(position, 1.0);
}
