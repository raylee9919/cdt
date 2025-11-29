#ifndef SW_CDT_H_INCLUDE
#define SW_CDT_H_INCLUDE
/* ========================================================================

   (C) Copyright 2025 by Seong Woo Lee, All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   ======================================================================== */

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// @Important: !!!!!!! DO NOT INTERSECT CONSTRAINTS, DEAR USERS !!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// @Todo: 
//        [ ] There's a bug in locate_point. Infinite loop.
//        [ ] Remove STL.
//        [ ] Remove printfs.
//        [ ] Remove new/delete.
//        [ ] Spatial Partitioning?
//        [ ] Intersecting contraints support.

#include <stdlib.h>
#include <stdint.h>

#include <vector>
#include <set>
#include <stack>
#include <queue>
#include <list>

#define cdt_assert(exp) if (!(exp)) {*(volatile int*)0=0;}
typedef float   float32;
typedef uint32_t Id;

typedef struct Quad_Edge Quad_Edge;

typedef struct {
    float32 x, y;
} Vec2;

typedef struct {
    Vec2 pos;
    std::list<Quad_Edge *> edges; // A list of quad-edges originating from the vertex.
} Vertex;

struct Quad_Edge {
    Vertex    *org;
    Vertex    *debug_dst; // @Temporary: 
    Quad_Edge *onext_ptr;
    uint8_t    idx;       // [0,3]
    uint8_t    visited;
};

typedef struct {
    Quad_Edge e[4];
    std::list<Id> ids;
} Edge;

typedef struct {
    std::list<Vertex *> vertices;
    std::list<Edge *>   edges;
} Context;

typedef struct {
    bool        exists;
    bool        on_edge;
    Vertex     *vertex;
    Quad_Edge  *edge;
} Locate_Result;

typedef struct {
    Vertex *vert;
    float32 dx;
    float32 dy;
} Vertex_Sort_Struct;



void debug_log(Context *ctx) {
    FILE *file = fopen("subdivision.log", "wb");
    if (file) {
        fprintf(file, "\n");
        fprintf(file, "segments = [\n");
        for (Edge *e : ctx->edges) {
            fprintf(file, "    ((%.2f, %.2f), (%.2f, %.2f)),\n", e->e[0].org->pos.x, e->e[0].org->pos.y, e->e[2].org->pos.x, e->e[2].org->pos.y);
        }
        fprintf(file, "]\n");
        fclose(file);
    }
}


// Quad_Edge Algebra
// 
Edge *get_edge(Quad_Edge *e) {
    return (Edge *)(e - e->idx);
}

Quad_Edge *rot(Quad_Edge *e) {
    int off[4] = {1,2,3,0};
    return e - e->idx + off[e->idx];
}

Quad_Edge *inv_rot(Quad_Edge *e) {
    int off[4] = {3,0,1,2};
    return e - e->idx + off[e->idx];
}

Quad_Edge *sym(Quad_Edge *e) {
    int off[4] = {2,3,0,1};
    return e - e->idx + off[e->idx];
}

Vertex *dst(Quad_Edge *e) {
    return sym(e)->org;
}

Quad_Edge *onext(Quad_Edge *e) {
    return e->onext_ptr;
}

Quad_Edge *oprev(Quad_Edge *e) {
    return rot(onext(rot(e)));
}

Quad_Edge *lnext(Quad_Edge *e) {
    return rot(onext(inv_rot(e)));
}

Quad_Edge *lprev(Quad_Edge *e) {
    return sym(onext(e));
}

Quad_Edge *dnext(Quad_Edge *e) {
    return sym(onext(sym(e)));
}

Quad_Edge *dprev(Quad_Edge *e) {
    return inv_rot(onext(inv_rot(e)));
}

Quad_Edge *rprev(Quad_Edge *e) {
    return onext(sym(e));
}

bool constrained(Quad_Edge *e) {
    return !get_edge(e)->ids.empty();
}

void splice(Quad_Edge *a, Quad_Edge *b) {
    Quad_Edge *alpha = rot(onext(a));
    Quad_Edge *beta  = rot(onext(b));

    Quad_Edge *b_onext = onext(b);
    b->onext_ptr = onext(a);
    a->onext_ptr = b_onext;

    Quad_Edge *alpha_onext = onext(alpha);
    alpha->onext_ptr = onext(beta);
    beta->onext_ptr  = alpha_onext;
}

