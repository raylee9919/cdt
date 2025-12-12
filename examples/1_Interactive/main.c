/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define assert(exp) if (!(exp)) { *(volatile int *)0 = 0; }
#define arrcnt(arr) (sizeof(arr)/sizeof(arr[0]))
#define COMPILER_UNREFERENCED(e) (void)(e)

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
#include "ex_log.h"

// [.c]
//
#include "ex_math.c"
#include "ex_gl.c"
#include "ex_log.c"


#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
int FRAMEBUFFER_X;
int FRAMEBUFFER_Y;
GLFWwindow *g_window;
cdt_context *ctx;
FILE *insert_log_file;
f32 polygon_scale = 10.f;
cdt_id next_gen_id = 1;

f32 scale = 1.f;
#define MIN_SCALE  1.f
#define MAX_SCALE 10.f

f32 g_points[][2] = {
    { 0.59607f,  0.80293f},
    { 0.00393f,  0.38195f},
    {-0.57943f,  0.81502f},
    {-0.36204f,  0.12176f},
    {-0.95418f, -0.29922f},
    {-0.22768f, -0.30669f},
    {-0.01028f, -0.99995f},
    { 0.22132f, -0.31131f},
    { 0.94783f, -0.31878f},
    { 0.36447f,  0.11429f},
};
cdt_id *constraints;
Vec2 camera_position;
int mouse_middle_down;
int mouse_middle_toggled;
f64 mouse_middle_last_down_x;
f64 mouse_middle_last_down_y;


void demo_cdt_insert_with_log(f32 x1, f32 y1, f32 x2, f32 y2);
void demo_cdt_insert(GLFWwindow *window, f32 x, f32 y);
void callback_mouse_button(GLFWwindow *window, int button, int action, int mods);
void callback_scroll(GLFWwindow *window, double xoffset, double yoffset);
void callback_key(GLFWwindow* window, int key, int scancode, int action, int mods);
void callback_drop_down(GLFWwindow *window, int count, const char **paths);
Vec2 screen_to_world_space(f64 x, f64 y);

int main(void) {
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_window = glfwCreateWindow(RESOLUTION_X, RESOLUTION_Y, "WASD move | MOUSE insert | DEL remove the oldest", 0, 0);
    glfwMakeContextCurrent(g_window);
    glfwSetKeyCallback(g_window, callback_key);
    glfwSetMouseButtonCallback(g_window, callback_mouse_button);
    glfwSetScrollCallback(g_window, callback_scroll);
    glfwSetDropCallback(g_window, callback_drop_down);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(g_window, gl_framebuffer_resize_callback);

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
    cdt_init(ctx, 0.f, 2048.f, -2048.f, -2048.f, 2048.f, -2048.f);


    cdt_vec2 *edge_end_points = 0;
    int *is_constrained = 0;


    

#if 1
    // Log
    char log_file_name[64];
    time_t now = time(0);
    strftime(log_file_name, sizeof(log_file_name), "%Y_%m_%d_%H%M%S.log", localtime(&now));
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
    glfwSetWindowSize(g_window, RESOLUTION_X, RESOLUTION_Y);
    while (!glfwWindowShouldClose(g_window)) {
        f64 new_frame_time = glfwGetTime();
        f32 dt = (f32)(new_frame_time - old_frame_time);
        old_frame_time = new_frame_time;

        mouse_middle_toggled = 0;

        glfwPollEvents();
        glfwGetFramebufferSize(g_window, &FRAMEBUFFER_X, &FRAMEBUFFER_Y);
        glViewport(0, 0, FRAMEBUFFER_X, FRAMEBUFFER_Y);
        Vec2 resolution = litv2(RESOLUTION_X, RESOLUTION_Y);

        f32 camera_speed = 1024.f;
        if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) { camera_position.y += dt*camera_speed; }
        if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) { camera_position.x -= dt*camera_speed; }
        if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) { camera_position.y -= dt*camera_speed; }
        if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) { camera_position.x += dt*camera_speed; }

        if (mouse_middle_down) {
            f64 mx, my;
            glfwGetCursorPos(g_window, &mx, &my);

            if (!mouse_middle_toggled) {
                f64 dx = mx - mouse_middle_last_down_x;
                f64 dy = my - mouse_middle_last_down_y;

                // [0,F] -> [0,2]
                dx *=  (-2.f/FRAMEBUFFER_X);
                dy *=  ( 2.f/FRAMEBUFFER_Y);

                // [0,2] -> [0,R]
                dx *= (RESOLUTION_X*0.5f);
                dy *= (RESOLUTION_Y*0.5f);

                camera_position.x += (f32)dx;
                camera_position.y += (f32)dy;
            }

            mouse_middle_last_down_x = mx;
            mouse_middle_last_down_y = my;
        }




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
            arrput(is_constrained, cdt_is_constrained(e));
        }
        

        { // Push polygon around mouse position for visual cue.
            f64 x, y;
            glfwGetCursorPos(g_window, &x, &y);
            Vec2 wp = screen_to_world_space(x, y);
            f32 mx = (f32)wp.x;
            f32 my = (f32)wp.y;

            for (int i = 0; i < arrcnt(g_points); ++i) {
                f32 x1 = scale*polygon_scale*g_points[i][0];
                f32 y1 = scale*polygon_scale*g_points[i][1];
                f32 x2 = scale*polygon_scale*g_points[(i+1)%arrcnt(g_points)][0];
                f32 y2 = scale*polygon_scale*g_points[(i+1)%arrcnt(g_points)][1];
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
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(cdt_vec2), 0);
            glBufferData(GL_ARRAY_BUFFER, arrlen(edge_end_points)*sizeof(cdt_vec2), edge_end_points, GL_DYNAMIC_DRAW);
            for (int i = 0; i < arrlen(is_constrained); i+=1) {
                if (is_constrained[i]) {
                    glUniform4f(simple_shader_color, 1.f, 1.f, 0.1f, 1.0f);
                } else {
                    glUniform4f(simple_shader_color, 0.5f, 0.5f, 0.5f, 1.0f);
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


        glfwSwapBuffers(g_window);
    }


    fclose(insert_log_file);
    return 0;
}

void demo_cdt_insert_with_log(f32 x1, f32 y1, f32 x2, f32 y2) {
    fprintf(insert_log_file, "0x%08X 0x%08X 0x%08X 0x%08X\n", *((u32 *)&x1), *((u32 *)&y1), *((u32 *)&x2), *((u32 *)&y2));
    cdt_insert(ctx, next_gen_id, x1, y1, x2, y2);
}

void demo_cdt_insert(GLFWwindow *window, f32 x, f32 y) {
    COMPILER_UNREFERENCED(window);

    f32 final_scale = scale * polygon_scale;

    // Log first.
    for (int i = 0; i < arrcnt(g_points); i += 1) {
        f32 x1 = x + final_scale*g_points[i][0];
        f32 y1 = y + final_scale*g_points[i][1];
        f32 x2 = x + final_scale*g_points[(i+1)%arrcnt(g_points)][0];
        f32 y2 = y + final_scale*g_points[(i+1)%arrcnt(g_points)][1];
        fprintf(insert_log_file, "0x%08X 0x%08X 0x%08X 0x%08X\n", *((u32 *)&x1), *((u32 *)&y1), *((u32 *)&x2), *((u32 *)&y2));
    }
    fprintf(insert_log_file, ";\n");

    // Insert afterwards.
    for (int i = 0; i < arrcnt(g_points); i += 1) {
        f32 x1 = x + final_scale*g_points[i][0];
        f32 y1 = y + final_scale*g_points[i][1];
        f32 x2 = x + final_scale*g_points[(i+1)%arrcnt(g_points)][0];
        f32 y2 = y + final_scale*g_points[(i+1)%arrcnt(g_points)][1];
        cdt_insert(ctx, next_gen_id, x1, y1, x2, y2);
    }

    arrput(constraints, next_gen_id);
    next_gen_id += 1;
}

// Callbacks
//
void callback_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    COMPILER_UNREFERENCED(mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        f64 xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        Vec2 world_pos = screen_to_world_space(xpos, ypos);
        f32 x = world_pos.x;
        f32 y = world_pos.y;

        demo_cdt_insert(window, x, y);
    }

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            mouse_middle_down = 1;
            mouse_middle_toggled = 1;
        } else if (action == GLFW_RELEASE) {
            mouse_middle_down = 0;
            mouse_middle_toggled = 1;
        }
    }
}

