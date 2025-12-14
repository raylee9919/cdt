/* ========================================================================







   ======================================================================== */

Perf_Log::Perf_Log(const char *text_) {
    text = text_;
    t = glfwGetTime();
}

Perf_Log::~Perf_Log() {
    f64 elapsed_ms = (glfwGetTime() - t)*1000.f;
    char buf[256];
    stbsp_snprintf(buf, 256, "[LOG] %.6fms : %s\n", elapsed_ms, text);
    printf(buf);
}

void engine_create_texture(Texture_Type type, u16 width, u16 height, u16 sprite_width, u16 sprite_height, u16 num_row, u16 num_col, void *data) {
    Texture result = {}; {
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

    engine->resolution.x = 2560.f;
    engine->resolution.y = 1440.f;

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

void draw_push_colored_vertex(f32 x, f32 y, f32 z, Vec4 color) {
    Colored_Vertex vert = {};
    {
        vert.position.x = x;
        vert.position.y = y;
        vert.position.z = z; 
        vert.color      = color;
    }
   
    engine->line_shader_buffer.push(vert);
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
            printf("\n");
            Perf_Log total_perf_log("Path planning by mouse clicking.");

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
            cdt_triangle src_tri = {};
            cdt_triangle dst_tri = {};
            {
                Perf_Log perf_log("Get triangles containing source and destination points.");

                src_tri = cdt_get_triangle_containing_point(&engine->navmesh, entity->position.x, entity->position.y);
                dst_tri = cdt_get_triangle_containing_point(&engine->navmesh, dst_x, dst_y);
            }


            // Get all triangles in the subdivision. @Note: Stupid..
            //
            int num_tri = cdt_get_triangle_count(&engine->navmesh);
            cdt_triangle *triangles = (cdt_triangle *)malloc(sizeof(cdt_triangle)*num_tri);
            {
                Perf_Log perf_log("Get all triangles in the subdivision.");

                cdt_quad_edge **quad_edge_visited = 0;

                // It's just DFS.
                cdt_quad_edge_array stk = {};
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

            {
                // A*
                //
                // Preprocess
                f32 unreachable_dist = F32_MAX;
                f32 *dist = (f32 *)malloc(sizeof(f32)*num_tri);
                int *from_idx = (int *)malloc(sizeof(int)*num_tri);
                int src_idx = -1;
                int dst_idx = -1;
                for (int i = 0; i < num_tri; ++i) {
                    dist[i] = unreachable_dist; 
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

                Priority_Queue pq = {};
                Index_Dist first = {}; {
                    first.index = src_idx;
                    first.dist = 0.f;
                }
                enqueue(&pq, first);


                Vec2 dst_center = vec2((dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333f, 
                                       (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333f);


                {
                    Perf_Log perf_log("A* loop");
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
                        Vec2 tri_center = vec2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                                               (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

                        // @Todo: Broken heuristics
                        cdt_triangles adj = cdt_get_adjacent_triangles(tri);
                        for (int i = 0; i < 3; ++i) {
                            cdt_triangle adj_tri = adj.triangles[i];

                            cdt_edge *portal = cdt_get_edge(adj_tri.edges[0]);

                            // One cannot pass through a solid wall.
                            if (cdt_is_constrained(portal)) {
                                continue;
                            }

                            // One cannot pass through a narrow pass.
                            Vec2 p = vec2(portal->e[0].org->pos.x, portal->e[0].org->pos.y);
                            Vec2 q = vec2(portal->e[2].org->pos.x, portal->e[2].org->pos.y);
                            f32 eps = 0.01f;
                            if (distance(p,q) < entity->radius*2.f + eps) {
                                continue;
                            }

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

                            Vec2 adj_center = vec2((adj_tri.x[0] + adj_tri.x[1] + adj_tri.x[2]) * 0.333333f,
                                                   (adj_tri.y[0] + adj_tri.y[1] + adj_tri.y[2]) * 0.333333f);

                            f32 new_dist = dist[tri_idx] + distance(tri_center, adj_center) + distance(adj_center, dst_center);
                            if (dist[adj_idx] > new_dist) {
                                from_idx[adj_idx] = tri_idx;
                                dist[adj_idx] = new_dist;

                                Index_Dist new_entry = {}; {
                                    new_entry.index = adj_idx;
                                    new_entry.dist  = new_dist;
                                }
                                enqueue(&pq, new_entry);
                            }
                        }
                    }
                }


                // Gather portal edges' points.
                //
                Array<Vec2> l_points = {};
                Array<Vec2> r_points = {};
                {
                    Perf_Log perf_log("Path smoothing by Funnel.");

                    if (dist[dst_idx] != unreachable_dist) {
                        entity->path_shadow_queue.clear();
                        entity->triangles_in_path.clear();

                        entity->triangles_in_path.push({src_tri.x[0], src_tri.y[0]});
                        entity->triangles_in_path.push({src_tri.x[1], src_tri.y[1]});
                        entity->triangles_in_path.push({src_tri.x[2], src_tri.y[2]});

                        if (src_idx != dst_idx) {
                            for (int t = from_idx[dst_idx]; t != src_idx; t = from_idx[t]) {
                                cdt_triangle tri = triangles[t];
                                Vec2 tri_cen = vec2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                                                    (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

                                entity->triangles_in_path.push({tri.x[0], tri.y[0]});
                                entity->triangles_in_path.push({tri.x[1], tri.y[1]});
                                entity->triangles_in_path.push({tri.x[2], tri.y[2]});
                            }

                            entity->triangles_in_path.push({dst_tri.x[0], dst_tri.y[0]});
                            entity->triangles_in_path.push({dst_tri.x[1], dst_tri.y[1]});
                            entity->triangles_in_path.push({dst_tri.x[2], dst_tri.y[2]});
                        }

                        entity->order_position = entity->position;



                        Vec2 dst = {dst_x, dst_y};
                        l_points.push(dst);
                        r_points.push(dst);

                        if (src_idx != dst_idx) {
                            for (int t = dst_idx; t != src_idx; t = from_idx[t]) {
                                cdt_triangle tri = triangles[t];
                                cdt_quad_edge *portal = cdt_get_portal_edge(tri, triangles[from_idx[t]]);

                                Vec2 l = vec2(portal->org->pos.x, portal->org->pos.y);
                                Vec2 r = vec2(cdt_sym(portal)->org->pos.x, cdt_sym(portal)->org->pos.y);

                                // Deflate by the diameter of the entity.
                                Vec2 lr = normalize(r-l);
                                Vec2 rl = normalize(l-r);
                                l += lr*entity->radius;
                                r += rl*entity->radius;

                                l_points.push(l);
                                r_points.push(r);
                            }
                            Vec2 src = entity->position; 
                            l_points.push(src);
                            r_points.push(src);
                        }


                        // Run 'Simple Stupid Funnel' and push points to the queue.
                        //
                        entity->path_queue.clear();

                        int portal_count = l_points.count;
                        int apex_idx = portal_count - 1;
                        int l_idx    = portal_count - 1;
                        int r_idx    = portal_count - 1;
                        Vec2 apex  = entity->position;
                        Vec2 l_end = entity->position;
                        Vec2 r_end = entity->position;

                        entity->path_queue.push(entity->position);
                        for (int i = portal_count - 2; i >= 0; --i) {
                            Vec2 l = l_points[i];
                            Vec2 r = r_points[i];

                            if (orientation(l, {apex, l_end}) >= 0.f) {
                                if ((apex.x==l_end.x && apex.y==l_end.y) || (orientation(l, {apex, r_end}) < 0.f)) {
                                    l_end = l;
                                    l_idx = i;
                                } else {
                                    entity->path_queue.push(r_end);

                                    apex = r_end;
                                    apex_idx = r_idx;

                                    l_end = apex;
                                    r_end = apex;

                                    r_idx = apex_idx;
                                    l_idx = apex_idx;

                                    i = apex_idx;
                                    continue;
                                }
                            }

                            if (orientation(r, {apex, r_end}) <= 0.f) {
                                if ((apex.x==r_end.x && apex.y==r_end.y) || (orientation(r, {apex, l_end}) > 0.f)) {
                                    r_end = r;
                                    r_idx = i;
                                } else {
                                    entity->path_queue.push(l_end);

                                    apex = l_end;
                                    apex_idx = l_idx;

                                    r_end = apex;
                                    l_end = apex;

                                    l_idx = apex_idx;
                                    r_idx = apex_idx;

                                    i = apex_idx;
                                    continue;
                                }
                            }
                        }
                        entity->path_queue.push(dst);
                    }


                    // @Temporary: Copy to debug-purpose path shadow stack.
                    entity->path_shadow_queue = entity->path_queue;


                    // @Temporary: Draw portal edges.
                    for (int i = l_points.count - 1; i >= 0; --i) {
                        f32 t = parabolic_wave(engine->shader_time);
                        Vec3 c = lerp(vec3(0.5f), Vec3{1.f,1.f,0.f}, t*0.5f + 0.5f);
                        draw_push_colored_vertex(l_points.data[i].x, l_points.data[i].y, 0.f, vec4(c, 1.f));
                        draw_push_colored_vertex(r_points.data[i].x, r_points.data[i].y, 0.f, vec4(c, 1.f));
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
            if (distance(entity->position, entity->order_position) < eps) {
                if (!entity->path_queue.empty()) {
                    entity->order_position = entity->path_queue.pop();
                } else {
                    entity->order = ORDER_TYPE_IDLE;
                    entity->triangles_in_path.clear();
                }
            } else {
                Vec2 dir = normalize(entity->order_position - entity->position);
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
    if (!entity->path_queue.empty()) {
        for (int i = 0; i < entity->path_shadow_queue.count() - 1; ++i) {
            int idx1 = ((entity->path_shadow_queue.front + i) % arrcnt(entity->path_shadow_queue.data));
            int idx2 = ((entity->path_shadow_queue.front + i + 1) % arrcnt(entity->path_shadow_queue.data));
            Vec2 p1 = entity->path_shadow_queue.data[idx1];
            Vec2 p2 = entity->path_shadow_queue.data[idx2];
            draw_push_colored_vertex(p1.x, p1.y, 0.f, Vec4{0,0,1,1});
            draw_push_colored_vertex(p2.x, p2.y, 0.f, Vec4{0,0,1,1});
        }
    }
}