void swap(Quad_Edge *e) {
    // Remove original reference from the vertices.
    e->org->edges.remove(e);
    dst(e)->edges.remove(sym(e));
 
    Quad_Edge *a = oprev(e);
    Quad_Edge *b = oprev(sym(e));
    splice(e, a);
    splice(sym(e), b);
    splice(e, lnext(a));
    splice(sym(e), lnext(b));

    e->org = dst(a);
    sym(e)->org = dst(b);

    // Add new references.
    e->org->edges.push_back(e);
    dst(e)->edges.push_back(sym(e));
}

Quad_Edge *create_edge(Context *ctx, Vertex *org, Vertex *dst) {
    Edge *edge = new Edge;

    ctx->edges.push_back(edge);

    edge->e[0].idx = 0;
    edge->e[1].idx = 1;
    edge->e[2].idx = 2;
    edge->e[3].idx = 3;

    edge->e[0].visited = 0;
    edge->e[1].visited = 0;
    edge->e[2].visited = 0;
    edge->e[3].visited = 0;

    // Primary
    edge->e[0].onext_ptr = &(edge->e[0]);
    edge->e[2].onext_ptr = &(edge->e[2]);
    edge->e[0].org = org;
    edge->e[2].org = dst;

    edge->e[0].debug_dst = dst;
    edge->e[2].debug_dst = org;

    // Dual
    edge->e[1].onext_ptr = &(edge->e[3]);
    edge->e[3].onext_ptr = &(edge->e[1]);

    // Add reference to the two endpoint vertices.
    org->edges.push_back(&edge->e[0]);
    dst->edges.push_back(&edge->e[2]);

    return edge->e;
}

void destroy_edge(Context *ctx, Quad_Edge *e) {
    // Remove reference from the two endpoint vertices.
    e->org->edges.remove(e);
    dst(e)->edges.remove(sym(e));

    splice(e, oprev(e));
    splice(sym(e), oprev(sym(e)));

    Edge *edge = get_edge(e);
    ctx->edges.remove(edge);

    delete edge;
}

Quad_Edge *connect(Context *ctx, Quad_Edge *a, Quad_Edge *b) {
    Quad_Edge *e = create_edge(ctx, dst(a), b->org);
    splice(e, lnext(a));
    splice(sym(e), b);
    return e;
}


// Geometry/Math
//
float32 orientation(Vec2 a, Vec2 b, Vec2 c) {
    // Returns twice the area of the triangle. Signed if the orientation is clockwise.
    // @Todo: SIMD?
    float32 x1 = b.x - a.x;
    float32 y1 = b.y - a.y;
    float32 x2 = c.x - a.x;
    float32 y2 = c.y - a.y;
    return (x1*y2 - x2*y1);
}

// @Todo: Epsilon allowance?
bool right_of(Vec2 p, Quad_Edge *e) {
    return orientation(e->org->pos, dst(e)->pos, p) < 0.f;
}

bool left_of(Vec2 p, Quad_Edge *e) {
    return orientation(e->org->pos, dst(e)->pos, p) > 0.f;
}

bool on_line(Vec2 p, Vec2 org, Vec2 dst) {
    return orientation(org, dst, p) == 0.f;
}

bool in_circumcircle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    // @Todo: Better math?
    //        "It is worth to mention that we have faced never-ending loops during 
    //         the flipping process when using only a simple determinant evaluation." -Kallmann
    //
    //        Shewchuck's precise math predicate seems solid.
    //
    float32 ax = a.x; float32 ay = a.y;
    float32 bx = b.x; float32 by = b.y;
    float32 cx = c.x; float32 cy = c.y;
    float32 px = p.x; float32 py = p.y;
    float32 ax_ = ax-px;
    float32 ay_ = ay-py;
    float32 bx_ = bx-px;
    float32 by_ = by-py;
    float32 cx_ = cx-px;
    float32 cy_ = cy-py;
    return ((ax_*ax_ + ay_*ay_) * (bx_*cy_-cx_*by_) -
            (bx_*bx_ + by_*by_) * (ax_*cy_-cx_*ay_) +
            (cx_*cx_ + cy_*cy_) * (ax_*by_-bx_*ay_)) > 0;
}

