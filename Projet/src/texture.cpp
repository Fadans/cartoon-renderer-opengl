#include "texture.hh"
#include <iostream>
#include <vector>

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
    // Force le chargement en RGBA (4 canaux) pour tout supporter
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

Texture *Texture::checkerboard(int size, int cases,
                               GLubyte r1, GLubyte g1, GLubyte b1,
                               GLubyte r2, GLubyte g2, GLubyte b2)
{
    std::vector<GLubyte> pixels(size * size * 4);
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int cx = x * cases / size;
            int cy = y * cases / size;
            bool white = (cx + cy) % 2 == 0;
            int idx = (y * size + x) * 4;
            pixels[idx + 0] = white ? r1 : r2;
            pixels[idx + 1] = white ? g1 : g2;
            pixels[idx + 2] = white ? b1 : b2;
            pixels[idx + 3] = 255;
        }
    }
    Texture *t = new Texture();
    glGenTextures(1, &t->id_);
    glBindTexture(GL_TEXTURE_2D, t->id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return t;
}

void Texture::bind(GLenum unit) const
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}