/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */


typedef uint32_t Shape_Type;
enum {
    SHAPE_QUAD,
    SHAPE_STAR,
    SHAPE_COUNT,
};

typedef struct {
    f32 (*points[SHAPE_COUNT])[2];
    int  num_points[SHAPE_COUNT];
    Shape_Type current_type;
} Shape_Set;

static void shape_set_init(Shape_Set *set) {
    memset(set, 0, sizeof(*set));

    {
        Shape_Type type = SHAPE_QUAD;
        f32 points[][2] = {
            {-1.f, 1.f},
            {-1.f, 0.f},
            {-1.f,-1.f},
            { 0.f,-1.f},
            { 1.f,-1.f},
            { 1.f, 0.f},
            { 1.f, 1.f},
            { 0.f, 1.f},
        };
        int num = arrcnt(points);
        set->num_points[type] = num;

        set->points[type] = malloc(sizeof(f32)*2*num);
        for (int i = 0; i < num; ++i) {
            set->points[type][i][0] = points[i][0];
            set->points[type][i][1] = points[i][1];
        }
    }

    {
        Shape_Type type = SHAPE_STAR;
        f32 points[][2] = {
            { 0.59607f,  0.80293f},
            { 0.00393f,  0.38195f},
            {-0.57943f,  0.81502f},
            {-0.36204f,  0.12176f},
            {-0.95418f, -0.29922f},
            {-0.22768f, -0.30669f},
            {-0.01028f, -0.99995f},
            { 0.22132f, -0.31131f},
            { 0.94783f, -0.31878f},
            { 0.36447f,  0.11429f},
        };
        int num = arrcnt(points);
        set->num_points[type] = num;

        set->points[type] = malloc(sizeof(f32)*2*num);
        for (int i = 0; i < num; ++i) {
            set->points[type][i][0] = points[i][0];
            set->points[type][i][1] = points[i][1];
        }
    }
}
