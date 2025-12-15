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
#define STB_SPRINTF_IMPLEMENTATION
#define STBDS_ASSERT assert
#include "../vendor/stb/stb_ds.h"
#include "../vendor/stb/stb_sprintf.h"
#include "../vendor/glad/glad.h"
#include "../vendor/GLFW/glfw3.h"
#include "../../cdt.h"
#include "ex_math.h"
#include "ex_ds.h"
#include "ex_engine.h"
#include "ex_gl.h"
#include "sprite/ex_sprite_player.h"
#include "sprite/ex_sprite_skeleton.h"
#include "sprite/ex_sprite_ground.h"
#include "sprite/ex_sprite_building.h"

// [.c]
//
#include "ex_math.cpp"
#include "ex_ds.cpp"
#include "pathfind.cpp"
#include "ex_engine.cpp"
#include "ex_gl.cpp"


int main(void) {
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1920, 1080, "Game", 0, 0);
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

    glEnable(GL_DEPTH_TEST); 
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



    cdt_init(&engine->navmesh.ctx, 0, 4096.f, -4096.f, -4096.f, 4096.f, -4096.f);

    Entity *ground = engine_alloc_entity(ENTITY_FLAG_DRAW|ENTITY_FLAG_DRAW_BACK); {
        f32 dim = 1024.f;
        f32 uv_repeat = 16.f;
        ground->size = vec2(dim, dim);
        ground->texture = TEXTURE_TYPE_GROUND;
        ground->u1 = 0.f;
        ground->v1 = 0.f;
        ground->u2 = uv_repeat;
        ground->v2 = uv_repeat;
        cdt_insert(&engine->navmesh.ctx, ground->id, -dim,  dim, -dim, -dim);
        cdt_insert(&engine->navmesh.ctx, ground->id, -dim, -dim,  dim, -dim);
        cdt_insert(&engine->navmesh.ctx, ground->id,  dim, -dim,  dim,  dim);
        cdt_insert(&engine->navmesh.ctx, ground->id,  dim,  dim, -dim,  dim);
    }

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            Entity *building   = engine_alloc_entity(ENTITY_FLAG_DRAW);
            building->position = vec2(-400.f+(175.f+r*10.f)*c, -400.f+(270.f+c*12.f)*r);
            building->size     = vec2(SPRITE_BUILDING_WIDTH, SPRITE_BUILDING_HEIGHT);
            building->texture  = TEXTURE_TYPE_BUILDING;
            building->offset   = vec2(0.f, -72.f);
            arrput(building->navmesh, vec2(  0.f,-120.f));
            arrput(building->navmesh, vec2(-85.f,-100.f));
            arrput(building->navmesh, vec2(-85.f,  10.f));
            arrput(building->navmesh, vec2(-10.f,  40.f));
            arrput(building->navmesh, vec2( 85.f,  10.f));
            arrput(building->navmesh, vec2( 85.f,-100.f));
            for (int i = 0 ; i < arrlen(building->navmesh); ++i) {
                int j = (i+1)%arrlen(building->navmesh);
                cdt_insert(&engine->navmesh.ctx, building->id, 
                           building->position.x + building->navmesh[i].x, building->position.y + building->navmesh[i].y,
                           building->position.x + building->navmesh[j].x, building->position.y + building->navmesh[j].y);
            }
        }
    }

    Entity *player = engine_alloc_entity(ENTITY_FLAG_DRAW|
                                         ENTITY_FLAG_ANIMATE|
                                         ENTITY_FLAG_MOUSE_CONTROL|
                                         ENTITY_FLAG_DIEABLE); 
    {
        player->position = vec2( 0.f,  0.f);
        player->size     = vec2(SPRITE_PLAYER_WIDTH, SPRITE_PLAYER_HEIGHT);
        player->radius   = 10.f;
        player->speed    = 250.0f;
        player->hp       = 100.f;
        player->texture  = TEXTURE_TYPE_PLAYER;
        player->offset   = vec2(0.f, -18.f);
        player->order    = ORDER_TYPE_IDLE;
    }

    {
        Entity *skeleton = engine_alloc_entity(ENTITY_FLAG_DRAW|ENTITY_FLAG_ANIMATE|ENTITY_FLAG_FOE|ENTITY_FLAG_DIEABLE); 
        skeleton->position = vec2( 0.f, 70.f);
        skeleton->size     = vec2(SPRITE_SKELETON_WIDTH, SPRITE_SKELETON_HEIGHT);
        skeleton->radius   = 20.f;
        skeleton->speed    = 150.0f;
        skeleton->hp       = 100.f;
        skeleton->texture  = TEXTURE_TYPE_SKELETON;
        skeleton->offset   = vec2(0.f, -18.f);
        skeleton->order    = ORDER_TYPE_IDLE;
    }

    {
        Entity *skeleton = engine_alloc_entity(ENTITY_FLAG_DRAW|ENTITY_FLAG_ANIMATE|ENTITY_FLAG_FOE|ENTITY_FLAG_DIEABLE); 
        skeleton->position = vec2(-200.f, 70.f);
        skeleton->size     = vec2(SPRITE_SKELETON_WIDTH, SPRITE_SKELETON_HEIGHT);
        skeleton->radius   = 20.f;
        skeleton->speed    = 120.0f;
        skeleton->hp       = 100.f;
        skeleton->texture  = TEXTURE_TYPE_SKELETON;
        skeleton->offset   = vec2(0.f, -18.f);
        skeleton->order    = ORDER_TYPE_IDLE;
    }


    glfwSetWindowSize(engine->window, (int)(engine->resolution.x+0.5f), (int)(engine->resolution.y+0.5f));
    while (!glfwWindowShouldClose(engine->window)) {
        engine_tick();
        engine->player_position = player->position;


        // Process events
        // 
        glfwPollEvents();    

        // Clear
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
        for (int i = 0; i < engine->navmesh.ctx.edges.num; ++i) {
            cdt_edge *edge = engine->navmesh.ctx.edges.data[i];

            Vec4 color = cdt_is_constrained(edge) ? Vec4{1,0,1,1} : Vec4{0,0,0,1};
            draw_push_colored_vertex(edge->e[0].org->pos.x, edge->e[0].org->pos.y, 0.3f, color);
            draw_push_colored_vertex(cdt_sym(&edge->e[0])->org->pos.x, cdt_sym(&edge->e[0])->org->pos.y, 0.3f, color);
        }

#if 1
        glUseProgram(engine->simple_shader);
        glDisable(GL_DEPTH_TEST); 
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        {
            int stride = 2;
            for (int base = 0; base < engine->line_shader_buffer.count; base+=stride) {
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Colored_Vertex), (void *)offsetof(Colored_Vertex, position));
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Colored_Vertex), (void *)offsetof(Colored_Vertex, color));
                glBufferData(GL_ARRAY_BUFFER, engine->line_shader_buffer.count*sizeof(Colored_Vertex), engine->line_shader_buffer.data, GL_DYNAMIC_DRAW);
                M4x4 view_proj = m4x4_view_proj(engine->camera_position, engine->resolution);
                glUniformMatrix4fv(engine->simple_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);
                glDrawArrays(GL_LINES, base, stride);
            }
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glEnable(GL_DEPTH_TEST); 
        glUseProgram(0);
#endif

        glfwSwapBuffers(engine->window);
    }

    return 0;
}
