/* ========================================================================

   (C) Copyright 2025 by Seong Woo Lee, All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   ======================================================================== */



const char *simple_vs = 
#include "shader/simple_vs.glsl"
const char *simple_fs = 
#include "shader/simple_fs.glsl"

void gl_framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}  

void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

GLuint glcreateshader(const char *vsrc, const char *fsrc) {
    GLuint program = 0;

    if (glCreateShader) {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { vsrc };
        glShaderSource(vshader, arrcnt(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { fsrc };
        glShaderSource(fshader, arrcnt(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            assert(!"s");
        }

        glDeleteShader(vshader);
        glDeleteShader(fshader);
    } else {
        // @TODO: Error-Handling.
    }
    
    return program;
}
