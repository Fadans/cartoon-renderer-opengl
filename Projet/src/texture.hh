#pragma once
#include <GL/glew.h>

class Texture {
public:
    ~Texture();
    static Texture* load(const char* path);
    void bind(GLenum unit) const;
    bool valid() const { return id_ != 0; }
private:
    Texture() = default;
    GLuint id_ = 0;
};