bool is_convex(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    // @Todo: SIMD?
    float32 x1 = b.x - a.x;
    float32 y1 = b.y - a.y;
    float32 x2 = c.x - b.x;
    float32 y2 = c.y - b.y;
    float32 x3 = d.x - c.x;
    float32 y3 = d.y - c.y;
    float32 x4 = a.x - d.x;
    float32 y4 = a.y - d.y;
    float32 c1 = x1*y2 - x2*y1;
    float32 c2 = x2*y3 - x3*y2;
    float32 c3 = x3*y4 - x4*y3;
    float32 c4 = x4*y1 - x1*y4;

    return (c1*c2 > 0.f) && (c2*c3 > 0.f) && (c3*c4 > 0.f);
}

// This is for ear-clipping.
bool cdt_in_triangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    Vec2 ab = {b.x - a.x, b.y - a.y};
    Vec2 bc = {c.x - b.x, c.y - b.y};
    Vec2 ca = {a.x - c.x, a.y - c.y};
    Vec2 ap = {p.x - a.x, p.y - a.y};
    Vec2 bp = {p.x - b.x, p.y - b.y};
    Vec2 cp = {p.x - c.x, p.y - c.y};
    float32 oa = ab.x*ap.y - ab.y*ap.x;
    float32 ob = bc.x*bp.y - bc.y*bp.x;
    float32 oc = ca.x*cp.y - ca.y*cp.x;
    // If the point is on the edge of a triangle, we can clip that ear triangle.
    return oa >= 0.f && ob >= 0.f && oc >= 0.f; 
}

Vertex *create_vertex(Context *ctx, Vec2 pos) {
    Vertex *result = new Vertex;
    result->pos = pos;
    ctx->vertices.push_back(result);
    return result;
}

void flip_until_stack_is_empty(std::stack<Quad_Edge *> &stk) {
    // Flip
    while (!stk.empty()) {
        Quad_Edge *e = stk.top();
        stk.pop();

        // @Robustness
        if (!constrained(e)) {
            if (in_circumcircle(dprev(e)->org->pos, dst(e)->pos, e->org->pos, dnext(e)->org->pos)) {
                swap(e);
                stk.push(lnext(e));
                stk.push(dnext(e));
            }
        }
    }
}


typedef struct Index_Node {
    uint32_t idx;
    Index_Node *next;
} Index_Node;

void cdt_ear_triangulate_simple_polygon(Context *ctx, int num_verts, Vertex_Sort_Struct *verts) {
    std::stack<Quad_Edge *>new_edges;

    // Preprocessing..
    int num = num_verts;
    Index_Node *nodes = (Index_Node *)malloc(sizeof(Index_Node)*num);
    for (int i = 0; i < num; ++i) {
        nodes[i].next = &nodes[(i+1)%num];
        nodes[i].idx  = i;
    }
    Index_Node *node_first = &nodes[0];

    while (num > 3) {
        Index_Node *left_of_tip = 0;
        for (int ii = 0; ii < num; ++ii) {
            Index_Node *node_lft = node_first;
            for (int i = 0; i < ii; ++i) { node_lft = node_lft->next; }
            Index_Node *node_tip = node_lft->next;
            Index_Node *node_rgt = node_tip->next;

            int idx0 = node_lft->idx;
            int idx1 = node_tip->idx;
            int idx2 = node_rgt->idx;

            Vec2 p[3] = { verts[idx0].vert->pos, verts[idx1].vert->pos, verts[idx2].vert->pos };
            Vec2 e[2] = { 
                { p[1].x - p[0].x, p[1].y - p[0].y },
                { p[2].x - p[0].x, p[2].y - p[0].y },
            };

            bool is_ear = 1;
            float orientation = e[0].x*e[1].y - e[0].y*e[1].x;
            if (orientation > 0.f) {
                for (Index_Node *node = node_rgt->next; node != node_lft; node = node->next) {
                    Vec2 v = verts[node->idx].vert->pos;
                    if (cdt_in_triangle(v, p[0], p[1], p[2])) { // This isn't an ear.
                        is_ear = 0;
                        break;
                    }
                }
            } else if (orientation < 0.f) {
                continue;
            } else {
                assert(!"Three sequentional points are collinear. This is not handled atm.");
            }

            if (is_ear) {
                left_of_tip = node_lft;
                goto ear_found;
            }
        }
        assert(!"Couldn't find an ear. Sad.");

ear_found:
        Index_Node *tip = left_of_tip->next;
        int l = left_of_tip->idx;
        int r = tip->next->idx;
        Vertex *vert_tip = verts[tip->idx].vert;
        Vertex *vert_lft = verts[l].vert;
        Vertex *vert_rgt = verts[r].vert;

        Quad_Edge *right_edge_in_face = 0;
        Quad_Edge *left_edge_in_face  = 0;
        for (Quad_Edge *e : vert_tip->edges) {
            if (dst(e) == vert_lft) {  left_edge_in_face = sym(e); }
            if (dst(e) == vert_rgt) { right_edge_in_face = e; }
        }
        cdt_assert(left_edge_in_face);
        cdt_assert(right_edge_in_face);
        Quad_Edge *new_edge = connect(ctx, right_edge_in_face, left_edge_in_face);
        new_edges.push(new_edge);

        // clip the ear triangle.
        if (node_first == tip) {
            node_first = node_first->next;
        }
        left_of_tip->next = tip->next;
        --num;
    }

    flip_until_stack_is_empty(new_edges);

    free(nodes);
}

