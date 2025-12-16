/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

#include <stdio.h>
#include "../../cdt.h"

void print_vertices(cdt_triangle tri) {
    for (int i = 0; i < 3; i+=1) {
        cdt_quad_edge *quad_edge = tri.edges[i];
        cdt_vertex *origin = quad_edge->org;
        cdt_vec2 coord = origin->pos;

        printf("(%.2f, %.2f)\n", coord.x, coord.y);
    }
}

int main(void) {
    // Initialize the context.
    //
    cdt_context *ctx = (cdt_context *)malloc(sizeof(cdt_context));
    cdt_init(ctx, 0.f, 2048.f, -2048.f, -2048.f, 2048.f, -2048.f);

    
    // Inserting quad.
    //
    float points[4][2] = {
        {-2.f, 2.f},
        {-2.f,-2.f},
        { 2.f,-2.f},
        { 2.f, 2.f},
    };
    cdt_insert(ctx, 0, points[0][0], points[0][1], points[1][0], points[1][1]);
    cdt_insert(ctx, 0, points[1][0], points[1][1], points[2][0], points[2][1]);
    cdt_insert(ctx, 0, points[2][0], points[2][1], points[3][0], points[3][1]);
    cdt_insert(ctx, 0, points[3][0], points[3][1], points[0][0], points[0][1]);


    // Get all triangles.
    //
    int num_tri = cdt_get_triangle_count(ctx);
    cdt_triangle *triangles = (cdt_triangle *)malloc(num_tri*sizeof(cdt_triangle));
    cdt_get_all_triangles(ctx, triangles);


    // Iterate through all triangles and print the coordinates of their vertices.
    //
    for (int i = 0; i < num_tri; i+=1) {
        print_vertices(triangles[i]);
        printf("\n");
    }


    // Finds the triangle encloses point (3,0).
    //
    cdt_triangle tri = cdt_get_triangle_containing_point(ctx, 3.f, 0.f);
    print_vertices(tri);
    printf("\n");


    // For that triangle, which edges are constrained?
    //
    for (int i = 0; i < 3; i+=1) {
        cdt_quad_edge *quad_edge = tri.edges[i];
        if (cdt_is_quad_edge_constrained(quad_edge)) {
            printf("[%d] is constrained.", i);
        } else {
            printf("[%d] is not constrained.\n", i);
        }
    }
    printf("\n\n");

    // Also, for that triangle, what are the adjacent triangles?
    //
    cdt_triangles adj_tris = cdt_get_adjacent_triangles(tri);
    for (int i = 0; i < 3; i+=1) {
        cdt_triangle adj = adj_tris.triangles[i];
        printf("adj %d\n", i);
        print_vertices(adj);
        printf("\n");
    }
}
