#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#include "kernels/Utility/Matrix.cl"

kernel void projection(global triplet* wst, global triplet* sst, mat4 VIEW_MATRIX, mat4 PROJECTION_MATRIX, mat4 CLIP_MATRIX, float screen_width, float screen_height) { //world-space triangle, screen-space triangles, camera-position
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
    pos_0 = mulf4(pos_0, VIEW_MATRIX);
    pos_1 = mulf4(pos_1, VIEW_MATRIX);
    pos_2 = mulf4(pos_2, VIEW_MATRIX);

    // Project
    fvec4 proj_pos0, proj_pos1, proj_pos2;
    proj_pos0 = mulf4(pos_0, PROJECTION_MATRIX);
    proj_pos1 = mulf4(pos_1, PROJECTION_MATRIX);
    proj_pos2 = mulf4(pos_2, PROJECTION_MATRIX);

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
    proj_pos0.x *= screen_width; proj_pos0.y *= screen_height;

    proj_pos1.x *= 0.5f; proj_pos1.x += 0.5f;
    proj_pos1.y *= 0.5f; proj_pos1.y += 0.5f;
    proj_pos1.x *= screen_width; proj_pos1.y *= screen_height;

    proj_pos2.x *= 0.5f; proj_pos2.x += 0.5f;
    proj_pos2.y *= 0.5f; proj_pos2.y += 0.5f;
    proj_pos2.x *= screen_width; proj_pos2.y *= screen_height;

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