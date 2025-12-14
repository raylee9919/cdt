/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

// Vec2
// 
Vec2 vec2(f32 x, f32 y) {
    return {x, y};
}

f32 lerp(f32 a, f32 b, f32 t) {
    return a + (b - a)*t;
}

Vec2 operator+(Vec2 a, Vec2 b) {
    return {a.x+b.x, a.y+b.y};
}

Vec2& operator+=(Vec2 &a, Vec2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vec2 operator-(Vec2 a, Vec2 b) {
    return {a.x-b.x, a.y-b.y};
}

Vec2 operator*(Vec2 a, f32 b) {
    return {a.x*b, a.y*b};
}

f32 length(Vec2 v) {
    return sqrtf(v.x*v.x + v.y*v.y);
}

f32 distance(Vec2 a, Vec2 b) {
    return length(a - b);
}

Vec2 normalize(Vec2 v) {
    Vec2 result = {};
    f32 len = length(v); 
    if (len != 0.f) {
        f32 invlen = 1.f / len;
        result.x = v.x * invlen;
        result.y = v.y * invlen;
    }
    return result;
}

f32 triarea2(Vec2 a, Vec2 b, Vec2 c) {
    Vec2 p = c - b;
    Vec2 q = a - b;
    return p.x*q.y - p.y*q.x;
}

f32 orientation(Vec2 point, Directed_Edge edge) {
    return triarea2(point, edge.src, edge.dst);
}

// Vec3
//
Vec3 vec3(f32 v) {
    Vec3 result = {v,v,v};
    return result;
}

Vec3 lerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 result = {};
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

// Vec4
//
Vec4 lerp(Vec4 a, Vec4 b, f32 t) {
    Vec4 result = {};
    result.r = lerp(a.r, b.r, t);
    result.g = lerp(a.g, b.g, t);
    result.b = lerp(a.b, b.b, t);
    result.a = lerp(a.a, b.a, t);
    return result;
}

Vec4 vec4(Vec3 rgb, f32 a) {
    Vec4 result = {};
    result.r = rgb.x;
    result.g = rgb.y;
    result.b = rgb.z;
    result.a = a;
    return result;
}

M4x4 m4x4_identity(void) {
    M4x4 result = {{
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},
    }};
    return result;
}

M4x4 operator *(M4x4 a, M4x4 b) {
    M4x4 R = {};
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < 4; ++i) {
                R.e[r][c] += a.e[r][i] * b.e[i][c];
            }
        }
    }
    return R;
}

// Render matrices
//
M4x4 m4x4_model(Vec2 position, Vec2 offset) {
    f32 a = position.x + offset.x;
    f32 b = position.y + offset.y;
    M4x4 result = {{
        { 1, 0, 0, a},
        { 0, 1, 0, b},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},
    }};
    return result;
}

M4x4 m4x4_view(Vec2 position) {
    f32 x = position.x;
    f32 y = position.y;
    M4x4 result = {{
        { 1, 0, 0,-x},
        { 0, 1, 0,-y},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},
    }};
    return result;
}

M4x4 m4x4_proj(Vec2 resolution) {
    f32 a =  2.f / resolution.x;
    f32 b = -2.f / resolution.y;
    M4x4 result = {{
        { a, 0, 0, 0},
        { 0, b, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},
    }};
    return result;
}

M4x4 m4x4_view_proj(Vec2 camera_position, Vec2 resolution) {
    return m4x4_proj(resolution)*m4x4_view(camera_position);
}


// 
//
f32 parabolic_wave(f32 t) {
    t = t - floorf(t);
    t =  8.f*t*(1.f - t) - 1.f;
    return t;
}
