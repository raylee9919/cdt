/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

void engine_create_texture(Texture_Type type, u16 width, u16 height, u16 sprite_width, u16 sprite_height, u16 num_row, u16 num_col, void *data) {
    Texture result = {0}; {
        result.type          = type;
        result.width         = width;
        result.height        = height;
        result.sprite_width  = sprite_width;
        result.sprite_height = sprite_height;
        result.num_col       = num_row;
        result.num_col       = num_col;
    }
    glGenTextures(1, &result.gl_id);
    glBindTexture(GL_TEXTURE_2D, result.gl_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    engine->textures[type] = result;
}

void engine_create_animation(Animation_Type type, Texture_Type spritesheet, u32 start_frame_index, u32 num_frames, f32 frame_interval) {
    engine->animations[type].spritesheet       = spritesheet;
    engine->animations[type].start_frame_index = start_frame_index;
    engine->animations[type].num_frames        = num_frames;
    engine->animations[type].frame_interval    = frame_interval;
}

void engine_init(GLFWwindow *window) {
    memset(engine, 0, sizeof(Engine));

    engine->window = window;
    engine->last_frame_time = glfwGetTime();

    engine->resolution.x = 1600.f;
    engine->resolution.y = 900.f;

    engine_create_texture(TEXTURE_TYPE_PLAYER, SPRITESHEET_PLAYER_WIDTH, SPRITESHEET_PLAYER_HEIGHT, SPRITE_PLAYER_WIDTH, SPRITE_PLAYER_HEIGHT, SPRITESHEET_PLAYER_ROWS, SPRITESHEET_PLAYER_COLUMNS, spritesheet_player);
    engine_create_texture(TEXTURE_TYPE_SKELETON, SPRITESHEET_SKELETON_WIDTH, SPRITESHEET_SKELETON_HEIGHT, SPRITE_SKELETON_WIDTH, SPRITE_SKELETON_HEIGHT, SPRITESHEET_SKELETON_ROWS, SPRITESHEET_SKELETON_COLUMNS, spritesheet_skeleton);
    engine_create_texture(TEXTURE_TYPE_GROUND, GRASS_TILE_WIDTH, GRASS_TILE_HEIGHT, GRASS_TILE_WIDTH, GRASS_TILE_HEIGHT, 1, 1, grass_tile);
    engine_create_texture(TEXTURE_TYPE_BUILDING, SPRITE_BUILDING_WIDTH, SPRITE_BUILDING_HEIGHT, SPRITE_BUILDING_WIDTH, SPRITE_BUILDING_HEIGHT, 1, 1, sprite_building);

    engine_create_animation(ANIMATION_TYPE_PLAYER_IDLE, TEXTURE_TYPE_PLAYER, 0, 4, 0.25f);
    engine_create_animation(ANIMATION_TYPE_PLAYER_RUN,  TEXTURE_TYPE_PLAYER, 8, 6, 0.1f);
    engine_create_animation(ANIMATION_TYPE_PLAYER_DIE,  TEXTURE_TYPE_PLAYER, 66, 3, 0.5f);
    engine_create_animation(ANIMATION_TYPE_SKELETON_ATTACK, TEXTURE_TYPE_SKELETON,  0, 8, 0.125f);
    engine_create_animation(ANIMATION_TYPE_SKELETON_DIE,    TEXTURE_TYPE_SKELETON,  8, 4, 0.125f);
    engine_create_animation(ANIMATION_TYPE_SKELETON_IDLE,   TEXTURE_TYPE_SKELETON, 16, 4, 0.25f);
    engine_create_animation(ANIMATION_TYPE_SKELETON_HIT,    TEXTURE_TYPE_SKELETON, 32, 4, 0.125f);
    engine_create_animation(ANIMATION_TYPE_SKELETON_MOVE,   TEXTURE_TYPE_SKELETON, 40, 4, 0.125f);


    // Init entity sentinel.
    engine->next_entity_id        = 1;
    engine->entity_sentinel       = (Entity *)malloc(sizeof(Entity));
    engine->entity_sentinel->next = engine->entity_sentinel;
    engine->entity_sentinel->prev = engine->entity_sentinel;

    // 'Spritesheet + Order = Animation' mapping
    engine->animation_map[TEXTURE_TYPE_PLAYER][ORDER_TYPE_IDLE] = ANIMATION_TYPE_PLAYER_IDLE;
    engine->animation_map[TEXTURE_TYPE_PLAYER][ORDER_TYPE_MOVE] = ANIMATION_TYPE_PLAYER_RUN;
    engine->animation_map[TEXTURE_TYPE_PLAYER][ORDER_TYPE_DIE]  = ANIMATION_TYPE_PLAYER_DIE;
    engine->animation_map[TEXTURE_TYPE_SKELETON][ORDER_TYPE_IDLE] = ANIMATION_TYPE_SKELETON_IDLE;
    engine->animation_map[TEXTURE_TYPE_SKELETON][ORDER_TYPE_MOVE] = ANIMATION_TYPE_SKELETON_MOVE;
    engine->animation_map[TEXTURE_TYPE_SKELETON][ORDER_TYPE_DIE]  = ANIMATION_TYPE_SKELETON_DIE;
}

void engine_tick(void) {
    engine->tick++;
    f64 new_frame_time = glfwGetTime();
    engine->dt = (f32)(new_frame_time - engine->last_frame_time);
    engine->last_frame_time = new_frame_time;
    engine->shader_time += engine->dt; // @Todo: Reset trick

    glfwGetFramebufferSize(engine->window, &engine->framebuffer_width, &engine->framebuffer_height);
}

Entity *engine_alloc_entity(Entity_Flags flags) {
    Entity *entity = (Entity *)malloc(sizeof(Entity));
    memset(entity, 0, sizeof(Entity));

    entity->id    = engine->next_entity_id++;
    entity->flags = flags;

    if (flags & ENTITY_FLAG_DRAW) {
        entity->u1 = 0.f;
        entity->v1 = 0.f;
        entity->u2 = 1.f;
        entity->v2 = 1.f;
    }

    // Add Chain
    entity->next = engine->entity_sentinel;
    entity->prev = engine->entity_sentinel->prev;
    engine->entity_sentinel->prev->next = entity;
    engine->entity_sentinel->prev = entity;

    return entity;
}

void engine_release_entity(Entity *entity) {
    // Remove chain
    entity->prev = entity->next;
    entity->next = entity->prev;

    free(entity);
}

void engine_update_entity(Entity *entity) {
    f32 dt = engine->dt;

    // Transition
    //
    if (entity->flags & ENTITY_FLAG_MOUSE_CONTROL && 
        (entity->order == ORDER_TYPE_IDLE || entity->order == ORDER_TYPE_MOVE)) 
    {
        int state = glfwGetMouseButton(engine->window, GLFW_MOUSE_BUTTON_RIGHT);
        if (state == GLFW_PRESS) {
            f64 x, y;
            glfwGetCursorPos(engine->window, &x, &y);

            // To NDC
            x =  2.f*(x / (f64)engine->framebuffer_width)  - 1.f;
            y = -2.f*(y / (f64)engine->framebuffer_height) + 1.f;

            // To camera space
            x *= ( engine->resolution.x*0.5f);
            y *= (-engine->resolution.y*0.5f);

            // To world space
            x += engine->camera_position.x;
            y += engine->camera_position.y;

            f32 dst_x = (f32)x;
            f32 dst_y = (f32)y;

            
            // Obtain triangles containing source and destination points each.
            //
            cdt_triangle src_tri = cdt_get_triangle_containing_point(&engine->navmesh, entity->position.x, entity->position.y);
            cdt_triangle dst_tri = cdt_get_triangle_containing_point(&engine->navmesh, dst_x, dst_y);
            int num_tri = cdt_get_triangle_count(&engine->navmesh);


            // A*
            //
            //dist;
            //fill(dist, INF);
            //dist[src_tri] = 0.f;

            //Vec2 src_center; {
            //    src_center.x = (src_tri.x[0] + src_tri.x[1] + src_tri.x[2]) * 0.333333f;
            //    src_center.y = (src_tri.y[0] + src_tri.y[1] + src_tri.y[2]) * 0.333333f;
            //}
            //Vec2 dst_center; {
            //    dst_center.x = (dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333f;
            //    dst_center.y = (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333f;
            //}

            //struct {
            //    f32 w;
            //    cdt_triangle tri;
            //} Weight_Tri;

            //while (!pq.empty()) {
            //    Weight_Tri weight_tri = pq.pop();
            //    f32 weight = weight_tri.w;
            //    cdt_triangle tri = weight_tri.tri;

            //    cdt_triangles adj = cdt_get_adjacent_triangles(tri);
            //    for (int i = 0; i < 3; ++i) {
            //        cdt_triangle adj_tri = adj.triangles[i];
            //        Vec2 adj_center; {
            //            adj_center.x = (adj_tri.x[0] + adj_tri.x[1] + adj_tri.x[2]) * 0.333333f;
            //            adj_center.y = (adj_tri.y[0] + adj_tri.y[1] + adj_tri.y[2]) * 0.333333f;
            //        }

            //        f32 new_dist = dist[tri] + distv2(tri_center, adj_center);
            //        if (dist[adj_center] > new_dist) {
            //            dist[adj_center] = new_dist;
            //            pq.push({new_dist, adj});
            //        }
            //    }
            //}




            { // @Temporary
                {
                    Vec2 v[] = {
                        litv2(src_tri.x[0], src_tri.y[0]),
                        litv2(src_tri.x[1], src_tri.y[1]),
                        litv2(src_tri.x[2], src_tri.y[2]),
                    };

                    glUseProgram(engine->simple_shader);
                    glEnableVertexAttribArray(0);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    {
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), 0);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);

                        glUniform4f(engine->simple_shader_color, 1.f, 0.f, 1.0f, 1.0f);
                        M4x4 view_proj = m4x4_view_proj(engine->camera_position, engine->resolution);
                        glUniformMatrix4fv(engine->simple_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);

                        glDrawArrays(GL_TRIANGLES, 0, 3);
                    }
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glDisableVertexAttribArray(0);
                    glUseProgram(0);
                }
                {
                    Vec2 v[] = {
                        litv2(dst_tri.x[0], dst_tri.y[0]),
                        litv2(dst_tri.x[1], dst_tri.y[1]),
                        litv2(dst_tri.x[2], dst_tri.y[2]),
                    };

                    glUseProgram(engine->simple_shader);
                    glEnableVertexAttribArray(0);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    {
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), 0);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);

                        glUniform4f(engine->simple_shader_color, 0.f, 1.f, 1.0f, 1.0f);
                        M4x4 view_proj = m4x4_view_proj(engine->camera_position, engine->resolution);
                        glUniformMatrix4fv(engine->simple_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);

                        glDrawArrays(GL_TRIANGLES, 0, 3);
                    }
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glDisableVertexAttribArray(0);
                    glUseProgram(0);
                }
            }
 
            



            entity->order = ORDER_TYPE_MOVE;
            entity->order_position = litv2((f32)x,(f32)y);


            // Cleanup
            //
            //free(dist);
        }
    }

    if (entity->flags & ENTITY_FLAG_DIEABLE) {
        if (entity->hp <= 0.f) {
            entity->order = ORDER_TYPE_DIE;
        }
    }

    // Order processing
    //
    switch (entity->order) {
        case ORDER_TYPE_IDLE: {
        } break;

        case ORDER_TYPE_MOVE: {
            f32 threshold = 1.0f;
            if (distv2(entity->order_position, entity->position) < threshold) {
                entity->order = ORDER_TYPE_IDLE;
            } else {
                Vec2 dir = normv2(subv2(entity->order_position, entity->position));
                Vec2 amount = mulv2f32(dir, dt*entity->speed);
                entity->position = addv2(entity->position, amount);

                if (entity->order_position.x - entity->position.x < 0.f) {
                    entity->flags |= ENTITY_FLAG_FLIP_TEX_U;
                } else {
                    entity->flags &= ~ENTITY_FLAG_FLIP_TEX_U;
                }
            } break;
        }

        case ORDER_TYPE_DIE: {
        } break;

        default: {
            assert(!"Invalid command.");
        } break;
    }

    if (entity->flags & ENTITY_FLAG_ANIMATE) {
        Animation_Type animation_type = engine->animation_map[entity->texture][entity->order];
        if (animation_type != ANIMATION_TYPE_INVALID) {
            Animation animation = engine->animations[animation_type];
            Texture sheet = engine->textures[animation.spritesheet];

            entity->animation_t += dt;
            while (entity->animation_t >= animation.frame_interval) {
                entity->animation_t -= animation.frame_interval;
                entity->animation_frame_offset += 1;
            }
            entity->animation_frame_offset %= animation.num_frames;

            int index  = animation.start_frame_index + entity->animation_frame_offset;
            int row    = index / sheet.num_col;
            int col    = index % sheet.num_col;
            f32 du     = (f32)sheet.sprite_width  / (f32)sheet.width;
            f32 dv     = (f32)sheet.sprite_height / (f32)sheet.height;
            entity->u1 = du*col;
            entity->v1 = dv*row;
            entity->u2 = entity->u1 + du;
            entity->v2 = entity->v1 + dv;
        }
    }

    if (entity->flags & ENTITY_FLAG_FLIP_TEX_U) {
        f32 tmp = entity->u1;
        entity->u1 = entity->u2;
        entity->u2 = tmp;
    }
}