// @Robustness:
int cdt_vert_ccw_cmp(const void *vert1, const void *vert2) {
    Vertex_Sort_Struct *va = (Vertex_Sort_Struct *)vert1;
    Vertex_Sort_Struct *vb = (Vertex_Sort_Struct *)vert2;
    Vec2 a = va->vert->pos;
    Vec2 b = vb->vert->pos;
    float32 adx = va->dx;
    float32 ady = va->dy;
    float32 bdx = vb->dx;
    float32 bdy = vb->dy;

    if (adx >= 0.f && bdx  < 0.f) { return -1; }
    if (adx  < 0.f && bdx >= 0.f) { return 11; }
    if (adx == 0.f && bdx == 0.f) {
        if (ady >= 0.f || bdy >= 0.f) {
            if (a.y > b.y) { return -1; }
            return  1;
        }
        if (b.y > a.y) { return -1; }
        return  1;
    }

    float32 det = adx*bdy - bdx*ady;
    if (det < 0.f) { return 1; }
    return -1;
}

void destroy_vertex(Context *ctx, Vertex *vert) {
    std::vector<Quad_Edge *> edges_to_destroy;
    int num_edges = 0;
    for (Quad_Edge *e : vert->edges) {
        edges_to_destroy.push_back(e);
        num_edges += 1;
    }

    std::vector<Vertex_Sort_Struct> verts(num_edges);
    for (int i = 0; i < num_edges; i += 1) {
        Quad_Edge *e = edges_to_destroy[i];
        verts[i].vert = dst(e);
        verts[i].dx   = dst(e)->pos.x - vert->pos.x; 
        verts[i].dy   = dst(e)->pos.y - vert->pos.y; 
        destroy_edge(ctx, e);
    }

    // Retriangulate the hole created from vertex removal.
    qsort(verts.data(), verts.size(), sizeof(verts[0]), cdt_vert_ccw_cmp);
    cdt_ear_triangulate_simple_polygon(ctx, (int)verts.size(), verts.data());

    delete vert;
}

// Actual heavy liftings.
//
void cdt_init(Context *ctx, float32 x1, float32 y1, float32 x2, float32 y2, float32 x3, float32 y3) {
    // @Todo: Support CW?
    Vec2 a = Vec2{x1, y1};
    Vec2 b = Vec2{x2, y2};
    Vec2 c = Vec2{x3, y3};

    Vertex *va = create_vertex(ctx, a);
    Vertex *vb = create_vertex(ctx, b);
    Vertex *vc = create_vertex(ctx, c);

    Quad_Edge *ab = create_edge(ctx, va, vb);
    Quad_Edge *bc = create_edge(ctx, vb, vc);
    Quad_Edge *ca = create_edge(ctx, vc, va);

    splice(sym(ab), bc);
    splice(sym(bc), ca);
    splice(sym(ca), ab);
}

