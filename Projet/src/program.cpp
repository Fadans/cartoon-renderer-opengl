#include "program.hh"
#include <cstdlib>
#include <cstring>
#include <iostream>

program::~program() {
    if (id_)  glDeleteProgram(id_);
    if (log_) std::free(log_);
}

GLuint program::compile_shader(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        char* buf = (char*)std::malloc(len + 1);
        glGetShaderInfoLog(shader, len, nullptr, buf);
        std::cerr << "Shader compile error:\n" << buf << "\n";
        std::free(buf);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

program* program::make_program(const std::string& vertex_src,
                               const std::string& fragment_src) {
    program* p = new program();

    GLuint vert = compile_shader(GL_VERTEX_SHADER,   vertex_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, fragment_src);

    if (!vert || !frag) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        delete p;
        return nullptr;
    }

    p->id_ = glCreateProgram();
    glAttachShader(p->id_, vert);
    glAttachShader(p->id_, frag);
    glLinkProgram(p->id_);

    glDetachShader(p->id_, vert); glDeleteShader(vert);
    glDetachShader(p->id_, frag); glDeleteShader(frag);

    GLint status;
    glGetProgramiv(p->id_, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint len;
        glGetProgramiv(p->id_, GL_INFO_LOG_LENGTH, &len);
        p->log_ = (char*)std::malloc(len + 1);
        glGetProgramInfoLog(p->id_, len, nullptr, p->log_);
        std::cerr << "Program link error:\n" << p->log_ << "\n";
        return p;
    }

    p->ready_ = true;
    return p;
}
