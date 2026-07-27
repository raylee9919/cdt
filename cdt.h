/* 
   cdt - v0.22 - Dynamic 2D Constrained Delaunay Triangulation Library
   Seong Woo Lee 2025


   LICENSE
       See end of file for license information.


   NOTES 
       Primarily of interest to game developers.


   LIMITATION
       Constrained edges cannot intersect each other; treat each constraint
       as a solid obstacle.


   USAGE
       #define CDT_IMPLEMENTATION in one source file before including the header.

       First, initialize your 'cdt_context'.

           void cdt_init(cdt_context *ctx,
                         cdt_float x1, cdt_float y1,
                         cdt_float x2, cdt_float y2,
                         cdt_float x3, cdt_float y3)
       
       Provide a pointer to the context along with the three vertices of the 
       super-triangle in counter-clockwise order. Make sure the super-triangle 
       fully encloses all points that will be added later.



       Then, you can insert constraints using the following call:

           void cdt_insert(cdt_context *ctx,
                           cdt_id id,
                           cdt_float x1, cdt_float y1, cdt_float x2, cdt_float y2)
       
       Pass in the initialized context with the ID you wish to assign to the 
       constraint. Then provide the coordinates of the two endpoints that define 
       the constrained edge.



       If you need to remove a constraint later, simply call the following 
       function and provide the ID of the constraint you want to remove.

           void cdt_remove(cdt_context *ctx, cdt_id id)
       
*/

// @Todo: 
// - Optimization
//      Use arena/pool and remove ptr to ptr.
// - Traversing triangles is painful with the current quad-edge approach. I 
//   have to either explore a triangle-based approach or retain triangles, and 
//   the latter seems quite challenging.

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef float         cdt_float;
typedef int64_t       cdt_id;

#if !defined(CDT_NO_ASSERT)
#  ifdef _WIN32
#    define cdt_assert(exp) if (!(exp)) { __debugbreak(); }
#  else
#    define cdt_assert(exp) if (!(exp)) {*(volatile int*)0=0;}
#  endif
#endif

#if !defined(CDT_MY_ALLOCATOR)
#  define cdt_malloc    malloc
#  define cdt_free      free
#endif

#ifdef CDT_STATIC
#  define CDTDEF static
#else
#  define CDTDEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cdt_vertex       cdt_vertex;
typedef struct cdt_quad_edge    cdt_quad_edge;
typedef struct cdt_edge         cdt_edge;

typedef struct {
    cdt_float x, y;
} cdt_vec2;

typedef struct {
    cdt_vertex **data;
    int num;
    int cap;
} cdt_vertex_array;

typedef struct {
    cdt_id *data;
    int num;
    int cap;
} cdt_id_array;

typedef struct {
    cdt_quad_edge **data;
    int num;
    int cap;
} cdt_quad_edge_array;

typedef struct {
    cdt_edge **data;
    int num;
    int cap;
} cdt_edge_array;

typedef struct {
    cdt_quad_edge **data;
    int front;
    int back;
    int cap;
} cdt_queue;

struct cdt_vertex {
    cdt_vec2 pos;
    cdt_quad_edge_array edges; // A list of quad-edges originating from the vertex.
};

struct cdt_quad_edge {
    cdt_vertex    *org;
    cdt_quad_edge *onext_ptr;
    uint8_t        idx;       // [0,3]
    uint8_t        visited;
};

struct cdt_edge {
    cdt_quad_edge e[4];
    cdt_id_array  ids;
};

typedef struct {
    cdt_vertex_array vertices;
    cdt_edge_array   edges;
} cdt_context;

typedef struct {
    int is_exact_vertex;
    int is_on_edge;
    cdt_vertex *vertex;
    cdt_quad_edge *edge;
} cdt_locate_result;

typedef struct {
    cdt_vertex *vert;
    cdt_float dx;
    cdt_float dy;
} cdt_vertex_sort_struct;

typedef struct cdt_index_node cdt_index_node;
struct cdt_index_node {
    int idx;
    cdt_index_node *next;
};

typedef struct {
    cdt_quad_edge *edges[3];
    cdt_float x[3];
    cdt_float y[3];
} cdt_triangle;

typedef struct {
    cdt_triangle triangles[3];
} cdt_triangles;


// APIs
//
CDTDEF void           cdt_init(cdt_context *ctx, cdt_float x1, cdt_float y1, cdt_float x2, cdt_float y2, cdt_float x3, cdt_float y3);

CDTDEF void           cdt_insert(cdt_context *ctx, cdt_id id, cdt_float x1, cdt_float y1, cdt_float x2, cdt_float y2);
CDTDEF void           cdt_remove(cdt_context *ctx, cdt_id id);

CDTDEF int            cdt_is_constrained(cdt_edge *edge);
CDTDEF int            cdt_is_quad_edge_constrained(cdt_quad_edge *edge);

CDTDEF int            cdt_get_vertex_count(cdt_context *ctx);
CDTDEF int            cdt_get_edge_count(cdt_context *ctx);
CDTDEF int            cdt_get_triangle_count(cdt_context *ctx);

CDTDEF void           cdt_get_all_triangles(cdt_context *ctx, cdt_triangle *out_triangles);
CDTDEF cdt_triangle   cdt_get_triangle_containing_point(cdt_context *ctx, cdt_float x, cdt_float y);
CDTDEF cdt_triangles  cdt_get_adjacent_triangles(cdt_triangle triangle);
CDTDEF cdt_quad_edge *cdt_get_portal_edge(cdt_triangle src, cdt_triangle dst);

