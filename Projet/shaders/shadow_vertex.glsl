#version 450

// Ombre projetée planaire : la position objet est d'abord passée en monde
// (model), aplatie sur le plan du sol (shadow), puis projetée caméra.
// Tout est replié dans une seule matrice `mvp` calculée côté CPU.
layout(location = 0) in vec3 position;

uniform mat4 mvp;   // projection * view * shadow * model

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
