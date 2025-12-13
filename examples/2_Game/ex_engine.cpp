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

    engine->resolution.x = 1920.f;
    engine->resolution.y = 1080.f;

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

    engine->line_shader_buffer.clear();
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
    if ((entity->flags & ENTITY_FLAG_MOUSE_CONTROL) && 
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


            // Get all triangles in the subdivision. @Note: Stupid..
            //
            int num_tri = cdt_get_triangle_count(&engine->navmesh);
            cdt_triangle *triangles = (cdt_triangle *)malloc(sizeof(cdt_triangle)*num_tri);
            {
                cdt_quad_edge **quad_edge_visited = 0;

                // It's just DFS.
                cdt_quad_edge_array stk = {0};
                cdt_quad_edge_array_push(&stk, &engine->navmesh.edges.data[0]->e[0]);
                int idx = 0;
                while (stk.num > 0) {
                    cdt_quad_edge *e1 = cdt_stack_pop(&stk);
                    cdt_quad_edge *e2 = cdt_lnext(e1);
                    cdt_quad_edge *e3 = cdt_lnext(e2);

                    int skip = 0;
                    for (int i = 0; i < arrlen(quad_edge_visited); ++i) {
                        if (quad_edge_visited[i]==e1) {
                            skip = 1;
                            break;
                        }
                    }
                    if (skip) { continue; }

                    arrput(quad_edge_visited, e1);
                    arrput(quad_edge_visited, e2);
                    arrput(quad_edge_visited, e3);

                    cdt_triangle *tri = &triangles[idx];
                    tri->edges[0] = e1;
                    tri->x[0]     = e1->org->pos.x;
                    tri->y[0]     = e1->org->pos.y;
                    tri->edges[1] = e2;
                    tri->x[1]     = e2->org->pos.x;
                    tri->y[1]     = e2->org->pos.y;
                    tri->edges[2] = e3;
                    tri->x[2]     = e3->org->pos.x;
                    tri->y[2]     = e3->org->pos.y;
                    idx++;

                    cdt_quad_edge_array_push(&stk, cdt_sym(e1));
                    cdt_quad_edge_array_push(&stk, cdt_sym(e2));
                    cdt_quad_edge_array_push(&stk, cdt_sym(e3));
                }
                assert(idx == num_tri);
                arrfree(quad_edge_visited);
            }

            // A*
            //
            {
                // Preprocess
                f32 unreachable = F32_MAX;
                f32 *dist = (f32 *)malloc(sizeof(f32)*num_tri);
                int *from_idx = (int *)malloc(sizeof(int)*num_tri);
                int src_idx = -1;
                int dst_idx = -1;
                for (int i = 0; i < num_tri; ++i) {
                    dist[i] = unreachable; 
                    from_idx[i] = i;
                }

                for (int i = 0; i < num_tri; ++i) {
                    if (triangles[i].edges[0] == src_tri.edges[0] ||
                        triangles[i].edges[1] == src_tri.edges[0] ||
                        triangles[i].edges[2] == src_tri.edges[0]) 
                    {
                        src_idx = i;
                        dist[i] = 0.f;
                        break;
                    }
                }
                from_idx[src_idx] = -1;

                for (int i = 0; i < num_tri; ++i) {
                    if (triangles[i].edges[0] == dst_tri.edges[0] ||
                        triangles[i].edges[1] == dst_tri.edges[0] ||
                        triangles[i].edges[2] == dst_tri.edges[0]) 
                    {
                        dst_idx = i;
                    }
                }
                assert(src_idx != -1 && dst_idx != -1);

                Priority_Queue pq = {0};
                Index_Dist first = {0}; {
                    first.index = src_idx;
                    first.dist = 0.f;
                }
                enqueue(&pq, first);


                Vec2 dst_center = litvec2((dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333f, 
                                          (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333f);


                while (pq.size > 0) {
                    Index_Dist index_dist = dequeue(&pq);
                    int tri_idx = index_dist.index;
                    f32 current_dist = index_dist.dist;

                    if (tri_idx == dst_idx) {
                        break; 
                    }

                    if (current_dist > dist[tri_idx]) {
                        continue;
                    }

                    cdt_triangle tri = triangles[tri_idx];
                    Vec2 tri_center = litvec2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                                            (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

                    // @Todo: Broken heuristics
                    cdt_triangles adj = cdt_get_adjacent_triangles(tri);
                    for (int i = 0; i < 3; ++i) {
                        cdt_triangle adj_tri = adj.triangles[i];
                        if (!cdt_is_constrained(cdt_get_edge(adj_tri.edges[0]))) {
                            int adj_idx = -1;
                            for (int j = 0; j < num_tri; ++j) {
                                if (triangles[j].edges[0] == adj_tri.edges[0] ||
                                    triangles[j].edges[1] == adj_tri.edges[0] ||
                                    triangles[j].edges[2] == adj_tri.edges[0]) 
                                {
                                    adj_idx = j;
                                }
                            }
                            assert(adj_idx != -1);

                            Vec2 adj_center = litvec2((adj_tri.x[0] + adj_tri.x[1] + adj_tri.x[2]) * 0.333333f,
                                                    (adj_tri.y[0] + adj_tri.y[1] + adj_tri.y[2]) * 0.333333f);

                            f32 new_dist = dist[tri_idx] + distv2(tri_center, adj_center) + distv2(adj_center, dst_center);
                            if (dist[adj_idx] > new_dist) {
                                from_idx[adj_idx] = tri_idx;
                                dist[adj_idx] = new_dist;

                                Index_Dist new_entry = {0}; {
                                    new_entry.index = adj_idx;
                                    new_entry.dist  = new_dist;
                                }
                                enqueue(&pq, new_entry);
                            }
                        }
                    }
                }


                // Trace
                if (dist[dst_idx] != unreachable) {
                    stack_clear(&entity->path_stack);
                    stack_clear(&entity->path_shadow_stack);
                    entity->triangles_in_path.clear();

                    entity->triangles_in_path.push({src_tri.x[0], src_tri.y[0]});
                    entity->triangles_in_path.push({src_tri.x[1], src_tri.y[1]});
                    entity->triangles_in_path.push({src_tri.x[2], src_tri.y[2]});

                    stack_push(&entity->path_stack, litvec2(dst_x, dst_y));
                    if (src_idx != dst_idx) {
                        for (int t = from_idx[dst_idx]; t != src_idx; t = from_idx[t]) {
                            cdt_triangle tri = triangles[t];
                            Vec2 tri_cen = litvec2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                                                   (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

                            stack_push(&entity->path_stack, tri_cen);
                            entity->triangles_in_path.push({tri.x[0], tri.y[0]});
                            entity->triangles_in_path.push({tri.x[1], tri.y[1]});
                            entity->triangles_in_path.push({tri.x[2], tri.y[2]});
                        }


                        stack_push(&entity->path_stack, entity->position);
                        entity->triangles_in_path.push({dst_tri.x[0], dst_tri.y[0]});
                        entity->triangles_in_path.push({dst_tri.x[1], dst_tri.y[1]});
                        entity->triangles_in_path.push({dst_tri.x[2], dst_tri.y[2]});
                    }
                    entity->order_position = entity->position;

                    // Copy to debug-purpose path shadow stack.
                    for (int i = entity->path_stack.top - 1; i >= 0; --i) {
                        stack_push(&entity->path_shadow_stack, entity->path_stack.data[i]);
                    }
                }


                // Cleanup
                free(dist);
                free(from_idx);
            }

            entity->order = ORDER_TYPE_MOVE;


            // Cleanup
            //
            free(triangles);
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
            f32 eps = 1.f;
            if (distv2(entity->position, entity->order_position) < eps) {
                if (!stack_empty(&entity->path_stack)) {
                    entity->order_position = stack_pop(&entity->path_stack);
                } else {
                    entity->order = ORDER_TYPE_IDLE;
                    entity->triangles_in_path.clear();
                }
            } else {
                Vec2 dir = normv2(entity->order_position - entity->position);
                Vec2 amount = dir * dt*entity->speed;
                entity->position = entity->position + amount;

                if (entity->order_position.x - entity->position.x < 0.f) {
                    entity->flags |= ENTITY_FLAG_FLIP_TEX_U;
                } else {
                    entity->flags &= ~ENTITY_FLAG_FLIP_TEX_U;
                }
            }
        } break;

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

void draw_push_colored_vertex(f32 x, f32 y, f32 z, Vec4 color) {
    Colored_Vertex vert = {0}; {
        vert.position.x = x;
        vert.position.y = y;
        vert.position.z = z; 
        vert.color      = color;
    }
    engine->line_shader_buffer.push(vert);

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
            M4x4 view_proj = proj*view;
            glUniformMatrix4fv(engine->sprite_shader_vp, 1, GL_TRUE, &view_proj.e[0][0]);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glUseProgram(0);
    }

    if (!entity->triangles_in_path.empty()) {
        for (int i = 0; i < entity->triangles_in_path.count; i+=3) {
            for (int j = 0; j < 3; ++j) {
                int d[] = {1, 1,-2};
                int k = j + d[j];
                int idx1 = i+j;
                int idx2 = i+k;

                draw_push_colored_vertex(entity->triangles_in_path[idx1].x, entity->triangles_in_path[idx1].y, 0.1f, Vec4{1,1,1,1});
                draw_push_colored_vertex(entity->triangles_in_path[idx2].x, entity->triangles_in_path[idx2].y, 0.1f, Vec4{1,1,1,1});
            }
        }
    }

    // Draw actual route
    //
    if (!stack_empty(&entity->path_stack)) {
        for (int i = entity->path_shadow_stack.top - 1; i > 0; --i) {
            int j = i - 1;
            Vec2 p1 = entity->path_shadow_stack.data[i];
            Vec2 p2 = entity->path_shadow_stack.data[j];

            draw_push_colored_vertex(p1.x, p1.y, 0.f, Vec4{0,0,1,1});
            draw_push_colored_vertex(p2.x, p2.y, 0.f, Vec4{0,0,1,1});
        }
    }

}