// Quad-Edge Operators
CDTDEF cdt_edge      *cdt_get_edge(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_rot(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_inv_rot(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_sym(cdt_quad_edge *e);
CDTDEF cdt_vertex    *cdt_dst(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_onext(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_oprev(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_lnext(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_lprev(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_dnext(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_dprev(cdt_quad_edge *e);
CDTDEF cdt_quad_edge *cdt_rprev(cdt_quad_edge *e);


#ifdef __cplusplus
}
#endif



#ifdef CDT_IMPLEMENTATION


//void cdt_log(cdt_context *ctx) {
//    FILE *file = fopen("subdivision.log", "wb");
//    if (file) {
//        fprintf(file, "\n");
//        fprintf(file, "segments = [\n");
//        for (int i = 0; i < ctx->edges.num; i+=1) {
//            cdt_edge *e = ctx->edges.data[i];
//            fprintf(file, "    ((%.2f, %.2f), (%.2f, %.2f)),\n", e->e[0].org->pos.x, e->e[0].org->pos.y, e->e[2].org->pos.x, e->e[2].org->pos.y);
//        }
//        fprintf(file, "]\n");
//        fclose(file);
//    }
//}


// Helper data structure codes
//
void cdt_vertex_array_push(cdt_vertex_array *arr, cdt_vertex *item) {
    if (arr->cap == 0) {
        arr->cap = 16;
        arr->data = (cdt_vertex **)cdt_malloc(sizeof(item)*arr->cap);
    } else if (arr->num >= arr->cap) {
        arr->cap <<= 1;
        arr->data = (cdt_vertex **)realloc(arr->data, sizeof(item)*arr->cap);
    }

    arr->data[arr->num] = item;
    arr->num += 1;
}

void cdt_vertex_array_pop(cdt_vertex_array *arr, cdt_vertex *item) {
    for (int i = 0; i < arr->num; i += 1) {
        if (arr->data[i] == item) {
            arr->num -= 1;
            arr->data[i] = arr->data[arr->num];
            return;
        }
    }
}

void cdt_quad_edge_array_push(cdt_quad_edge_array *arr, cdt_quad_edge *item) {
    if (arr->cap == 0) {
        arr->cap = 16;
        arr->data = (cdt_quad_edge **)cdt_malloc(sizeof(item)*arr->cap);
    } else if (arr->num >= arr->cap) {
        arr->cap <<= 1;
        arr->data = (cdt_quad_edge **)realloc(arr->data, sizeof(item)*arr->cap);
    }

    arr->data[arr->num] = item;
    arr->num += 1;
}

void cdt_quad_edge_array_pop(cdt_quad_edge_array *arr, cdt_quad_edge *item) {
    for (int i = 0; i < arr->num; i += 1) {
        if (arr->data[i] == item) {
            arr->num -= 1;
            arr->data[i] = arr->data[arr->num];
            return;
        }
    }
}

cdt_quad_edge *cdt_stack_pop(cdt_quad_edge_array *arr) {
    cdt_quad_edge *result = arr->data[arr->num-1];
    arr->num -= 1;
    return result;
}

void cdt_edge_array_push(cdt_edge_array *arr, cdt_edge *item) {
    if (arr->cap == 0) {
        arr->cap = 16;
        arr->data = (cdt_edge **)cdt_malloc(sizeof(item)*arr->cap);
    } else if (arr->num >= arr->cap) {
        arr->cap <<= 1;
        arr->data = (cdt_edge **)realloc(arr->data, sizeof(item)*arr->cap);
    }

    arr->data[arr->num] = item;
    arr->num += 1;
}

void cdt_edge_array_pop(cdt_edge_array *arr, cdt_edge *item) {
    for (int i = 0; i < arr->num; i += 1) {
        if (arr->data[i] == item) {
            arr->num -= 1;
            arr->data[i] = arr->data[arr->num];
            return;
        }
    }
}

void cdt_id_array_push(cdt_id_array *arr, cdt_id item) {
    if (arr->cap == 0) {
        arr->cap = 16;
        arr->data = (cdt_id *)cdt_malloc(sizeof(item)*arr->cap);
    } else if (arr->num >= arr->cap) {
        arr->cap <<= 1;
        arr->data = (cdt_id *)realloc(arr->data, sizeof(item)*arr->cap);
    }

    arr->data[arr->num] = item;
    arr->num += 1;
}

void cdt_queue_push(cdt_queue *q, cdt_quad_edge *e) {
    if (q->cap == 0) {
        q->cap = 32;
        q->data = (cdt_quad_edge **)cdt_malloc(sizeof(cdt_quad_edge *)*q->cap);
    }

    if ((q->back+1)%q->cap == q->front) {
        int new_cap = (q->cap << 1);
        cdt_quad_edge **ptr = (cdt_quad_edge **)cdt_malloc(sizeof(cdt_quad_edge *)*new_cap);
        for (int j = 0, i = q->front; i != q->back; j+=1, i = ((i+1)%q->cap)) {
            ptr[j] = q->data[i];
        }

        q->front = 0;
        q->back  = q->cap;
        q->cap   = new_cap;

        cdt_free(q->data);
        q->data = ptr;
    }

    q->data[q->back] = e;
    q->back = (q->back+1)%q->cap;
}

cdt_quad_edge *cdt_queue_pop(cdt_queue *q) {
    cdt_quad_edge *result = q->data[q->front];
    q->front = ((q->front+1)%q->cap);
    return result;
}

int cdt_queue_empty(cdt_queue *q) {
    return q->front == q->back;
}



// Quad Edge Algebra
// 
cdt_edge *cdt_get_edge(cdt_quad_edge *e) {
    return (cdt_edge *)(e - e->idx);
}

cdt_quad_edge *cdt_rot(cdt_quad_edge *e) {
    int off[4] = {1,2,3,0};
    return e - e->idx + off[e->idx];
}

cdt_quad_edge *cdt_inv_rot(cdt_quad_edge *e) {
    int off[4] = {3,0,1,2};
    return e - e->idx + off[e->idx];
}

cdt_quad_edge *cdt_sym(cdt_quad_edge *e) {
    int off[4] = {2,3,0,1};
    return e - e->idx + off[e->idx];
}

cdt_vertex *cdt_dst(cdt_quad_edge *e) {
    return cdt_sym(e)->org;
}

cdt_quad_edge *cdt_onext(cdt_quad_edge *e) {
    return e->onext_ptr;
}

cdt_quad_edge *cdt_oprev(cdt_quad_edge *e) {
    return cdt_rot(cdt_onext(cdt_rot(e)));
}

cdt_quad_edge *cdt_lnext(cdt_quad_edge *e) {
    return cdt_rot(cdt_onext(cdt_inv_rot(e)));
}

cdt_quad_edge *cdt_lprev(cdt_quad_edge *e) {
    return cdt_sym(cdt_onext(e));
}

cdt_quad_edge *cdt_dnext(cdt_quad_edge *e) {
    return cdt_sym(cdt_onext(cdt_sym(e)));
}

cdt_quad_edge *cdt_dprev(cdt_quad_edge *e) {
    return cdt_inv_rot(cdt_onext(cdt_inv_rot(e)));
}

cdt_quad_edge *cdt_rprev(cdt_quad_edge *e) {
    return cdt_onext(cdt_sym(e));
}

void cdt_splice(cdt_quad_edge *a, cdt_quad_edge *b) {
    cdt_quad_edge *alpha = cdt_rot(cdt_onext(a));
    cdt_quad_edge *beta  = cdt_rot(cdt_onext(b));

    cdt_quad_edge *b_onext = cdt_onext(b);
    b->onext_ptr = cdt_onext(a);
    a->onext_ptr = b_onext;

    cdt_quad_edge *alpha_onext = cdt_onext(alpha);
    alpha->onext_ptr = cdt_onext(beta);
    beta->onext_ptr  = alpha_onext;
}

void cdt_swap(cdt_quad_edge *e) {
    // Remove original reference from the vertices.
    cdt_quad_edge_array_pop(&e->org->edges, e);
    cdt_quad_edge_array_pop(&cdt_dst(e)->edges, cdt_sym(e));
 
    cdt_quad_edge *a = cdt_oprev(e);
    cdt_quad_edge *b = cdt_oprev(cdt_sym(e));
    cdt_splice(e, a);
    cdt_splice(cdt_sym(e), b);
    cdt_splice(e, cdt_lnext(a));
    cdt_splice(cdt_sym(e), cdt_lnext(b));

    e->org = cdt_dst(a);
    cdt_sym(e)->org = cdt_dst(b);

    // Add new references.
    cdt_quad_edge_array_push(&e->org->edges, e);
    cdt_quad_edge_array_push(&cdt_dst(e)->edges, cdt_sym(e));
}

cdt_quad_edge *cdt_create_edge(cdt_context *ctx, cdt_vertex *org, cdt_vertex *dst) {
    cdt_edge *edge = (cdt_edge *)cdt_malloc(sizeof(cdt_edge));
    memset(edge, 0, sizeof(cdt_edge));

    cdt_edge_array_push(&ctx->edges, edge);

    edge->e[0].idx = 0;
    edge->e[1].idx = 1;
    edge->e[2].idx = 2;
    edge->e[3].idx = 3;

    // Primary
    edge->e[0].onext_ptr = &(edge->e[0]);
    edge->e[2].onext_ptr = &(edge->e[2]);
    edge->e[0].org = org;
    edge->e[2].org = dst;

    // Dual
    edge->e[1].onext_ptr = &(edge->e[3]);
    edge->e[3].onext_ptr = &(edge->e[1]);

    // Add reference to the two endpoint vertices.
    cdt_quad_edge_array_push(&org->edges, &edge->e[0]);
    cdt_quad_edge_array_push(&dst->edges, &edge->e[2]);

    return edge->e;
}

void cdt_destroy_edge(cdt_context *ctx, cdt_quad_edge *e) {
    // Remove reference from the two endpoint vertices.
    cdt_quad_edge_array_pop(&e->org->edges, e);
    cdt_quad_edge_array_pop(&cdt_dst(e)->edges, cdt_sym(e));

    cdt_splice(e, cdt_oprev(e));
    cdt_splice(cdt_sym(e), cdt_oprev(cdt_sym(e)));

    cdt_edge *edge = cdt_get_edge(e);
    cdt_edge_array_pop(&ctx->edges, edge);

    cdt_free(edge);
}

cdt_quad_edge *cdt_connect(cdt_context *ctx, cdt_quad_edge *a, cdt_quad_edge *b) {
    cdt_quad_edge *e = cdt_create_edge(ctx, cdt_dst(a), b->org);
    cdt_splice(e, cdt_lnext(a));
    cdt_splice(cdt_sym(e), b);
    return e;
}

// Geometry/Math
//
cdt_float cdt_orientation(cdt_vec2 a, cdt_vec2 b, cdt_vec2 c) {
    // Returns twice the area of the triangle. Signed if the orientation is clockwise.
    // @Todo: SIMD?
    cdt_float x1 = b.x - a.x;
    cdt_float y1 = b.y - a.y;
    cdt_float x2 = c.x - a.x;
    cdt_float y2 = c.y - a.y;
    return (x1*y2 - x2*y1);
}

int cdt_right_of(cdt_vec2 p, cdt_quad_edge *e) {
    return cdt_orientation(e->org->pos, cdt_dst(e)->pos, p) < 0.f;
}

int cdt_left_of(cdt_vec2 p, cdt_quad_edge *e) {
    return cdt_orientation(e->org->pos, cdt_dst(e)->pos, p) > 0.f;
}

int cdt_on_line(cdt_vec2 p, cdt_vec2 org, cdt_vec2 dst) {
    return cdt_orientation(org, dst, p) == 0.f;
}

int cdt_in_circumcircle(cdt_vec2 p, cdt_vec2 a, cdt_vec2 b, cdt_vec2 c) {
    // @Todo: Better math?
    //        "It is worth to mention that we have faced never-ending loops during 
    //         the flipping process when using only a simple determinant evaluation." -Kallmann
    //
    //        Shewchuck's precise math predicate seems solid.
    //
    cdt_float ax = a.x; cdt_float ay = a.y;
    cdt_float bx = b.x; cdt_float by = b.y;
    cdt_float cx = c.x; cdt_float cy = c.y;
    cdt_float px = p.x; cdt_float py = p.y;
    cdt_float ax_ = ax-px;
    cdt_float ay_ = ay-py;
    cdt_float bx_ = bx-px;
    cdt_float by_ = by-py;
    cdt_float cx_ = cx-px;
    cdt_float cy_ = cy-py;
    return ((ax_*ax_ + ay_*ay_) * (bx_*cy_-cx_*by_) -
            (bx_*bx_ + by_*by_) * (ax_*cy_-cx_*ay_) +
            (cx_*cx_ + cy_*cy_) * (ax_*by_-bx_*ay_)) > 0;
}

int cdt_is_convex(cdt_vec2 a, cdt_vec2 b, cdt_vec2 c, cdt_vec2 d) {
    // @Todo: SIMD?
    cdt_float x1 = b.x - a.x;
    cdt_float y1 = b.y - a.y;
    cdt_float x2 = c.x - b.x;
    cdt_float y2 = c.y - b.y;
    cdt_float x3 = d.x - c.x;
    cdt_float y3 = d.y - c.y;
    cdt_float x4 = a.x - d.x;
    cdt_float y4 = a.y - d.y;
    cdt_float c1 = x1*y2 - x2*y1;
    cdt_float c2 = x2*y3 - x3*y2;
    cdt_float c3 = x3*y4 - x4*y3;
    cdt_float c4 = x4*y1 - x1*y4;

    return (c1*c2 > 0.f) && (c2*c3 > 0.f) && (c3*c4 > 0.f);
}

// This is for ear-clipping.
int cdt_in_triangle(cdt_vec2 p, cdt_vec2 a, cdt_vec2 b, cdt_vec2 c) {
    // @Todo: SIMD?
    cdt_vec2 ab = {b.x - a.x, b.y - a.y};
    cdt_vec2 bc = {c.x - b.x, c.y - b.y};
    cdt_vec2 ca = {a.x - c.x, a.y - c.y};
    cdt_vec2 ap = {p.x - a.x, p.y - a.y};
    cdt_vec2 bp = {p.x - b.x, p.y - b.y};
    cdt_vec2 cp = {p.x - c.x, p.y - c.y};
    cdt_float oa = ab.x*ap.y - ab.y*ap.x;
    cdt_float ob = bc.x*bp.y - bc.y*bp.x;
    cdt_float oc = ca.x*cp.y - ca.y*cp.x;
    // If the point is on the edge of a triangle, we can clip that ear triangle.
    return oa >= 0.f && ob >= 0.f && oc >= 0.f; 
}

cdt_vertex *cdt_create_vertex(cdt_context *ctx, cdt_vec2 pos) {
    cdt_vertex *result = (cdt_vertex *)cdt_malloc(sizeof(cdt_vertex));
    memset(result, 0, sizeof(cdt_vertex));
    result->pos = pos;
    cdt_vertex_array_push(&ctx->vertices, result);
    return result;
}

// @Note: To exactly match real-world mathematics and avoid infinite loops,
//        robust predicates (e.g., Shewchuk’s) are required. However, my goal
//        here is a "good enough" triangle, and I want to avoid heavy numerical
//        computations. For now, I simply stop pushing edges onto the stack
//        once they have already been visited.
//
void cdt_flip_until_stack_is_empty(cdt_quad_edge_array *stk) {
    cdt_quad_edge_array visited_array = {0};

    // Flip
    while (stk->num != 0) {
        cdt_quad_edge *e = cdt_stack_pop(stk);

        if (e->visited) { continue; }
        e->visited = 1;
        cdt_quad_edge_array_push(&visited_array, e);

        // @Robustness
        if (!cdt_is_constrained(cdt_get_edge(e))) {
            if (cdt_in_circumcircle(cdt_dprev(e)->org->pos, cdt_dst(e)->pos, e->org->pos, cdt_dnext(e)->org->pos)) {
                cdt_quad_edge *e1 = cdt_lnext(e);
                cdt_quad_edge *e2 = cdt_dnext(e);
                if (!e1->visited) { cdt_quad_edge_array_push(stk, e1); }
                if (!e2->visited) { cdt_quad_edge_array_push(stk, e2); }
                cdt_swap(e);
            }
        }
    }

    // Cleanup
    for (int i = 0; i < visited_array.num; i+=1) {
        visited_array.data[i]->visited = 0;
    }
    cdt_free(visited_array.data);
}

void cdt_ear_triangulate_simple_polygon(cdt_context *ctx, int num_verts, cdt_vertex_sort_struct *verts) {
    cdt_quad_edge_array new_edges = {0};

    // Preprocessing..
    int num = num_verts;
    cdt_index_node *nodes = (cdt_index_node *)cdt_malloc(sizeof(cdt_index_node)*num);
    for (int i = 0; i < num; ++i) {
        nodes[i].next = &nodes[(i+1)%num];
        nodes[i].idx  = i;
    }
    cdt_index_node *node_first = &nodes[0];

    while (num > 3) {
        cdt_index_node *cdt_left_of_tip = 0;
        for (int ii = 0; ii < num; ++ii) {
            cdt_index_node *node_lft = node_first;
            for (int i = 0; i < ii; ++i) { node_lft = node_lft->next; }
            cdt_index_node *node_tip = node_lft->next;
            cdt_index_node *node_rgt = node_tip->next;

            int idx0 = node_lft->idx;
            int idx1 = node_tip->idx;
            int idx2 = node_rgt->idx;

            cdt_vec2 p[3] = { verts[idx0].vert->pos, verts[idx1].vert->pos, verts[idx2].vert->pos };
            cdt_vec2 e[2] = { 
                { p[1].x - p[0].x, p[1].y - p[0].y },
                { p[2].x - p[0].x, p[2].y - p[0].y },
            };

            int is_ear = 1;
            cdt_float orientation = e[0].x*e[1].y - e[0].y*e[1].x;
            if (orientation > 0.f) {
                for (cdt_index_node *node = node_rgt->next; node != node_lft; node = node->next) {
                    cdt_vec2 v = verts[node->idx].vert->pos;
                    if (cdt_in_triangle(v, p[0], p[1], p[2])) { // This isn't an ear.
                        is_ear = 0;
                        break;
                    }
                }
            } else {
                continue; // @Note: Am I just ignoring collinear points?
            }

            if (is_ear) {
                cdt_left_of_tip = node_lft;
                goto ear_found;
            }
        }
        cdt_assert(!"Couldn't find an ear. Sad.");

ear_found:
        cdt_index_node *tip = cdt_left_of_tip->next;
        int l = cdt_left_of_tip->idx;
        int r = tip->next->idx;
        cdt_vertex *vert_tip = verts[tip->idx].vert;
        cdt_vertex *vert_lft = verts[l].vert;
        cdt_vertex *vert_rgt = verts[r].vert;

        cdt_quad_edge *right_edge_in_face = 0;
        cdt_quad_edge *left_edge_in_face  = 0;

        for (int i = 0; i < vert_tip->edges.num; i+=1) {
            cdt_quad_edge *e = vert_tip->edges.data[i];
            if (cdt_dst(e) == vert_lft) {  left_edge_in_face = cdt_sym(e); }
            if (cdt_dst(e) == vert_rgt) { right_edge_in_face = e; }
        }

        cdt_assert(left_edge_in_face);
        cdt_assert(right_edge_in_face);
        cdt_quad_edge *edge1 = cdt_connect(ctx, right_edge_in_face, left_edge_in_face);
        cdt_quad_edge *edge2 = cdt_sym(edge1);
        cdt_quad_edge_array_push(&new_edges, edge1);
        cdt_quad_edge_array_push(&new_edges, edge2);

        // clip the ear triangle.
        if (node_first == tip) {
            node_first = node_first->next;
        }
        cdt_left_of_tip->next = tip->next;
        --num;
    }

    cdt_flip_until_stack_is_empty(&new_edges);

    cdt_free(nodes);
    cdt_free(new_edges.data);
}

// @Robustness:
int cdt_vert_ccw_cmp(const void *vert1, const void *vert2) {
    cdt_vertex_sort_struct *va = (cdt_vertex_sort_struct *)vert1;
    cdt_vertex_sort_struct *vb = (cdt_vertex_sort_struct *)vert2;
    cdt_float ax = va->dx;
    cdt_float ay = va->dy;
    cdt_float bx = vb->dx;
    cdt_float by = vb->dy;

    // @Todo: Is atan bad?
    cdt_float angle_a = atan2f(ay, ax);
    cdt_float angle_b = atan2f(by, bx);

    if (angle_a != angle_b) {
        return angle_a < angle_b ? -1 : 1;
    }

    cdt_float dist_a = ax * ax + ay * ay;
    cdt_float dist_b = bx * bx + by * by;

    if (dist_a != dist_b) {
        return dist_a < dist_b ? -1 : 1;
    }

    return 0;
}

void cdt_destroy_vertex(cdt_context *ctx, cdt_vertex *vert) {
    // Get list of edges to destroy.
    int num_edges = 0;
    for (int i = 0; i < vert->edges.num; i+=1) {
        num_edges += 1;
    }

    cdt_quad_edge **edges_to_destroy = (cdt_quad_edge **)cdt_malloc(sizeof(cdt_quad_edge *)*num_edges);
    for (int i = 0; i < vert->edges.num; i+=1) {
        edges_to_destroy[i] = vert->edges.data[i];
    }

    // Angular sort outline vertices ccw.
    cdt_vertex_sort_struct *outline = (cdt_vertex_sort_struct *)cdt_malloc(sizeof(cdt_vertex_sort_struct)*num_edges);
    for (int i = 0; i < num_edges; i += 1) {
        cdt_quad_edge *e = edges_to_destroy[i];
        outline[i].vert = cdt_dst(e);
        outline[i].dx   = cdt_dst(e)->pos.x - vert->pos.x; 
        outline[i].dy   = cdt_dst(e)->pos.y - vert->pos.y; 
        cdt_destroy_edge(ctx, e);
    }
    qsort(outline, num_edges, sizeof(outline[0]), cdt_vert_ccw_cmp);

    // Retriangulate the hole created from vertex removal.
    cdt_ear_triangulate_simple_polygon(ctx, num_edges, outline);

    // Cleanup
    cdt_vertex_array_pop(&ctx->vertices, vert);
    cdt_free(outline);
    cdt_free(edges_to_destroy);
    cdt_free(vert);
}

// 0. If the point is outside of the super-triangle,
//    @Todo: Not handled yet. Will cause infinite loop.
//
// 1. If the point is equivalent some vertex: 
//        '.is_exact_vertex' is set to 1
//        '.vertex' is set to the pointer to that vertex.
//        '.edge' is set to the pointer to the arbitrary quad-edge that contains the vertex.
// 
// 2. If not, and the point is on some edge:
//        '.is_on_edge' is set to 1
//        '.edge' is set to the arbitrary quad edge which is part of the found edge.
//
// 3. If not, and the point in some triangle:
//        '.is_on_edge' is set to 1
//        '.edge' is set to the arbitrary quad edge which is part of the found edge.
// 
// The important thing is that it always return some quad-edge, and you can 
// start from there.
//
cdt_locate_result cdt_locate_point(cdt_context *ctx, cdt_float x, cdt_float y) {
    // @Todo: Better strategy for locating the point. 'Jump and Walk' is one 
    //        way to go. Should I implement spatial partitioning in here?
    //
    //        Think harder whether merging two sufficiently close vertices could 
    //        be a remedy for FP issues.
    //
    //        If the "target" is close enough to an edge, we might want to project 
    //        it onto the edge.
    //
    cdt_locate_result result = {0};
    cdt_quad_edge *begin_edge = &ctx->edges.data[0]->e[0];

    cdt_vec2 target = {0};
    target.x = x;
    target.y = y;

    for (cdt_quad_edge *e1 = begin_edge;;) {
        cdt_quad_edge *e2 = cdt_lnext(e1);
        cdt_quad_edge *e3 = cdt_lnext(e2);
        cdt_vertex *v[3] = { e1->org, e2->org, e3->org };
        cdt_vec2 p[3] = { v[0]->pos, v[1]->pos, v[2]->pos };

        cdt_quad_edge *e[3] = { e1, e2, e3 };

        // If one of its points is identical to my target, end iteration.
        for (int i = 0; i < 3; i+=1) {
            if (p[i].x == target.x && p[i].y == target.y) {
                result.is_exact_vertex = 1;
                result.vertex = v[i];
                result.edge = v[i]->edges.data[0];
                return result;
            }
        }

        // In triangle?
        if (cdt_left_of(target,e1) && cdt_left_of(target,e2) && cdt_left_of(target,e3)) {
            result.edge = e1;
            return result;
        }

        // If not, find next edge to pass through.
        cdt_quad_edge *next = 0;
        for (int i = 0; i < 3; i+=1) {
            int j = (i+1)%3;
            if (cdt_left_of(cdt_dst(e[j])->pos, e[i]) && cdt_right_of(target, e[i])) {
                next = cdt_sym(e[i]);
                break;
            }
        }

        // If not found, the point is on an edge.
        if (next == 0) {
            for (int i = 0; i < 3; i+=1) {
                if (cdt_on_line(target, e[i]->org->pos, cdt_dst(e[i])->pos)) {
                    result.is_on_edge = 1;
                    result.edge = e[i];
                    return result;
                }
            }
        } else {
            e1 = next;
        }
    }
}

cdt_vertex *cdt_insert_point(cdt_context *ctx, cdt_vec2 pos) {
    cdt_locate_result locate = cdt_locate_point(ctx, pos.x, pos.y); // @Todo: I should just inline it.

    if (locate.is_exact_vertex) {
        return locate.vertex;
    }

    cdt_vertex *new_vertex = cdt_create_vertex(ctx, pos);

    cdt_quad_edge *start_side = locate.edge;
    if (locate.is_on_edge) {
        // @Todo: According to Kallmann, I have to project the point to the edge, 
        //        but I assume it is for the epsilon-tested on-edge case. Currently, I am 
        //        defining 'point-on-edge' as a case if the determinant is 0. So, 
        //        I don't have to bother atm?
        start_side = cdt_oprev(start_side);
        cdt_destroy_edge(ctx, cdt_onext(start_side));
    }

    // Subdivide quadrilateral/triangle.
    {
        cdt_quad_edge *diagonal = cdt_create_edge(ctx, start_side->org, new_vertex);
        cdt_splice(start_side, diagonal);
        cdt_quad_edge *start = diagonal;
        cdt_quad_edge *side = start_side;
        do {
            diagonal = cdt_connect(ctx, side, cdt_sym(diagonal));
            side = cdt_oprev(diagonal);
        } while (cdt_lnext(side) != start);
    }

    {
        // Push initial potentially flippable edges to stack.
        cdt_quad_edge_array check_stack = {0};
        cdt_quad_edge *side = start_side;
        do {
            cdt_quad_edge_array_push(&check_stack, side);
            side = cdt_oprev(cdt_lnext(side));
        } while (side != start_side);

        cdt_flip_until_stack_is_empty(&check_stack);
        cdt_free(check_stack.data);
    }


    return new_vertex;
}

void cdt_insert_segment(cdt_id id, cdt_vertex *vert1, cdt_vertex *vert2) {
    cdt_vec2 p = vert1->pos;
    cdt_vec2 q = vert2->pos;

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

    cdt_queue intersectings = {0};

    cdt_vertex *pivot = vert1;
    cdt_quad_edge *r = pivot->edges.data[0];
    cdt_quad_edge *l = cdt_onext(r);
    while (pivot != vert2) {
        if (cdt_orientation(cdt_dst(r)->pos, p,q) < 0.f) { // right
            while (cdt_orientation(cdt_dst(l)->pos, p,q) < 0.f) { // right
                r = l;
                l = cdt_onext(l);
            }
        } else { // left or on-line
            do {
                l = r;
                r = cdt_oprev(r);
            } while (cdt_orientation(cdt_dst(r)->pos, p,q) >= 0.f);
        }

        if (cdt_orientation(cdt_dst(l)->pos, p,q) == 0.f) { // l is on-line
            l = cdt_sym(cdt_dnext(l));
            r = cdt_lprev(r);
            pivot = l->org;
        } else {
            for (;;) {
                cdt_queue_push(&intersectings, cdt_lnext(r));
                cdt_vertex *s = cdt_dnext(cdt_dnext(l))->org;
                if (cdt_orientation(s->pos, p,q) < 0.f) { // right
                    l = cdt_dnext(l);
                    r = cdt_oprev(l);
                } else if (cdt_orientation(s->pos, p,q) > 0.f) { // left
                    r = cdt_dprev(r);
                    l = cdt_onext(r);
                } else { // on-line
                    r = cdt_dnext(cdt_dnext(l));
                    l = cdt_onext(r);
                    pivot = s;
                    break;
                }
            }
        }
    }



    // The input consists of the collected intersecting edges.
    // Using this input, we will perform the required edge flips.
    //
    cdt_quad_edge_array flip_stack = {0};
    while (!cdt_queue_empty(&intersectings)) {
        cdt_quad_edge *e = cdt_queue_pop(&intersectings);

        cdt_vec2 a = e->org->pos;
        cdt_vec2 b = cdt_dst(cdt_oprev(e))->pos;
        cdt_vec2 c = cdt_dst(e)->pos;
        cdt_vec2 d = cdt_lprev(e)->org->pos;

        if (cdt_is_constrained(cdt_get_edge(e))) {
            cdt_assert(!"Intersecting constrains isn't allowed. Why would you do that? It is not covered atm. Sorry!");
        }

        if (cdt_is_convex(a, b, c, d)) {
            cdt_swap(e);
            if (cdt_orientation(e->org->pos, p,q)*cdt_orientation(cdt_dst(e)->pos, p,q) < 0.f) {
                cdt_queue_push(&intersectings, e);
            } else {
                cdt_quad_edge_array_push(&flip_stack, e);
                cdt_quad_edge_array_push(&flip_stack, cdt_sym(e));
            }
        } else {
            cdt_queue_push(&intersectings, e);
        }
    }

    // Assign an ID to the edges that lie on the constrained edge by walking 
    // from the start vetex to the end vertex.
    for (cdt_vertex *cur = vert1, *nxt = 0;;cur = nxt) {
        if (cur == vert2) { break; }

        for (int i = 0; i < cur->edges.num; i+=1) {
            cdt_quad_edge *e = cur->edges.data[i];
            nxt = cdt_dst(e);
            if (cdt_orientation(nxt->pos, p,q) == 0.f) {
                cdt_float x1 = q.x - cur->pos.x;
                cdt_float y1 = q.y - cur->pos.y;
                cdt_float x2 = nxt->pos.x - cur->pos.x;
                cdt_float y2 = nxt->pos.y - cur->pos.y;
                if (x1*x2 + y1*y2 > 0.f) {
                    cdt_id_array_push(&cdt_get_edge(e)->ids, id);
                    break;
                }
            }
        }
    }

    // Now that all relevant edges have been marked as constrained, it's safe to 
    // perform edge flips, since constrained edges will be skipped.
    //
    cdt_flip_until_stack_is_empty(&flip_stack);
    cdt_free(flip_stack.data);
    cdt_free(intersectings.data);
}

//
// APIs
//
void cdt_init(cdt_context *ctx, cdt_float x1, cdt_float y1, cdt_float x2, cdt_float y2, cdt_float x3, cdt_float y3) {
    memset(ctx, 0, sizeof(cdt_context));

    // @Todo: Support CW?
    cdt_vec2 a = {0};
    cdt_vec2 b = {0};
    cdt_vec2 c = {0};
    a.x = x1; a.y = y1;
    b.x = x2; b.y = y2;
    c.x = x3; c.y = y3;

    cdt_vertex *va = cdt_create_vertex(ctx, a);
    cdt_vertex *vb = cdt_create_vertex(ctx, b);
    cdt_vertex *vc = cdt_create_vertex(ctx, c);

    cdt_quad_edge *ab = cdt_create_edge(ctx, va, vb);
    cdt_quad_edge *bc = cdt_create_edge(ctx, vb, vc);
    cdt_quad_edge *ca = cdt_create_edge(ctx, vc, va);

    cdt_splice(cdt_sym(ab), bc);
    cdt_splice(cdt_sym(bc), ca);
    cdt_splice(cdt_sym(ca), ab);
}

void cdt_insert(cdt_context *ctx, cdt_id id, cdt_float x1, cdt_float y1, cdt_float x2, cdt_float y2) {
    cdt_vec2 p1;
    p1.x = x1;
    p1.y = y1;
    cdt_vertex *vtx1 = cdt_insert_point(ctx, p1);

    cdt_vec2 p2;
    p2.x = x2;
    p2.y = y2;
    cdt_vertex *vtx2 = cdt_insert_point(ctx, p2);

    cdt_insert_segment(id, vtx1, vtx2);
}

void cdt_remove(cdt_context *ctx, cdt_id id) {
    // @Note: Since the pointer is a key, it is not deterministic. That is, 
    //        it's hard to reproduce the bug.
    cdt_vertex_array vertices = {0};

    // Get a set of vertices to check. Yes, it is a stupid linear array.
    for (int idx = 0; idx < ctx->edges.num; idx+=1) {
        cdt_edge *e = ctx->edges.data[idx];
        cdt_id_array *ids = &e->ids;
        for (int j = 0; j < ids->num; j+=1) {
            if (ids->data[j] == id) {
                ids->num-=1;
                ids->data[j] = ids->data[ids->num];
                   
                int is_in1 = 0;
                int is_in2 = 0;
                for (int i = 0; i < vertices.num; i+=1) {
                    if (vertices.data[i] == e->e[0].org) { is_in1 = 1; }
                    if (vertices.data[i] == e->e[2].org) { is_in2 = 1; }
                }
                if (!is_in1) { cdt_vertex_array_push(&vertices, e->e[0].org); }
                if (!is_in2) { cdt_vertex_array_push(&vertices, e->e[2].org); }

                break;
            }
        }
    }

    for (int i = 0; i < vertices.num; i+=1) {
        cdt_vertex *v = vertices.data[i];
        int should_destroy = 1;
        for (int j = 0; j < v->edges.num; j+=1) {
            cdt_quad_edge *e = v->edges.data[j];
            if (cdt_is_constrained(cdt_get_edge(e))) {
                should_destroy = 0;
                break;
            }
        }
        if (should_destroy) { cdt_destroy_vertex(ctx, v); }
    }

    cdt_free(vertices.data);
}

int cdt_get_vertex_count(cdt_context *ctx) {
    return ctx->vertices.num;
}

int cdt_get_edge_count(cdt_context *ctx) {
    return ctx->edges.num;
}

int cdt_get_triangle_count(cdt_context *ctx) {
    // F = 2 - V + E
    return 2 - ctx->vertices.num + ctx->edges.num;
}

// If the point lies exactly on a vertex or an edge, an arbitrary triangle 
// containing that vertex or edge is returned.
//
cdt_triangle cdt_get_triangle_containing_point(cdt_context *ctx, cdt_float x, cdt_float y) {
    cdt_triangle result = {0};
    cdt_locate_result loc = cdt_locate_point(ctx, x, y);

    result.edges[0] = loc.edge;
    result.edges[1] = cdt_lnext(result.edges[0]);
    result.edges[2] = cdt_lnext(result.edges[1]);
    result.x[0] = result.edges[0]->org->pos.x;
    result.y[0] = result.edges[0]->org->pos.y;
    result.x[1] = result.edges[1]->org->pos.x;
    result.y[1] = result.edges[1]->org->pos.y;
    result.x[2] = result.edges[2]->org->pos.x;
    result.y[2] = result.edges[2]->org->pos.y;

    return result;
}

// @Todo: If the edge is part of the super-triangle, the adjacent triangle will
//        also be a super-triangle. This is a bit awkward.
//
cdt_triangles cdt_get_adjacent_triangles(cdt_triangle triangle) {
    cdt_triangles result = {0};
    for (int i = 0; i < 3; ++i) {
        cdt_triangle *tri = result.triangles + i;
        {
            tri->edges[0] = cdt_sym(triangle.edges[i]);
            tri->edges[1] = cdt_lnext(tri->edges[0]);
            tri->edges[2] = cdt_lnext(tri->edges[1]);
            tri->x[0] = tri->edges[0]->org->pos.x;
            tri->y[0] = tri->edges[0]->org->pos.y;
            tri->x[1] = tri->edges[1]->org->pos.x;
            tri->y[1] = tri->edges[1]->org->pos.y;
            tri->x[2] = tri->edges[2]->org->pos.x;
            tri->y[2] = tri->edges[2]->org->pos.y;
        }

    }
    return result;
}

// Returns the quad-edge of 'src' triangle that leads to 'dst' triangle.
// Returns 0 if not found.
//
cdt_quad_edge *cdt_get_portal_edge(cdt_triangle src, cdt_triangle dst) {
    for (int i = 0; i < 3; ++i) {
        cdt_quad_edge *e1 = cdt_sym(src.edges[i]);
        for (int j = 0; j < 3; ++j) {
            cdt_quad_edge *e2 = dst.edges[j];
            if (e1 == e2) {
                return e1;
            }
        }
    }
    return 0;
}

int cdt_is_constrained(cdt_edge *edge) {
    return edge->ids.num > 0;
}

int cdt_is_quad_edge_constrained(cdt_quad_edge *quad_edge) {
    return cdt_is_constrained(cdt_get_edge(quad_edge));
}

void cdt_get_all_triangles(cdt_context *ctx, cdt_triangle *out_triangles) {
    int num_edge = cdt_get_edge_count(ctx);
    cdt_quad_edge **quad_edge_visited = (cdt_quad_edge **)cdt_malloc(sizeof(cdt_quad_edge *)*num_edge*2);
    int next = 0;

    // DFS.
    cdt_quad_edge_array stk = {0};
    cdt_quad_edge_array_push(&stk, &ctx->edges.data[0]->e[0]);
    int idx = 0;
    while (stk.num > 0) {
        cdt_quad_edge *e1 = cdt_stack_pop(&stk);
        cdt_quad_edge *e2 = cdt_lnext(e1);
        cdt_quad_edge *e3 = cdt_lnext(e2);

        int skip = 0;
        for (int i = 0; i < next; ++i) {
            if (quad_edge_visited[i]==e1) {
                skip = 1;
                break;
            }
        }
        if (skip) { continue; }

        quad_edge_visited[next++] = e1;
        quad_edge_visited[next++] = e2;
        quad_edge_visited[next++] = e3;

        cdt_triangle *tri = out_triangles + idx;
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

    cdt_free(quad_edge_visited);
}



#endif





/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2025 Seong Woo Lee
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
