#include "texture.hh"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

Texture::~Texture()
{
    if (id_)
        glDeleteTextures(1, &id_);
}

Texture *Texture::load(const char *path)
{
    int w, h, channels;

    unsigned char *data = stbi_load(path, &w, &h, &channels, 4);
    if (!data)
    {
        std::cerr << "Texture::load — cannot open: " << path
                  << " (" << stbi_failure_reason() << ")\n";
        return nullptr;
    }

    Texture *t = new Texture();
    glGenTextures(1, &t->id_);
    glBindTexture(GL_TEXTURE_2D, t->id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    std::cout << "Texture loaded: " << path
              << " (" << w << "x" << h << ", "
              << channels << " canaux)\n";
    stbi_image_free(data);
    return t;
}

void Texture::bind(GLenum unit) const
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}
