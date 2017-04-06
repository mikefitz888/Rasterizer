#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
//#define SCREEN_WIDTH 500 //For testing compilation with Nsight, actually provided by c++ program
//#define SCREEN_HEIGHT 500
typedef struct {
    float x, y, z;

} fvec3;

inline fvec3 set3(float x, float y, float z) {
    fvec3 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    return ret;
}

typedef struct {
    float x, y, z, w;

    
} fvec4;

typedef struct {
    //float m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33;//
    float m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33;
} mat4;

inline fvec4 set4(float x, float y, float z, float w) {
    fvec4 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    ret.w = w;
    return ret;
}

inline fvec4 mul(fvec4 input, mat4 mat) {
    fvec4 vec;
    vec.x = input.x*mat.m00 + input.y*mat.m10 + input.z*mat.m20 + input.w*mat.m30;
    vec.y = input.x*mat.m01 + input.y*mat.m11 + input.z*mat.m21 + input.w*mat.m31;
    vec.z = input.x*mat.m02 + input.y*mat.m12 + input.z*mat.m22 + input.w*mat.m32;
    vec.w = input.x*mat.m03 + input.y*mat.m13 + input.z*mat.m23 + input.w*mat.m33;
    return vec;
}

typedef struct { 
    uint x, y;
    float z, w;
} ivec4;


typedef union {
    fvec4 f;
    ivec4 i;
} ufvec4;

typedef struct __attribute__((packed)) {
    ufvec4 v0, v1, v2;
    uint id;
} triplet;

kernel void projection(global triplet* wst, global triplet* sst, mat4 VIEW_MATRIX, mat4 PROJECTION_MATRIX, mat4 CLIP_MATRIX) { //world-space triangle, screen-space triangles, camera-position
    uint id = get_global_id(0);
    triplet t = wst[id];
    triplet s;
    //global triplet* t = &wst[id];

    // World pos
    fvec4 pos_0 = set4(t.v0.f.x, t.v0.f.y, t.v0.f.z, 1);
    fvec4 pos_1 = set4(t.v1.f.x, t.v1.f.y, t.v1.f.z, 1);
    fvec4 pos_2 = set4(t.v2.f.x, t.v2.f.y, t.v2.f.z, 1);

    // Transform to view position
    //fvec4 view_pos0, view_pos1, view_pos2;
    pos_0 = mul(pos_0, VIEW_MATRIX);
    pos_1 = mul(pos_1, VIEW_MATRIX);
    pos_2 = mul(pos_2, VIEW_MATRIX);

    // Project
    fvec4 proj_pos0, proj_pos1, proj_pos2;
    proj_pos0 = mul(pos_0, PROJECTION_MATRIX);
    proj_pos1 = mul(pos_1, PROJECTION_MATRIX);
    proj_pos2 = mul(pos_2, PROJECTION_MATRIX);

    /*proj_pos0.w = (proj_pos0.w == 0.0f) ? 1.0f : proj_pos0.w;
    proj_pos1.w = (proj_pos1.w == 0.0f) ? 1.0f : proj_pos1.w;
    proj_pos2.w = (proj_pos2.w == 0.0f) ? 1.0f : proj_pos2.w;*/

    // PERSPECTIVE DIVIDE
    proj_pos0.x /= proj_pos0.w; proj_pos0.y /= proj_pos0.w; proj_pos0.z /= proj_pos0.w; //proj_pos0.w = 1.0f;
    proj_pos1.x /= proj_pos1.w; proj_pos1.y /= proj_pos1.w; proj_pos1.z /= proj_pos1.w; //proj_pos1.w = 1.0f;
    proj_pos2.x /= proj_pos2.w; proj_pos2.y /= proj_pos2.w; proj_pos2.z /= proj_pos2.w; //proj_pos2.w = 1.0f;


    // Clipping
    /*proj_pos0 = mul(proj_pos0, CLIP_MATRIX);
    proj_pos1 = mul(proj_pos1, CLIP_MATRIX);
    proj_pos2 = mul(proj_pos2, CLIP_MATRIX);*/
    proj_pos0.x *= 0.5f; proj_pos0.x += 0.5f;
    proj_pos0.y *= 0.5f; proj_pos0.y += 0.5f;
    proj_pos0.x *= 600.0f; proj_pos0.y *= 480.0f;

    proj_pos1.x *= 0.5f; proj_pos1.x += 0.5f;
    proj_pos1.y *= 0.5f; proj_pos1.y += 0.5f;
    proj_pos1.x *= 600.0f; proj_pos1.y *= 480.0f;

    proj_pos2.x *= 0.5f; proj_pos2.x += 0.5f;
    proj_pos2.y *= 0.5f; proj_pos2.y += 0.5f;
    proj_pos2.x *= 600.0f; proj_pos2.y *= 480.0f;

    // Collect result
    s.v0.f.x = proj_pos0.x;
    s.v0.f.y = proj_pos0.y;
    s.v0.f.z = -pos_0.z; // Use view instead of projected z for depth as it equates to actual distance from camera, rather than re-scaled
    s.v0.f.w = proj_pos0.w;

    s.v1.f.x = proj_pos1.x;
    s.v1.f.y = proj_pos1.y;
    s.v1.f.z = -pos_1.z;
    s.v0.f.w = proj_pos1.w;

    s.v2.f.x = proj_pos2.x;
    s.v2.f.y = proj_pos2.y;
    s.v2.f.z = -pos_2.z;
    s.v2.f.w = proj_pos2.w;

    sst[id] = s;
}