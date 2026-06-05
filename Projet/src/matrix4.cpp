#include "matrix4.hh"
#include <cstring>
#include <iomanip>

namespace mygl {

matrix4::matrix4() { std::memset(m, 0, sizeof(m)); }

matrix4 matrix4::identity() {
    matrix4 r;
    r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.f;
    return r;
}

matrix4 matrix4::operator*(const matrix4& rhs) const {
    matrix4 res;
    for (int col=0;col<4;col++)
        for (int row=0;row<4;row++)
            for (int k=0;k<4;k++)
                res.m[col*4+row]+=m[k*4+row]*rhs.m[col*4+k];
    return res;
}

matrix4& matrix4::operator*=(const matrix4& rhs){*this=*this*rhs;return *this;}

std::ostream& operator<<(std::ostream& out, const matrix4& mat) {
    for (int row=0;row<4;row++){
        for (int col=0;col<4;col++)
            out<<std::setw(10)<<std::fixed<<std::setprecision(4)<<mat.m[col*4+row];
        out<<"\n";
    }
    return out;
}

matrix4 matrix4::translate(GLfloat tx, GLfloat ty, GLfloat tz) {
    matrix4 r = identity();
    r.m[12]=tx; r.m[13]=ty; r.m[14]=tz;
    return r;
}

matrix4 matrix4::rotate_x(GLfloat a) {
    matrix4 r = identity();
    r.m[5]=std::cos(a);  r.m[9]=-std::sin(a);
    r.m[6]=std::sin(a);  r.m[10]=std::cos(a);
    return r;
}

matrix4 matrix4::rotate_y(GLfloat a) {
    matrix4 r = identity();
    r.m[0]=std::cos(a);  r.m[8]=std::sin(a);
    r.m[2]=-std::sin(a); r.m[10]=std::cos(a);
    return r;
}

matrix4 matrix4::rotate_z(GLfloat a) {
    matrix4 r = identity();
    r.m[0]=std::cos(a);  r.m[4]=-std::sin(a);
    r.m[1]=std::sin(a);  r.m[5]=std::cos(a);
    return r;
}

matrix4 matrix4::scale(GLfloat sx, GLfloat sy, GLfloat sz) {
    matrix4 r = identity();
    r.m[0]=sx; r.m[5]=sy; r.m[10]=sz;
    return r;
}

matrix4 matrix4::shadow(GLfloat a, GLfloat b, GLfloat c, GLfloat d,
                        const vec3& light, GLfloat lw) {
    GLfloat lx=light.x, ly=light.y, lz=light.z;
    GLfloat dot = a*lx + b*ly + c*lz + d*lw;
    matrix4 r;
    r.m[0]=dot - lx*a;  r.m[4]=-lx*b;       r.m[8] =-lx*c;       r.m[12]=-lx*d;
    r.m[1]=-ly*a;       r.m[5]=dot - ly*b;  r.m[9] =-ly*c;       r.m[13]=-ly*d;
    r.m[2]=-lz*a;       r.m[6]=-lz*b;       r.m[10]=dot - lz*c;  r.m[14]=-lz*d;
    r.m[3]=-lw*a;       r.m[7]=-lw*b;       r.m[11]=-lw*c;       r.m[15]=dot - lw*d;
    return r;
}

matrix4 lookat(GLfloat eX,GLfloat eY,GLfloat eZ,
               GLfloat cX,GLfloat cY,GLfloat cZ,
               GLfloat uX,GLfloat uY,GLfloat uZ) {
    vec3 eye{eX,eY,eZ}, center{cX,cY,cZ}, up{uX,uY,uZ};
    vec3 F=(center-eye).normalize();
    vec3 s=F.cross(up).normalize();
    vec3 u=s.cross(F);
    matrix4 r=matrix4::identity();
    r.m[0]=s.x; r.m[4]=s.y; r.m[8] =s.z; r.m[12]=-s.dot(eye);
    r.m[1]=u.x; r.m[5]=u.y; r.m[9] =u.z; r.m[13]=-u.dot(eye);
    r.m[2]=-F.x;r.m[6]=-F.y;r.m[10]=-F.z;r.m[14]= F.dot(eye);
    return r;
}

matrix4 frustum(GLfloat l,GLfloat r,GLfloat b,GLfloat t,GLfloat n,GLfloat f) {
    matrix4 m;
    m.m[0]=2*n/(r-l); m.m[5]=2*n/(t-b);
    m.m[8]=(r+l)/(r-l); m.m[9]=(t+b)/(t-b);
    m.m[10]=-(f+n)/(f-n); m.m[11]=-1.f;
    m.m[14]=-2.f*f*n/(f-n);
    return m;
}

}
