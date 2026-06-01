#pragma once

#include <GL/glew.h>
#include <string>

// Encapsule la compilation + linkage d'un programme shader OpenGL.
// Usage :
//   program* p = program::make_program(vert_src, frag_src);
//   if (!p || !p->is_ready()) { /* erreur */ }
//   p->use();
class program {
public:
    ~program();

    // Compile les deux shaders et les linke. Retourne nullptr si échec.
    static program* make_program(const std::string& vertex_src,
                                 const std::string& fragment_src);

    bool  is_ready() const { return ready_; }
    char* get_log()  const { return log_;   }
    void  use()            { glUseProgram(id_); }
    GLuint id()      const { return id_;    }

private:
    program() = default;

    static GLuint compile_shader(GLenum type, const std::string& src);
    static void   append_log(char*& log, const char* msg);

    GLuint id_    = 0;
    bool   ready_ = false;
    char*  log_   = nullptr;
};
