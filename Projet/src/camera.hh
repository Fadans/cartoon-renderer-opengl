#pragma once
#include "matrix4.hh"

class Camera {
public:
    float angle_h   =  0.3f;
    float angle_v   =  0.4f;
    float distance  =  4.0f;
    float speed     =  0.06f;
    float pan_speed =  0.10f;

    mygl::vec3 center = {0, 0, 0};

    mygl::matrix4 get_view() const;
    mygl::vec3    get_eye()  const;

    void pan(float dx, float dy);

    void move(float strafe, float forward);

    void on_key    (unsigned char key);
    void on_special(int key);
};
