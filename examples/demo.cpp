/* ========================================================================

   (C) Copyright 2025 by Seong Woo Lee, All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>

#include "vendor/glad/glad.h"
#include "vendor/GLFW/glfw3.h"

#define app_assert(exp) if (!(exp)) { *(volatile int *)0 = 0; }
#define arrcnt(arr) (sizeof(arr)/sizeof(arr[0]))

#include "demo_gl.h"
#include "../cdt.h"



// Globals
//
int next_gen_id = 1;
float scale = 0.0125f;
float polygon_scale = 1.f;
Context *ctx;
std::list<int> constrains;


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
float points[][2] = {
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

struct V2 {
    float x, y;
};

static V2 operator - (V2 a, V2 b) {
    return V2{a.x-b.x, a.y-b.y};
}

static float cross(V2 a, V2 b) {
    return a.x*b.y - a.y*b.x;
}

static bool in_triangle(V2 p, V2 a, V2 b, V2 c) {
    V2 ap = p-a;
    V2 bp = p-b;
    V2 cp = p-c;
    float ca = cross(b-a, ap);
    float cb = cross(c-b, bp);
    float cc = cross(a-c, cp);
    if (ca>0.f && cb>0.f && cc>0.f) { return true; }
    return false;
}

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

    
    ctx = new Context;
    cdt_init(ctx, 0, 200.f, -200.f, -200.f, 200.f, -200.f);


    GLuint simpleshader_scale = glad_glGetUniformLocation(simpleshader, "scale");
    GLuint simpleshader_color = glad_glGetUniformLocation(simpleshader, "color");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Ear clipping triangulation test.
    //
    V2 test_points[] = {
        {   3.f,  48.f},
        {  52.f,   8.f},
        {  99.f,  50.f},
        { 138.f,  25.f},
        { 175.f,  77.f},
        { 131.f,  72.f},
        { 111.f, 113.f},
        {  72.f,  43.f},
        {  26.f,  55.f},
        {  29.f, 100.f},
    };
    int n = arrcnt(test_points);
    std::list<int> verts;
    std::list<int> convex;
    std::list<int> reflex;
    std::list<int> ears;

    std::vector<int> indices;

    for (int i = 0; i < n; ++i) { verts.push_back(i); }

    for (int i0 = 0; i0 < n; ++i0) {
        int i1 = (i0+1)%n;
        int i2 = (i1+1)%n;

        V2 p0 = test_points[i0];
        V2 p1 = test_points[i1];
        V2 p2 = test_points[i2];

        V2 e01 = p1 - p0;
        V2 e12 = p2 - p1;

        float c = cross(e01, e12);
        if (c > 0.f) {
            convex.push_back(i1);

            bool is_ear = 1;
            for (int i=1; i <= n-3; i+=1) {
                int idx = (i2 + i) % n;
                V2 p = test_points[idx];

                if (in_triangle(p, p0, p1, p2)) {
                    is_ear = 0;
                    break;
                }
            }
            if (is_ear) { ears.push_back(i1); }
        } else if (c < 0.f) {
            reflex.push_back(i1);
        } else {
            app_assert("!Collinear degenerate detected!");
        }
    }


    while (verts.size() != 3) {
        // Pop an ear tip.
        int ear = ears.front();
        ears.pop_front();

        // Remove ear tip from the vertex list.
        auto it = std::find(verts.begin(), verts.end(), ear);
        int l   = *std::prev(it);
        int r   = *std::next(it);
        int ll  = *std::prev(std::prev(it));
        int rr  = *std::next(std::next(it));
        verts.remove(ear);

        V2 pl  = test_points[l];
        V2 pr  = test_points[r];
        V2 pll = test_points[ll];
        V2 prr = test_points[rr];

        // Push 3 indices for drawing. Equivalent to creating a triangle.
        indices.push_back(ear);
        indices.push_back(r);
        indices.push_back(l);
    }

    // Push remaining 3 indices.
    while (!verts.empty()) {
        indices.push_back(verts.front());
        verts.pop_front();
    }


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

            for (int i = 0; i < arrcnt(points); i += 1) {
                float x1 = polygon_scale*points[i][0];
                float y1 = polygon_scale*points[i][1];
                float x2 = polygon_scale*points[(i+1)%arrcnt(points)][0];
                float y2 = polygon_scale*points[(i+1)%arrcnt(points)][1];
                edge_end_points.push_back(Vec2{mx+x1,my+y1});
                edge_end_points.push_back(Vec2{mx+x2,my+y2});
                constrained_list.push_back(false);
            }
        }


        // Draw edges in cdt'ed subdivision.
        //
#if 0
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

static void demo_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
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

        for (int i = 0; i < arrcnt(points); i += 1) {
            float x1 = x + polygon_scale*points[i][0];
            float y1 = y + polygon_scale*points[i][1];
            float x2 = x + polygon_scale*points[(i+1)%arrcnt(points)][0];
            float y2 = y + polygon_scale*points[(i+1)%arrcnt(points)][1];
            cdt_insert(ctx, next_gen_id, x1, y1, x2, y2);
        }

        constrains.push_back(next_gen_id);
        printf("[App] Add constrain ID %u\n", next_gen_id);
        next_gen_id += 1;
    }
}

static void demo_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_DELETE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        int id = constrains.front();
        cdt_remove(ctx, id);
        constrains.pop_front();
        printf("[App] Removed constrain ID: %u\n", id);
    }
}