Locate_Result locate_point(Context *ctx, Vec2 target) {
    // @Todo: Better strategy for locating the point. 'Jump and Walk' is one 
    //        way to go. Should I implement spatial partitioning in here?
    //
    //        Think harder whether merging two sufficiently close vertices could 
    //        be a remedy for FP issues.
    //
    //        If the "target" is close enough to an edge, we might want to project 
    //        it onto the edge.
    //
    Locate_Result result = {0};
    Quad_Edge *begin_edge = &ctx->edges.front()->e[0];

    // @Temporary:
    std::vector<Quad_Edge *> visited_edges;

    for (Quad_Edge *base = begin_edge, *next = 0;; base = next) {
        // Check whether the "target" lies on one of the current triangle's 
        // vertices or one of its edges.
        //
        for (Quad_Edge *e = base;;) {
            if ((e->org->pos.x == target.x) && (e->org->pos.y == target.y)) {
                result.exists  = 1;
                result.vertex  = e->org;
                goto search_cleanup;
            } else if ((dst(e)->pos.x == target.x) && (dst(e)->pos.y == target.y)) {
                result.exists  = 1;
                result.vertex  = dst(e);
                goto search_cleanup;
            } else if (on_line(target, e->org->pos, dst(e)->pos)) {
                result.on_edge = 1;
                result.edge = e;
                goto search_cleanup;
            }
            e = lnext(e);
            if (e == base) { break; }
        }

        // Oriented walk.
        Quad_Edge *side = base;
        do {
            next = sym(side);
            if (!next->visited && left_of(target, next)) {
                goto next_edge_found;
            }
            side = lnext(side);
        } while (side != base);

        // @Todo: Epsilon-based testing? I'm just assuming the vertex is in the 
        //        face if I've reached here.
        result.edge = base;
        goto search_cleanup;

next_edge_found: // @Cleanup
        next->visited = 1;
        visited_edges.push_back(next);
    }

search_cleanup:
    // Clear visited mark.
    for (Quad_Edge *e : visited_edges) {
        e->visited = 0;
    }

    return result;
}

Vertex *insert_point(Context *ctx, Vec2 pos) {
    Locate_Result locate = locate_point(ctx, pos); // @Todo: I should just inline it.

    if (locate.exists) {
        return locate.vertex;
    }

    Vertex *new_vertex = create_vertex(ctx, pos);

    Quad_Edge *start_side = locate.edge;
    if (locate.on_edge) {
        // @Todo: According to Kallmann, I have to project the point to the edge, 
        //        but I assume it is for the epsilon-tested on-edge case. Currently, I am 
        //        defining 'point-on-edge' as a case if the determinant is 0. So, 
        //        I don't have to bother atm?
        start_side = oprev(start_side);
        destroy_edge(ctx, onext(start_side));
    }

    // Subdivide quadrilateral/triangle.
    {
        Quad_Edge *diagonal = create_edge(ctx, start_side->org, new_vertex);
        splice(start_side, diagonal);
        Quad_Edge *start = diagonal;
        Quad_Edge *side = start_side;
        do {
            diagonal = connect(ctx, side, sym(diagonal));
            side = oprev(diagonal);
        } while (lnext(side) != start);
    }

    {
        // Push initial potentially flippable edges to stack.
        std::stack<Quad_Edge *> check_stack;
        Quad_Edge *side = start_side;
        do {
            check_stack.push(side);
            side = oprev(lnext(side));
        } while (side != start_side);

        flip_until_stack_is_empty(check_stack);
    }


    return new_vertex;
}

