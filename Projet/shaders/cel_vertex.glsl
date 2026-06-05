#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 model_view;
uniform mat4 projection;

out vec3 frag_pos;
out vec3 frag_normal;
out vec3 view_normal;
out vec2 frag_uv;

void main() {
    gl_Position = projection * model_view * vec4(position, 1.0);

    frag_pos    = vec3(model * vec4(position, 1.0));
    frag_normal = normalize(mat3(model) * normal);

    view_normal = normalize(mat3(model_view) * normal);
    frag_uv     = uv;
}
