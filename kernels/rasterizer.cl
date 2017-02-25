#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
    uint triangle_id;
    float depth;
} Fragment;

typedef struct {
    float x, y, z;
} fvec3;

typedef struct {
    uint x, y;
    float z;
} ivec3;


typedef union {
    fvec3 f;
    ivec3 i;
} ufvec3;

typedef struct __attribute__((packed)) {
    ufvec3 v0, v1, v2;
    uint id;
} triplet;


kernel void rasterizer(global Fragment* const frame_buf, global triplet* const sst) {

    uint id = get_global_id(0);
    global triplet* s = &(sst[id]);
    printf("%d %d\n", s->v0.i.x, s->v0.i.y);
    uint2 a = (uint2)(s->v0.i.x, s->v0.i.y);
    uint2 b = (uint2)(s->v1.i.x, s->v1.i.y);
    uint2 c = (uint2)(s->v2.i.x, s->v2.i.y);

    //computeBaryCentric pre-reqs
    float2 v0 = convert_float2(b - a);
    float2 v1 = convert_float2(c - a);
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float inv_denom = 1 / (d00 * d11 - d01 * d01);

    //bounding-box
    //printf("%d %d, %d %d, %d %d\n", a.x, a.y, b.x, b.y, c.x, c.y);
    uint minX = min(min(a.x, b.x), c.x);
    uint minY = min(min(a.y, b.y), c.y);
    uint maxX = max(max(a.x, b.x), c.x);
    uint maxY = max(max(a.y, b.y), c.y);
    //printf("min (%f %f), max (%f %f)\n", minX, minY, maxX, maxY);
    for (uint x = minX; x <= maxX; x++) {
        for (uint y = minY; y <= maxY; y++) {
            float2 v2 = convert_float2((uint2)(x, y) - a);
            float d20 = dot(v2, v0);
            float d21 = dot(v2, v1);

            float v = (d11 * d20 - d01 * d21) * inv_denom;
            float w = (d00 * d21 - d01 * d20) * inv_denom;
            float u = 1.f - v - w;
            if (u <= 1 && v <= 1 && w <= 1 && u >= 0 && v >= 0 && w >= 0) {
                frame_buf[x + y * SCREEN_WIDTH].r = 128;
            }
        }
    }

    uint p_id = s->v0.i.x + s->v0.i.y * SCREEN_WIDTH;
    frame_buf[p_id].r = (uchar)255;

    p_id = s->v1.i.x + s->v1.i.y * SCREEN_WIDTH;
    frame_buf[p_id].g = 255;

    p_id = s->v2.i.x + s->v2.i.y * SCREEN_WIDTH;
    frame_buf[p_id].b = 255;
    //printf("%d = %d %d\n", p_id, s->v2.i.x, s->v2.i.y);
}

