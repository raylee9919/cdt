/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */


void path_find(Entity *entity, Vec2 src, Vec2 dst) {
    // Clear old path data.
    //
    entity->path_queue.clear();
    entity->l_points.clear();
    entity->r_points.clear();
    entity->debug_path_queue.clear();
    entity->debug_triangles_in_path.clear();


    // Get all triangles in the subdivision.
    // 
    // This shouldn't be here. Ideally, gather all triangles at the beginning of 
    // the frame and share them across entities. This is kept only for user 
    // readability.
    //
    cdt_context *ctx = &engine->navmesh.ctx;
    free(engine->navmesh.triangles);
    int num_tri = cdt_get_triangle_count(ctx);
    engine->navmesh.num_tri = num_tri;
    engine->navmesh.triangles = (cdt_triangle *)malloc(sizeof(cdt_triangle)*num_tri);
    cdt_get_all_triangles(ctx, engine->navmesh.triangles);
    cdt_triangle *triangles = engine->navmesh.triangles;


    // Find the triangles that contain the source and destination points.
    //
    cdt_triangle src_tri = cdt_get_triangle_containing_point(ctx, src.x, src.y);
    cdt_triangle dst_tri = cdt_get_triangle_containing_point(ctx, dst.x, dst.y);

    int src_idx = -1;
    int dst_idx = -1;

    for (int i = 0; i < num_tri; ++i) {
        if (triangles[i].edges[0] == src_tri.edges[0] ||
            triangles[i].edges[1] == src_tri.edges[0] ||
            triangles[i].edges[2] == src_tri.edges[0]) 
        {
            src_idx = i;
            break;
        }
    }

    for (int i = 0; i < num_tri; ++i) {
        if (triangles[i].edges[0] == dst_tri.edges[0] ||
            triangles[i].edges[1] == dst_tri.edges[0] ||
            triangles[i].edges[2] == dst_tri.edges[0]) 
        {
            dst_idx = i;
        }
    }

    // A*
    //
    // Preprocess
    f32 unreachable_dist = F32_MAX;
    f32 *dist = (f32 *)malloc(sizeof(f32)*num_tri);
    int *from_idx = (int *)malloc(sizeof(int)*num_tri);
    for (int i = 0; i < num_tri; ++i) {
        dist[i] = unreachable_dist; 
        from_idx[i] = i;
    }
    dist[src_idx] = 0.f;
    from_idx[src_idx] = -1;

    Priority_Queue pq = {};
    Index_Dist first = {}; {
        first.index = src_idx;
        first.dist = 0.f;
    }
    enqueue(&pq, first);

    Vec2 dst_center = vec2((dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333f, 
                           (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333f);


    // A* loop
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

            cdt_edge *portal_edge = cdt_get_edge(tri.edges[i]);

            // One cannot pass through a solid wall.
            if (cdt_is_constrained(portal_edge)) {
                continue;
            }

            // One cannot pass through a narrow pass.
            Vec2 p = vec2(portal_edge->e[2].org->pos.x, portal_edge->e[2].org->pos.y);
            Vec2 q = vec2(portal_edge->e[0].org->pos.x, portal_edge->e[0].org->pos.y);
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


    // Gather portal edges' points.
    //
    if (dist[dst_idx] != unreachable_dist) {
        entity->debug_triangles_in_path.push({src_tri.x[0], src_tri.y[0]});
        entity->debug_triangles_in_path.push({src_tri.x[1], src_tri.y[1]});
        entity->debug_triangles_in_path.push({src_tri.x[2], src_tri.y[2]});

        if (src_idx != dst_idx) {
            for (int t = from_idx[dst_idx]; t != src_idx; t = from_idx[t]) {
                cdt_triangle tri = triangles[t];
                Vec2 tri_cen = vec2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                                    (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

                entity->debug_triangles_in_path.push({tri.x[0], tri.y[0]});
                entity->debug_triangles_in_path.push({tri.x[1], tri.y[1]});
                entity->debug_triangles_in_path.push({tri.x[2], tri.y[2]});
            }

            entity->debug_triangles_in_path.push({dst_tri.x[0], dst_tri.y[0]});
            entity->debug_triangles_in_path.push({dst_tri.x[1], dst_tri.y[1]});
            entity->debug_triangles_in_path.push({dst_tri.x[2], dst_tri.y[2]});
        }

        entity->order_position = entity->position;


        entity->l_points.push(dst);
        entity->r_points.push(dst);
        if (src_idx != dst_idx) {
            for (int t = dst_idx; t != src_idx; t = from_idx[t]) {
                cdt_triangle tri = triangles[t];
                cdt_quad_edge *portal = cdt_get_portal_edge(tri, triangles[from_idx[t]]);

                Vec2 l = vec2(portal->org->pos.x, portal->org->pos.y);
                Vec2 r = vec2(cdt_sym(portal)->org->pos.x, cdt_sym(portal)->org->pos.y);

                // Deflate the edge widths by the entity's diameter.
                Vec2 lr = normalize(r-l);
                Vec2 rl = normalize(l-r);
                l += lr*entity->radius;
                r += rl*entity->radius;

                entity->l_points.push(l);
                entity->r_points.push(r);
            }
            entity->l_points.push(src);
            entity->r_points.push(src);
        }


        // Run the 'Simple Stupid Funnel' and push the resulting waypoints to the queue.
        // https://digestingduck.blogspot.com/2010/03/simple-stupid-funnel-algorithm.html
        //
        // @Todo: Can I do better?
        //

        int portal_count = entity->l_points.count;
        int apex_idx = portal_count - 1;
        int l_idx    = portal_count - 1;
        int r_idx    = portal_count - 1;
        Vec2 apex  = entity->position;
        Vec2 l_end = entity->position;
        Vec2 r_end = entity->position;

        entity->path_queue.push(entity->position);
        for (int i = portal_count - 2; i >= 0; --i) {
            Vec2 l = entity->l_points[i];
            Vec2 r = entity->r_points[i];

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


    // @Temporary: Copy to debug-only path queue.
    entity->debug_path_queue = entity->path_queue;


    // Cleanup. If you are reading this, just use an arena allocator.
    free(dist);
    free(from_idx);
}
