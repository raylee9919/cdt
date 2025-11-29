/* ========================================================================

   (C) Copyright 2025 by Seong Woo Lee, All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <vector>

#include "vendor/glad/glad.h"
#include "vendor/GLFW/glfw3.h"

#define assert(exp) if (!(exp)) { *(volatile int *)0 = 0; }
#define arrcnt(arr) (sizeof(arr)/sizeof(arr[0]))

#include "demo_gl.h"
#include "demo_log.h"
#include "../cdt.h"



// Globals
//
int next_gen_id = 1;
float scale = 0.0125f;
float polygon_scale = 1.f;
Context *ctx;
std::list<int> constrains;
FILE *insert_log_file;


#if 0
/ * Quad */
float points[][2] = {
    {-2.f, -2.f},
    { 2.f, -2.f},
    { 2.f,  2.f},
    {-2.f,  2.f},
};

#else
/* Pentagon */
float g_points[][2] = {
    { 1.000000f,  0.000000f},
    { 0.309016f,  0.951056f},
    {-0.809016f,  0.587785f},
    {-0.809016f, -0.587785f},
    { 0.309016f, -0.951056f},
};
#endif




static void demo_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
static void demo_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void demo_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void demo_drop_callback(GLFWwindow *window, int count, const char **paths);



int main(void) {
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1920, 1080, "CDT Demo", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, demo_scroll_callback);
    glfwSetMouseButtonCallback(window, demo_mouse_button_callback);
    glfwSetKeyCallback(window, demo_key_callback);
    glfwSetDropCallback(window, demo_drop_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(window, gl_framebuffer_resize_callback);

    GLuint vao, vio, vbo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glGenBuffers(1, &vio);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);

    GLuint simpleshader = glcreateshader(simple_vs, simple_fs);

    // Init
    //
    ctx = new Context;
    cdt_init(ctx, 0, 300.f, -300.f, -300.f, 300.f, -300.f);




    char log_file_name[64];
    time_t now = time(0);
    strftime(log_file_name, sizeof(log_file_name), "insert_%Y%m%d_%H%M%S.log", localtime(&now));
    insert_log_file = fopen(log_file_name, "wb");
    if (!insert_log_file) {
        fprintf(stderr, "[Error] Could not open insert log.\n");
        return -1;
    } else {
        // Disabling file cache won't discard data even if we kill the process  
        // in a debugger.
        setvbuf(insert_log_file, NULL, _IONBF, 0);
    }

    


    GLuint simpleshader_scale = glad_glGetUniformLocation(simpleshader, "scale");
    GLuint simpleshader_color = glad_glGetUniformLocation(simpleshader, "color");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();    

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        double fw = (double)w;
        double fh = (double)h;
        glViewport(0, 0, w, h);


        glClearColor(0.05f,0.05f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        std::vector<Vec2> edge_end_points;
        for (Edge *e : ctx->edges) {
            edge_end_points.push_back(e->e[0].org->pos);
            edge_end_points.push_back(e->e[2].org->pos);
        }

        std::vector<bool> constrained_list;
        for (Edge *e : ctx->edges) {
            constrained_list.push_back(constrained(&e->e[0]));
        }

        { // Push polygon around mouse position for visual cue.
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            double xn =   (xpos / fw)*2.f - 1.f;
            double yn = -((ypos / fh)*2.f - 1.f);

            float mx = (float)xn/scale;
            float my = (float)yn/scale;

            for (int i = 0; i < arrcnt(g_points); i += 1) {
                float x1 = polygon_scale*g_points[i][0];
                float y1 = polygon_scale*g_points[i][1];
                float x2 = polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
                float y2 = polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];
                edge_end_points.push_back(Vec2{mx+x1,my+y1});
                edge_end_points.push_back(Vec2{mx+x2,my+y2});
                constrained_list.push_back(false);
            }
        }


        // Draw edges in cdt'ed subdivision.
        //
#if 1
        glUseProgram(simpleshader);
        glEnableVertexAttribArray(0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        {
            GLsizei sz = sizeof(Vec2);
            glVertexAttribPointer(0, 2, GL_FLOAT, false, sz, 0);
            glBufferData(GL_ARRAY_BUFFER, edge_end_points.size()*sz, edge_end_points.data(), GL_DYNAMIC_DRAW);
            glUniform1f(simpleshader_scale, scale);
            for (int i = 0; i < edge_end_points.size(); i += 1) {
                if (constrained_list[i]) {
                    glUniform4f(simpleshader_color, 0.8f, 0.8f, 0.1f, 1.0f);
                } else {
                    glUniform4f(simpleshader_color, 0.6f, 0.6f, 0.6f, 1.0f);
                }
                glDrawArrays(GL_LINES, i*2, sz);
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

static void demo_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    int ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
    if (ctrl == GLFW_PRESS) {
        polygon_scale += (float)yoffset;
    } else {
        scale += (float)yoffset * 0.01f;
        scale = scale > 0.0125f ? scale : 0.0125f;
    }
}

static void demo_cdt_insert_with_log(float x1, float y1, float x2, float y2) {
    fprintf(insert_log_file, "0x%08X 0x%08X 0x%08X 0x%08X\n", *((uint32_t *)&x1), *((uint32_t *)&y1), *((uint32_t *)&x2), *((uint32_t *)&y2));
    cdt_insert(ctx, next_gen_id, x1, y1, x2, y2);
}

static void demo_cdt_insert(GLFWwindow *window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int wi, hi;
    glfwGetFramebufferSize(window, &wi, &hi);
    double w = (double)wi;
    double h = (double)hi;

    double xn =   (xpos / w)*2.f - 1.f;
    double yn = -((ypos / h)*2.f - 1.f);

    float x = (float)xn/scale;
    float y = (float)yn/scale;


    for (int i = 0; i < arrcnt(g_points); i += 1) {
        float x1 = x + polygon_scale*g_points[i][0];
        float y1 = y + polygon_scale*g_points[i][1];
        float x2 = x + polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
        float y2 = y + polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];

        demo_cdt_insert_with_log(x1, y1, x2, y2);
    }

    fprintf(insert_log_file, ";\n");

    constrains.push_back(next_gen_id);
    next_gen_id += 1;
}

// Callbacks
//
static void demo_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        demo_cdt_insert(window);
    }
}

static void demo_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_DELETE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        int id = constrains.front();
        cdt_remove(ctx, id);
        constrains.pop_front();
    }
}

static void demo_drop_callback(GLFWwindow *window, int count, const char **paths) {
    if (count == 1) {
        // Redo log
        //
        const char *log_file_name = paths[0];
        Parse_Result parse = float_array_from_log_file(log_file_name);  
        int offset = 0;
        for (int n : parse.counts) {
            for (int i = 0; i < n; i +=4) {
                float x1 = parse.floats[offset + i];
                float y1 = parse.floats[offset + i+1];
                float x2 = parse.floats[offset + i+2];
                float y2 = parse.floats[offset + i+3];
                demo_cdt_insert_with_log(x1, y1, x2, y2);
            }
            fprintf(insert_log_file, ";\n");
            offset += n;
        }
    }
}
