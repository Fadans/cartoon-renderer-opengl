#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

uniform mat4 model;        // objet → monde  (pour l'éclairage)
uniform mat4 model_view;   // objet → caméra (pour gl_Position)
uniform mat4 projection;

out vec3 frag_pos;     // espace monde
out vec3 frag_normal;  // espace monde
out vec2 frag_uv;

void main() {
    gl_Position = projection * model_view * vec4(position, 1.0);

    // Espace monde : la lumière est fixe dans le monde, pas dans l'objet
    frag_pos    = vec3(model * vec4(position, 1.0));
    frag_normal = normalize(mat3(model) * normal);
    frag_uv     = uv;
}
