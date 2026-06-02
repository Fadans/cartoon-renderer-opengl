#include "camera.hh"
#include <GL/freeglut.h>
#include <cmath>
#include <algorithm>

mygl::vec3 Camera::get_eye() const {
    // Orbite autour de center (et non de l'origine) → le pan déplace œil + cible ensemble
    return {
        center.x + distance * std::cos(angle_v) * std::sin(angle_h),
        center.y + distance * std::sin(angle_v),
        center.z + distance * std::cos(angle_v) * std::cos(angle_h)
    };
}

void Camera::pan(float dx, float dy) {
    mygl::vec3 e = get_eye();
    mygl::vec3 fwd   = (center - e).normalize();          // œil → cible
    mygl::vec3 right = fwd.cross({0, 1, 0}).normalize();  // droite écran
    mygl::vec3 up    = right.cross(fwd);                  // haut écran
    center.x += (right.x * dx + up.x * dy) * pan_speed;
    center.y += (right.y * dx + up.y * dy) * pan_speed;
    center.z += (right.z * dx + up.z * dy) * pan_speed;
    glutPostRedisplay();
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
        // Pan (déplacement de la cible dans le plan écran)
        case 'j': case 'J': pan(-1.0f,  0.0f); break;  // gauche
        case 'l': case 'L': pan( 1.0f,  0.0f); break;  // droite
        case 'i': case 'I': pan( 0.0f,  1.0f); break;  // haut
        case 'k': case 'K': pan( 0.0f, -1.0f); break;  // bas
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
