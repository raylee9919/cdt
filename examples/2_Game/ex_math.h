/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

typedef int8_t      s8;  
typedef int16_t     s16; 
typedef int32_t     s32; 
typedef int64_t     s64; 
typedef uint8_t     u8;  
typedef uint16_t    u16; 
typedef uint32_t    u32; 
typedef uint64_t    u64; 
typedef float       f32; 
typedef double      f64; 

struct Vec2 {
    f32 x,y;
};

struct Vec3 {
    f32 x,y,z;
};

struct Vec4 {
    f32 r,g,b,a;
};

struct M4x4 {
    f32 e[4][4];
};

#define F32_MAX ( 3.402823e+38f)
#define F32_MIN (-3.402823e+38f)
