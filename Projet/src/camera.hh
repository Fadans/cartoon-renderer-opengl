#pragma once
#include "matrix4.hh"

class Camera {
public:
    float angle_h   =  0.3f;   // angle horizontal (radians)
    float angle_v   =  0.4f;   // angle vertical
    float distance  =  4.0f;   // distance au centre
    float speed     =  0.06f;  // sensibilité orbite
    float pan_speed =  0.10f;  // sensibilité pan

    mygl::vec3 center = {0, 0, 0};  // cible regardée (déplaçable au pan)

    // Matrices pour les uniforms
    mygl::matrix4 get_view() const;
    mygl::vec3    get_eye()  const;

    // Déplace la cible dans le plan écran (dx = droite, dy = haut)
    void pan(float dx, float dy);

    // Déplacement horizontal au sol (strafe = droite, forward = avant projeté en XZ)
    void move(float strafe, float forward);

    // Callbacks GLUT
    void on_key    (unsigned char key);  // ZQSD déplacer · A/E zoom · Espace/C vertical
    void on_special(int key);            // flèches = orbite
};
