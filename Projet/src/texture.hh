#pragma once
#include <GL/glew.h>

class Texture {
public:
    ~Texture();
    static Texture* load(const char* path);

    // Texture procédurale — aucun fichier nécessaire
    // cases_per_side = nombre de cases sur chaque côté
    static Texture* checkerboard(int size, int cases_per_side,
                                 GLubyte r1, GLubyte g1, GLubyte b1,
                                 GLubyte r2, GLubyte g2, GLubyte b2);

    void bind(GLenum unit) const;
    bool valid() const { return id_ != 0; }
private:
    Texture() = default;
    GLuint id_ = 0;
};
