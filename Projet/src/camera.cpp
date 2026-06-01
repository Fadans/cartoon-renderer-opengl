#include "camera.hh"
#include <GL/freeglut.h>
#include <cmath>
#include <algorithm>

mygl::vec3 Camera::get_eye() const {
    return {
        distance * std::cos(angle_v) * std::sin(angle_h),
        distance * std::sin(angle_v),
        distance * std::cos(angle_v) * std::cos(angle_h)
    };
}

mygl::matrix4 Camera::get_view() const {
    mygl::vec3 e = get_eye();
    return mygl::lookat(e.x, e.y, e.z,
                        center.x, center.y, center.z,
                        0, 1, 0);
}

void Camera::on_key(unsigned char key) {
    switch (key) {
        case 'a': case 'A': angle_h -= speed; break;
        case 'd': case 'D': angle_h += speed; break;
        case 'w': case 'W': angle_v = std::min(angle_v + speed,  1.5f); break;
        case 's': case 'S': angle_v = std::max(angle_v - speed, -1.5f); break;
        case 'q': case 'Q': distance = std::max(distance - 0.2f, 1.0f); break;
        case 'e': case 'E': distance += 0.2f; break;
        case 27:            std::exit(0); // ESC
    }
    glutPostRedisplay();
}

void Camera::on_special(int key) {
    switch (key) {
        case GLUT_KEY_LEFT:  angle_h -= speed; break;
        case GLUT_KEY_RIGHT: angle_h += speed; break;
        case GLUT_KEY_UP:    angle_v = std::min(angle_v + speed,  1.5f); break;
        case GLUT_KEY_DOWN:  angle_v = std::max(angle_v - speed, -1.5f); break;
    }
    glutPostRedisplay();
}