void insert_segment(Context *ctx, Id id, Vertex *vert1, Vertex *vert2) {
    Vec2 p = vert1->pos;
    Vec2 q = vert2->pos;

    // Gather intersecting edges.
    //
    // -----==========---------- : Intersecting X
    //
    //          \
    //           \
    // -----------o------------- : Intersecting X
    //
    //
    //             \
    // -------------\----------- : Intersecting O
    //               \
    //        
    // Why? We don't actually "create" the constrained edges. That is, we don't 
    // actually allocate memory for the constrained edge. We simply flip existing 
    // edges that intersect with this "virtual" constrained edge. When the flipping 
    // routine is over, the resulting sequence of edges will form the actual 
    // constrained edge.
    //
    // By the theorem derived from Euler's formula,
    //
    //                  V - E + F = 2
    //
    // the number of edges in a triangulation remains unchanged. Therefore, we 
    // don't need to create new edges-the subdivision is already fullly triangulated.
    //


    std::queue<Quad_Edge *> intersectings;

    Vertex *pivot = vert1;
    Quad_Edge *r = pivot->edges.front();
    Quad_Edge *l = onext(r);
    while (pivot != vert2) {
        if (orientation(dst(r)->pos, p,q) < 0.f) { // right
            while (orientation(dst(l)->pos, p,q) < 0.f) { // right
                r = l;
                l = onext(l);
            }
        } else { // left or on-line
            do {
                l = r;
                r = oprev(r);
            } while (orientation(dst(r)->pos, p,q) >= 0.f);
        }

        if (orientation(dst(l)->pos, p,q) == 0.f) { // l is on-line
            l = sym(dnext(l));
            r = lprev(r);
            pivot = l->org;
        } else {
            for (;;) {
                intersectings.push(lnext(r));
                Vertex *s = dnext(dnext(l))->org;
                if (orientation(s->pos, p,q) < 0.f) { // right
                    l = dnext(l);
                    r = oprev(l);
                } else if (orientation(s->pos, p,q) > 0.f) { // left
                    r = dprev(r);
                    l = onext(r);
                } else { // on-line
                    r = dnext(dnext(l));
                    l = onext(r);
                    pivot = s;
                    break;
                }
            }
        }
    }



    // The input consists of the collected intersecting edges.
    // Using this input, we will perform the required edge flips.
    //
    std::stack<Quad_Edge *> flip_stack;
    while (!intersectings.empty()) {
        Quad_Edge *e = intersectings.front();
        intersectings.pop();

        // @Fix: Infinite loop + Strictness
        Vec2 a = e->org->pos;
        Vec2 b = dst(oprev(e))->pos;
        Vec2 c = dst(e)->pos;
        Vec2 d = lprev(e)->org->pos;

        // @Temporary
        if (constrained(e)) {
            cdt_assert(!"I think you have intersecting constrained edgse. It is not covered atm. Sorry!");
        }

        if (is_convex(a, b, c, d)) {
            swap(e);
            if (orientation(e->org->pos, p,q)*orientation(dst(e)->pos, p,q) < 0.f) {
                intersectings.push(e);
            } else {
                flip_stack.push(e);
            }
        } else {
            intersectings.push(e);
        }
    }

    // Assign an ID to the edges that lie on the constrained edge by walking 
    // from the start vetex to the end vertex.
    for (Vertex *cur = vert1, *nxt = 0;;cur = nxt) {
        if (cur == vert2) { break; }

        for (Quad_Edge *e : cur->edges) {
            nxt = dst(e);
            if (orientation(nxt->pos, p,q) == 0.f) {
                float32 x1 = q.x - cur->pos.x;
                float32 y1 = q.y - cur->pos.y;
                float32 x2 = nxt->pos.x - cur->pos.x;
                float32 y2 = nxt->pos.y - cur->pos.y;
                if (x1*x2 + y1*y2 > 0.f) {
                    get_edge(e)->ids.push_back(id);
                    break;
                }
            }
        }
    }

    // Now that all relevant edges have been marked as constrained, it's safe to 
    // perform edge flips, since constrained edges will be skipped.
    //
    flip_until_stack_is_empty(flip_stack);
}

void cdt_insert(Context *ctx, Id id, float32 x1, float32 y1, float32 x2, float32 y2) {
    Vec2 p1;
    p1.x = x1;
    p1.y = y1;
    Vertex *vtx1 = insert_point(ctx, p1);

    Vec2 p2;
    p2.x = x2;
    p2.y = y2;
    Vertex *vtx2 = insert_point(ctx, p2);

    insert_segment(ctx, id, vtx1, vtx2);
}

void cdt_remove(Context *ctx, Id id) {
    std::set<Vertex *> vertices;

    for (Edge *e : ctx->edges) {
        if (find(e->ids.begin(), e->ids.end(), id) != e->ids.end()) {
            e->ids.remove(id);
            vertices.insert(e->e[0].org);
            vertices.insert(e->e[2].org);
        }
    }

    for (Vertex *v : vertices) {
        int should_destroy = 1;
        for (Quad_Edge *e : v->edges) {
            if (constrained(e)) {
                should_destroy = 0;
                break;
            }
        }
        if (should_destroy) { destroy_vertex(ctx, v); }

    }
}


#endif // SW_CDT_H_INCLUDE
