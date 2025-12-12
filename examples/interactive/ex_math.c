/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

f32 lerp(f32 a, f32 b, f32 t) {
    return a*t + b*(1.f-t);
}

Vec2 litv2(f32 x, f32 y) {
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

Vec2 addv2(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

Vec2 subv2(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

Vec2 mulv2f32(Vec2 a, f32 b) {
    Vec2 result;
    result.x = a.x*b;
    result.y = a.y*b;
    return result;
}

f32 lenv2(Vec2 v) {
    return sqrtf(v.x*v.x + v.y*v.y);
}

f32 distv2(Vec2 a, Vec2 b) {
    return lenv2(subv2(a,b));
}

Vec2 normv2(Vec2 v) {
    Vec2 result = {0};
    f32 len = lenv2(v); 
    if (len != 0.f) {
        f32 invlen = 1.f / len;
        result.x = v.x * invlen;
        result.y = v.y * invlen;
    }
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

M4x4 m4x4_mul(M4x4 a, M4x4 b) {
    M4x4 R = {0};
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
    f32 b =  2.f / resolution.y;
    M4x4 result = {{
        { a, 0, 0, 0},
        { 0, b, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},
    }};
    return result;
}

M4x4 m4x4_view_proj(Vec2 camera_position, Vec2 resolution) {
    return m4x4_mul(m4x4_proj(resolution), m4x4_view(camera_position));
}