void callback_scroll(GLFWwindow *window, double xoffset, double yoffset) {
    COMPILER_UNREFERENCED(window);
    COMPILER_UNREFERENCED(xoffset);

    if (yoffset > 0.f) {
        scale += 1.f;
        scale = min(MAX_SCALE, scale);
    } else if (yoffset < 0.f) {
        scale -= 1.f;
        scale = max(MIN_SCALE, scale);
    }
}

void callback_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    COMPILER_UNREFERENCED(window);
    COMPILER_UNREFERENCED(scancode);
    COMPILER_UNREFERENCED(mods);

    if (key == GLFW_KEY_DELETE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        int id = constraints[0];
        cdt_remove(ctx, id);
        arrdeln(constraints, 0, 1);
    }
}

void callback_drop_down(GLFWwindow *window, int count, const char **paths) {
    glfwFocusWindow(window);

    if (count == 1) {
        // Redo log
        //
        const char *log_file_name = paths[0];
        Parse_Result parse = float_array_from_log_file(log_file_name);  
        int offset = 0;
        for (int j = 0; j < arrlen(parse.counts); j+=1) {
            int n = parse.counts[j];
            for (int i = 0; i < n; i +=4) {
                f32 x1 = parse.floats[offset + i];
                f32 y1 = parse.floats[offset + i+1];
                f32 x2 = parse.floats[offset + i+2];
                f32 y2 = parse.floats[offset + i+3];
                demo_cdt_insert_with_log(x1, y1, x2, y2);
            }
            fprintf(insert_log_file, ";\n");
            offset += n;

            arrput(constraints, next_gen_id);
            next_gen_id += 1;
        }
    }
}

Vec2 screen_to_world_space(f64 x, f64 y) {
    // To NDC
    x =  2.f*(x / (f64)FRAMEBUFFER_X) - 1.f;
    y = -2.f*(y / (f64)FRAMEBUFFER_Y) + 1.f;

    // To camera space
    x *= (RESOLUTION_X*0.5f);
    y *= (RESOLUTION_Y*0.5f);

    // To world space
    x += camera_position.x;
    y += camera_position.y;

    return litv2((f32)x, (f32)y);
}
