#include "texture.hh"
#include "../lib/image.hh"
#include "../lib/image_io.hh"
#include <iostream>
#include <vector>

Texture::~Texture() {
    if (id_) glDeleteTextures(1, &id_);
}

Texture* Texture::load(const char* path) {
    tifo::rgb24_image* img = tifo::load_image(path);
    if (!img) {
        std::cerr << "Texture::load — cannot open: " << path << "\n";
        return nullptr;
    }
    Texture* t = new Texture();
    glGenTextures(1, &t->id_);
    glBindTexture(GL_TEXTURE_2D, t->id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 img->sx, img->sy, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, img->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    std::cout << "Texture loaded: " << path
              << " (" << img->sx << "x" << img->sy << ")\n";
    delete img;
    return t;
}

Texture* Texture::checkerboard(int size, int cases,
                               GLubyte r1, GLubyte g1, GLubyte b1,
                               GLubyte r2, GLubyte g2, GLubyte b2) {
    std::vector<GLubyte> pixels(size * size * 3);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int cx = x * cases / size;
            int cy = y * cases / size;
            bool white = (cx + cy) % 2 == 0;
            int idx = (y * size + x) * 3;
            pixels[idx+0] = white ? r1 : r2;
            pixels[idx+1] = white ? g1 : g2;
            pixels[idx+2] = white ? b1 : b2;
        }
    }
    Texture* t = new Texture();
    glGenTextures(1, &t->id_);
    glBindTexture(GL_TEXTURE_2D, t->id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return t;
}

void Texture::bind(GLenum unit) const {
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}
