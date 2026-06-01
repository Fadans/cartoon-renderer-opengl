#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model_view;
uniform mat4 projection;
uniform float outline_width;   // épaisseur en unités view space (0.03)

void main() {
    // Expansion le long de la normale en view space
    // → épaisseur visuellement constante quelle que soit la distance
    vec4 pos_view  = model_view * vec4(position, 1.0);
    vec3 norm_view = normalize(mat3(model_view) * normal);
    pos_view.xyz  += norm_view * outline_width;
    gl_Position    = projection * pos_view;
}
