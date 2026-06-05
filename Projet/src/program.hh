#pragma once

#include <GL/glew.h>
#include <string>

class program {
public:
    ~program();

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
