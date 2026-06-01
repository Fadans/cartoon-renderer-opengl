#include "texture.hh"
#include "../lib/image.hh"
#include "../lib/image_io.hh"
#include <iostream>

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

void Texture::bind(GLenum unit) const {
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}
