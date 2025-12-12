/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define assert(exp) if (!(exp)) { *(volatile int *)0 = 0; }
#define arrcnt(arr) (sizeof(arr)/sizeof(arr[0]))

// [.h]
//
#define STB_DS_IMPLEMENTATION
#define STBDS_ASSERT assert
#include "../vendor/stb/stb_ds.h"
#include "../vendor/glad/glad.h"
#include "../vendor/GLFW/glfw3.h"
#include "../../cdt.h"
#include "ex_math.h"
#include "ex_gl.h"

// [.c]
//
#include "ex_math.c"
#include "ex_gl.c"


#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
int FRAMEBUFFER_X;
int FRAMEBUFFER_Y;
GLFWwindow *window;
cdt_context *ctx;
FILE *insert_log_file;
f32 polygon_scale = 10.f;
f32 g_points[][2] = {
    {-1.f, 1.f},
    {-1.f,-1.f},
    { 1.f,-1.f},
    { 1.f, 1.f},
};



int main(void) {
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(RESOLUTION_X, RESOLUTION_Y, "WASD move, MOUSE insert, DEL remove", 0, 0);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(window, gl_framebuffer_resize_callback);

    GLuint vao, vio, vbo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glGenBuffers(1, &vio);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);

    glDisable(GL_DEPTH_TEST); 
    glDepthFunc(GL_LEQUAL);

    GLuint simple_shader       = glcreateshader(simple_vs, simple_fs);
    GLuint simple_shader_vp    = glad_glGetUniformLocation(simple_shader, "vp");
    GLuint simple_shader_color = glad_glGetUniformLocation(simple_shader, "color");



    ctx = malloc(sizeof(cdt_context));
    cdt_init(ctx, 0.f, 1024.f, -1024.f, -1024.f, 1024.f, -1024.f);


    cdt_vec2 *edge_end_points = 0;
    int *is_constrained = 0;


    Vec2 camera_position = {0};

#if 1
    // Log
    char log_file_name[64];
    time_t now = time(0);
    strftime(log_file_name, sizeof(log_file_name), "insert_%Y%m%d_%H%M%S.log", localtime(&now));
    insert_log_file = fopen(log_file_name, "wb");
    if (!insert_log_file) {
        fprintf(stderr, "[Error] Could not open insert log.\n");
        return -1;
    } else {
        // Disabling the file cache prevents data from being discarded, even if 
        // the process is terminated in a debugger. ..or does it?
        setvbuf(insert_log_file, 0, _IONBF, 0);
    }
#endif


    f64 old_frame_time = glfwGetTime();
    glfwSetWindowSize(window, RESOLUTION_X, RESOLUTION_Y);
    while (!glfwWindowShouldClose(window)) {
        f64 new_frame_time = glfwGetTime();
        f32 dt = (f32)(new_frame_time - old_frame_time);
        old_frame_time = new_frame_time;

        glfwPollEvents();    
        glfwGetFramebufferSize(window, &FRAMEBUFFER_X, &FRAMEBUFFER_Y);
        glViewport(0, 0, FRAMEBUFFER_X, FRAMEBUFFER_Y);
        Vec2 resolution = litv2(RESOLUTION_X, RESOLUTION_Y);

        f32 camera_speed = 1024.f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { camera_position.y += dt*camera_speed; }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { camera_position.x -= dt*camera_speed; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { camera_position.y -= dt*camera_speed; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { camera_position.x += dt*camera_speed; }



        // Clear
        arrsetlen(edge_end_points, 0);
        arrsetlen(is_constrained, 0);

        glClearColor(0.05f,0.05f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);


        // Draw navmesh
        //
        for (int i = 0; i < ctx->edges.num; i+= 1) {
            cdt_edge *e = ctx->edges.data[i];
            arrput(edge_end_points, e->e[0].org->pos);
            arrput(edge_end_points, e->e[2].org->pos);
        }

        for (int i = 0; i < ctx->edges.num; i+= 1) {
            cdt_edge *e = ctx->edges.data[i];
            arrput(is_constrained, cdt_is_constrained(&e->e[0]));
        }
        

        { // Push polygon around mouse position for visual cue.
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            // To NDC
            x =  2.f*(x / (f64)FRAMEBUFFER_X) - 1.f;
            y = -2.f*(y / (f64)FRAMEBUFFER_Y) + 1.f;

            // To camera space
            x *= (RESOLUTION_X*0.5f);
            y *= (RESOLUTION_Y*0.5f);

            // To world space
            x += camera_position.x;
            y += camera_position.y;

            f32 mx = (f32)x;
            f32 my = (f32)y;

            for (int i = 0; i < arrcnt(g_points); ++i) {
                f32 x1 = polygon_scale*g_points[i][0];
                f32 y1 = polygon_scale*g_points[i][1];
                f32 x2 = polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
                f32 y2 = polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];
                cdt_vec2 p1 = {mx+x1,my+y1};
                cdt_vec2 p2 = {mx+x2,my+y2};
                arrput(edge_end_points, p1);
                arrput(edge_end_points, p2);

                arrput(is_constrained, 0);
            }
        }



#if 1
        glUseProgram(simple_shader);
        glEnableVertexAttribArray(0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        {
            GLsizei sz = sizeof(cdt_vec2);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sz, 0);
            glBufferData(GL_ARRAY_BUFFER, arrlen(edge_end_points)*sz, edge_end_points, GL_DYNAMIC_DRAW);
            for (int i = 0; i < arrlen(is_constrained); i+=1) {
                if (is_constrained[i]) {
                    glUniform4f(simple_shader_color, 1.f, 1.f, 0.1f, 1.0f);
                } else {
                    glUniform4f(simple_shader_color, 1.f, 1.f, 1.f, 1.0f);
                }
                M4x4 view_proj = m4x4_view_proj(camera_position, resolution);
                glUniformMatrix4fv(simple_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);
                glDrawArrays(GL_LINES, i*2, 2);

            }
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisableVertexAttribArray(0);
        glUseProgram(0);
#endif


        glfwSwapBuffers(window);
    }


    fclose(insert_log_file);
    return 0;
}
