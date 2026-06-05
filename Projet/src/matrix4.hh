#pragma once
#include <GL/glew.h>
#include <iostream>
#include <cmath>

namespace mygl {

struct vec3 {
    GLfloat x, y, z;
    vec3(GLfloat x=0,GLfloat y=0,GLfloat z=0):x(x),y(y),z(z){}
    vec3 operator-(const vec3& o)const{return{x-o.x,y-o.y,z-o.z};}
    GLfloat dot(const vec3& o)const{return x*o.x+y*o.y+z*o.z;}
    vec3 cross(const vec3& o)const{return{y*o.z-z*o.y,z*o.x-x*o.z,x*o.y-y*o.x};}
    vec3 normalize()const{GLfloat n=std::sqrt(x*x+y*y+z*z);return{x/n,y/n,z/n};}
};

class matrix4 {
public:
    GLfloat m[16];

    matrix4();
    matrix4  operator* (const matrix4& rhs) const;
    matrix4& operator*=(const matrix4& rhs);
    const GLfloat* data() const { return m; }

    static matrix4 identity();
    static matrix4 translate(GLfloat tx, GLfloat ty, GLfloat tz);
    static matrix4 rotate_x(GLfloat angle);
    static matrix4 rotate_y(GLfloat angle);
    static matrix4 rotate_z(GLfloat angle);
    static matrix4 scale(GLfloat sx, GLfloat sy, GLfloat sz);

    static matrix4 shadow(GLfloat a, GLfloat b, GLfloat c, GLfloat d,
                          const vec3& light, GLfloat lw = 1.0f);

    friend std::ostream& operator<<(std::ostream& out, const matrix4& m);
};

matrix4 lookat(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ,
               GLfloat  cxX, GLfloat  cxY, GLfloat  cxZ,
               GLfloat  upX, GLfloat  upY, GLfloat  upZ);

matrix4 frustum(GLfloat left, GLfloat right,
                GLfloat bottom, GLfloat top,
                GLfloat znear, GLfloat zfar);

}