void engine_draw_entity(Entity *entity) {
    if (!entity->flags & ENTITY_FLAG_DRAW) {
        return;
    }

    if (entity->texture != TEXTURE_TYPE_NULL) {
        Texture texture = engine->textures[entity->texture];

        glUseProgram(engine->sprite_shader);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glBindTexture(GL_TEXTURE_2D, texture.gl_id);
        {
            GLsizei sz = sizeof(Sprite_Vertex);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sz, (GLvoid *)offsetof(Sprite_Vertex, position));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sz, (GLvoid *)offsetof(Sprite_Vertex, uv));

            Sprite_Vertex vertices[4] = {
                {-entity->size.x, -entity->size.y, entity->u1, entity->v1},
                { entity->size.x, -entity->size.y, entity->u2, entity->v1},
                {-entity->size.x,  entity->size.y, entity->u1, entity->v2},
                { entity->size.x,  entity->size.y, entity->u2, entity->v2},
            };

            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

            // @Temporary:
            M4x4 model = m4x4_model(entity->position, entity->offset);
            glUniformMatrix4fv(engine->sprite_shader_model, 1, GL_TRUE, &model.e[0][0]);

            M4x4 view      = m4x4_view(engine->camera_position);
            M4x4 proj      = m4x4_proj(engine->resolution);
            M4x4 view_proj = m4x4_mul(proj, view);
            glUniformMatrix4fv(engine->sprite_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glUseProgram(0);
    }
}
