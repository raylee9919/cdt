/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

struct Sprite_Vertex {
    Vec2 position;
    Vec2 uv;
};

struct Colored_Vertex {
    Vec3 position;
    Vec4 color;
};

typedef u32 Texture_Type;
enum {
    TEXTURE_TYPE_NULL   = 0,
    TEXTURE_TYPE_PLAYER,
    TEXTURE_TYPE_SKELETON,
    TEXTURE_TYPE_GROUND,
    TEXTURE_TYPE_BUILDING,
    TEXTURE_TYPE_COUNT
};

struct Texture {
    Texture_Type type;
    GLuint       gl_id;
    u16          width;
    u16          height;
    u16          sprite_width;
    u16          sprite_height;
    u16          num_row;
    u16          num_col;
};

struct Animation {
    Texture_Type spritesheet;
    u32 start_frame_index;
    u32 num_frames;
    f32 frame_interval;
};

typedef u32 Order_Type;
enum {
    ORDER_TYPE_IDLE,
    ORDER_TYPE_MOVE,
    ORDER_TYPE_DIE,
    ORDER_TYPE_COUNT
};

typedef u32 Animation_Type;
enum {
    ANIMATION_TYPE_INVALID=0,
    ANIMATION_TYPE_PLAYER_IDLE,
    ANIMATION_TYPE_PLAYER_RUN,
    ANIMATION_TYPE_PLAYER_DIE,
    ANIMATION_TYPE_SKELETON_IDLE,
    ANIMATION_TYPE_SKELETON_DIE,
    ANIMATION_TYPE_SKELETON_ATTACK,
    ANIMATION_TYPE_SKELETON_HIT,
    ANIMATION_TYPE_SKELETON_MOVE,
    ANIMATION_TYPE_COUNT
};

typedef u32 Entity_Type;
enum {
    ENTITY_TYPE_NULL    = 0,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_COUNT
};

typedef u32 Entity_Flags;
enum {
    ENTITY_FLAG_DRAW            = 0x1,
    ENTITY_FLAG_ANIMATE         = 0x2,
    ENTITY_FLAG_FLIP_TEX_U      = 0x4,
    ENTITY_FLAG_MOUSE_CONTROL   = 0x8,
    ENTITY_FLAG_DIEABLE         = 0x10,
};

struct Entity {
    Entity         *next;
    Entity         *prev;

    u32             id;
    Entity_Flags    flags;

    Vec2            position;
    Vec2            size;
    f32             radius;
    f32             speed;

    f32             hp;

    Vec2            offset;
    Texture_Type    texture;

    // Order
    Order_Type      order;
    Vec2            order_position;
    Stack<Vec2>     path_stack;
    Stack<Vec2>     path_shadow_stack;
    

    // Animation
    f32 animation_t;
    u32 animation_frame_offset;

    // Navigation
    Vec2       *navmesh;
    Array<Vec2> triangles_in_path;

    // Draw
    f32 u1, v1, u2, v2;
};

struct Engine {
    GLFWwindow *window;
    int framebuffer_width;
    int framebuffer_height;

    u64  tick;
    f64  last_frame_time;
    f32  dt;
    f32  shader_time;
    Vec2 resolution;

    u32 next_entity_id;
    Entity *entity_sentinel;

    cdt_context navmesh;

    Vec2 camera_position;

    Texture textures[TEXTURE_TYPE_COUNT];
    Animation animations[ANIMATION_TYPE_COUNT];
    Animation_Type animation_map[TEXTURE_TYPE_COUNT][ORDER_TYPE_COUNT];

    GLuint sprite_shader;
    GLuint sprite_shader_model;
    GLuint sprite_shader_vp;

    GLuint simple_shader;
    GLuint simple_shader_vp;
    GLuint simple_shader_color;

    Array<Colored_Vertex> line_shader_buffer;
};

static Engine *engine;
