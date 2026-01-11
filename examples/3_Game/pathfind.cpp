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

    // @Todo: This is getting gnarly. I'm iterating over all triangles to 
    //        find the indices of the two triangles found above.
    //
    //        Feasible solutions include:
    //
    //        1. Use a hash table for efficient lookups. Consider hashing the 
    //           three sorted vertex coordinates of a triangle as the hash key.
    //        2. Retain triangle data within the library, though this approach 
    //           appears challenging.
    //        3. Switch to a triangle-based approach, but this likely comse with 
    //           is own pros and cons.
    //        
    //        The first solution can be implemented easily by the users, but the 
    //        rest is really my responsibility.
    //
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
            break;
        }
    }

    // A*
    //
    // Uses Euclidean distance to the destination triangle as the heuristic.
    //
    // Uses the sum of distance from each triangle centroid to the shared edge 
    // as the edge weight.
    //

    // Preprocess
    f32 unreachable_dist = F32_MAX;
    f32 *f_costs = (f32 *)malloc(sizeof(f32)*num_tri);
    int *parent  = (int *)malloc(sizeof(int)*num_tri);
    for (int i = 0; i < num_tri; ++i) {
        f_costs[i] = unreachable_dist; 
        parent[i] = i;
    }
    f_costs[src_idx] = 0.f;
    parent[src_idx] = -1;

    Priority_Queue open_list = {};
    Index_Dist first = {}; {
        first.index = src_idx;
        first.dist  = 0.f;
    }
    open_list.push(first);

    Vec2 dst_center = vec2((dst_tri.x[0] + dst_tri.x[1] + dst_tri.x[2]) * 0.333333f, 
                           (dst_tri.y[0] + dst_tri.y[1] + dst_tri.y[2]) * 0.333333f);


    // A* loop: f_cost = g_cost + h_cost(heuristic)
    //
    while (open_list.size > 0) {
        // Pop the shortest in the open list. Implemented with priority queue.
        Index_Dist index_dist = open_list.pop();
        int idx_cur    = index_dist.index;
        f32 f_cost_cur = index_dist.dist;

        // Reached the destination triangle.
        if (idx_cur == dst_idx) {
            break; 
        }

        if (f_cost_cur > f_costs[idx_cur]) {
            continue;
        }

        cdt_triangle tri = triangles[idx_cur];
        Vec2 tri_center = vec2((tri.x[0] + tri.x[1] + tri.x[2]) * 0.333333f,
                               (tri.y[0] + tri.y[1] + tri.y[2]) * 0.333333f);

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
            f32 margin = 0.01f;
            if (distance(p,q) < entity->radius*2.f + margin) {
                continue;
            }
            Vec2 edge_center = (p+q)*0.5f;

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

            f32 h_cost_cur   = distance(tri_center, dst_center);
            f32 g_cost_cur   = f_cost_cur - h_cost_cur;
            f32 g_cur_to_adj = distance(tri_center, edge_center) + distance(edge_center, adj_center);
            f32 g_cost_adj   = g_cost_cur + g_cur_to_adj;
            f32 h_cost_adj   = distance(adj_center, dst_center);
            f32 f_cost_new   = g_cost_adj + h_cost_adj;

            if (f_costs[adj_idx] > f_cost_new) {
                parent[adj_idx] = idx_cur;
                f_costs[adj_idx]  = f_cost_new;

                Index_Dist new_entry = {}; {
                    new_entry.index = adj_idx;
                    new_entry.dist  = f_cost_new;
                }
                open_list.push(new_entry);
            }
        }
    }


    // Gather portal edges' points.
    //
    if (f_costs[dst_idx] != unreachable_dist) {
        entity->debug_triangles_in_path.push({src_tri.x[0], src_tri.y[0]});
        entity->debug_triangles_in_path.push({src_tri.x[1], src_tri.y[1]});
        entity->debug_triangles_in_path.push({src_tri.x[2], src_tri.y[2]});

        if (src_idx != dst_idx) {
            for (int t = parent[dst_idx]; t != src_idx; t = parent[t]) {
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

        // @Todo: ??
        entity->order_position = entity->position;


        entity->l_points.push(dst);
        entity->r_points.push(dst);
        if (src_idx != dst_idx) {
            for (int t = dst_idx; t != src_idx; t = parent[t]) {
                cdt_triangle tri = triangles[t];
                cdt_quad_edge *portal = cdt_get_portal_edge(tri, triangles[parent[t]]);

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


    // Cleanup.
    free(f_costs);
    free(parent);
}
