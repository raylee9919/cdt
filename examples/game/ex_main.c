/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define assert(exp) if (!(exp)) { *(volatile int *)0 = 0; }
#define arrcnt(arr) (sizeof(arr)/sizeof(arr[0]))
#define COMPILER_UNREFERENCED(var) ((void)var)

// [.h]
//
#define STB_DS_IMPLEMENTATION
#define STBDS_ASSERT assert
#include "../vendor/stb/stb_ds.h"
#include "../vendor/glad/glad.h"
#include "../vendor/GLFW/glfw3.h"
#include "../../cdt.h"
#include "ex_math.h"
#include "ex_engine.h"
#include "ex_gl.h"
#include "ex_log.h"
#include "sprite/ex_sprite_player.h"
#include "sprite/ex_sprite_skeleton.h"
#include "sprite/ex_sprite_ground.h"
#include "sprite/ex_sprite_building.h"

// [.c]
//
#include "ex_math.c"
#include "ex_engine.c"
#include "ex_gl.c"


int main(void) {
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1920, 1080, "CDT Game", 0, 0);
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


    // Initialize
    //
    engine = (Engine *)malloc(sizeof(Engine));
    engine_init(window);

    engine->sprite_shader       = glcreateshader(sprite_vs, sprite_fs);
    engine->sprite_shader_model = glad_glGetUniformLocation(engine->sprite_shader, "model");
    engine->sprite_shader_vp    = glad_glGetUniformLocation(engine->sprite_shader, "vp");

    engine->simple_shader       = glcreateshader(simple_vs, simple_fs);
    engine->simple_shader_vp    = glad_glGetUniformLocation(engine->simple_shader, "vp");
    engine->simple_shader_color = glad_glGetUniformLocation(engine->simple_shader, "color");



    cdt_init(&engine->navmesh, 0, 4096.f, -4096.f, -4096.f, 4096.f, -4096.f);

#if 0
    Entity *ground = engine_alloc_entity(ENTITY_FLAG_DRAW); {
        ground->size    = litv2(1024.f, 1024.f);
        ground->texture = TEXTURE_TYPE_GROUND;
        ground->u1 = 0.f; ground->v1 = 0.f;
        ground->u2 = 16.f; ground->v2 = 16.f;
    }
#endif

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            Entity *building = engine_alloc_entity(ENTITY_FLAG_DRAW);
            building->position = litv2(-400.f+(270.f+r*10.f)*c, -400.f+(270.f-c*12.f)*r);
            building->size     = litv2(SPRITE_BUILDING_WIDTH, SPRITE_BUILDING_HEIGHT);
            building->texture  = TEXTURE_TYPE_BUILDING;
            building->offset   = litv2(0.f, -72.f);
            arrput(building->navmesh, litv2(  0.f,-120.f));
            arrput(building->navmesh, litv2(-85.f,-100.f));
            arrput(building->navmesh, litv2(-85.f,  10.f));
            arrput(building->navmesh, litv2(-10.f,  40.f));
            arrput(building->navmesh, litv2( 85.f,  10.f));
            arrput(building->navmesh, litv2( 85.f,-100.f));
            for (int i = 0 ; i < arrlen(building->navmesh); ++i) {
                int j = (i+1)%arrlen(building->navmesh);
                cdt_insert(&engine->navmesh, building->id, 
                           building->position.x + building->navmesh[i].x, building->position.y + building->navmesh[i].y,
                           building->position.x + building->navmesh[j].x, building->position.y + building->navmesh[j].y);
            }
        }
    }

    Entity *player = engine_alloc_entity(ENTITY_FLAG_DRAW|ENTITY_FLAG_ANIMATE|
                                         ENTITY_FLAG_MOUSE_CONTROL|ENTITY_FLAG_DIEABLE); 
    {
        player->position = litv2( 0.f,  0.f);
        player->size     = litv2(SPRITE_PLAYER_WIDTH, SPRITE_PLAYER_HEIGHT);
        player->radius   = 8.f;
        player->speed    = 150.0f;
        player->hp       = 100.f;
        player->texture  = TEXTURE_TYPE_PLAYER;
        player->offset   = litv2(0.f, -18.f);
        player->order    = ORDER_TYPE_IDLE;
    }

    {
        Entity *skeleton = engine_alloc_entity(ENTITY_FLAG_DRAW|ENTITY_FLAG_ANIMATE|ENTITY_FLAG_DIEABLE); 
        skeleton->position = litv2( 0.f, 80.f);
        skeleton->size     = litv2(SPRITE_SKELETON_WIDTH, SPRITE_SKELETON_HEIGHT);
        skeleton->radius   = 8.f;
        skeleton->speed    = 150.0f;
        skeleton->hp       = 100.f;
        skeleton->texture  = TEXTURE_TYPE_SKELETON;
        skeleton->offset   = litv2(0.f, -18.f);
        skeleton->order    = ORDER_TYPE_IDLE;
    }


    cdt_vec2 *edge_end_points = 0;
    int *is_constrained = 0;

    glfwSetWindowSize(engine->window, (int)(engine->resolution.x+0.5f), (int)(engine->resolution.y+0.5f));
    while (!glfwWindowShouldClose(engine->window)) {
        engine_tick();


        // Process events
        // 
        glfwPollEvents();    

        // Clear
        arrsetlen(edge_end_points, 0);
        arrsetlen(is_constrained, 0);
        glClearColor(0.05f,0.05f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);


        // Update entities
        engine->camera_position = player->position;
        for (Entity *entity = engine->entity_sentinel->next; entity != engine->entity_sentinel; entity = entity->next) {
            engine_update_entity(entity);
        }



        glViewport(0, 0, engine->framebuffer_width, engine->framebuffer_height);




        for (Entity *entity = engine->entity_sentinel->next; entity != engine->entity_sentinel; entity = entity->next) {
            engine_draw_entity(entity);
        }


        // Draw navmesh
        //
        for (int i = 0; i < engine->navmesh.edges.num; i+= 1) {
            cdt_edge *e = engine->navmesh.edges.data[i];
            arrput(edge_end_points, e->e[0].org->pos);
            arrput(edge_end_points, e->e[2].org->pos);
        }

        for (int i = 0; i < engine->navmesh.edges.num; i+= 1) {
            cdt_edge *e = engine->navmesh.edges.data[i];
            arrput(is_constrained, cdt_is_constrained(&e->e[0]));
        }


#if 0
        glUseProgram(engine->simple_shader);
        glEnableVertexAttribArray(0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        {
            GLsizei sz = sizeof(cdt_vec2);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sz, 0);
            glBufferData(GL_ARRAY_BUFFER, arrlen(edge_end_points)*sz, edge_end_points, GL_DYNAMIC_DRAW);
            for (int i = 0; i < arrlen(is_constrained); i+=1) {
                if (is_constrained[i]) {
                    glUniform4f(engine->simple_shader_color, 1.f, 1.f, 0.1f, 1.0f);
                } else {
                    glUniform4f(engine->simple_shader_color, 0.f, 0.f, 0.f, 1.0f);
                }
                M4x4 view_proj = m4x4_view_proj(engine->camera_position, engine->resolution);
                glUniformMatrix4fv(engine->simple_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);
                glDrawArrays(GL_LINES, i*2, 2);
            }
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisableVertexAttribArray(0);
        glUseProgram(0);
#endif


        glfwSwapBuffers(engine->window);
    }

    return 0;
}
