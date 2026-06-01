#pragma once
#include "matrix4.hh"

class Camera {
public:
    float angle_h   =  0.3f;   // angle horizontal (radians)
    float angle_v   =  0.4f;   // angle vertical
    float distance  =  4.0f;   // distance au centre
    float speed     =  0.06f;  // sensibilité clavier

    mygl::vec3 center = {0, 0, 0};

    // Matrices pour les uniforms
    mygl::matrix4 get_view() const;
    mygl::vec3    get_eye()  const;

    // Callbacks GLUT
    void on_key    (unsigned char key);  // WASD + Q/E
    void on_special(int key);            // flèches
};
