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

float3 computeBaryCentric(float2 p, float2 a, float2 b, float2 c){//Point p, Triangle(a, b, c) 
    float2 v0 = b - a;
    float2 v1 = c - a;
    float2 v2 = p - a;

    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float inv_denom = 1 / (d00 * d11 - d01 * d01);
    float v = (d11 * d20 - d01 * d21) * inv_denom;
    float w = (d00 * d21 - d01 * d20) * inv_denom;
    float u = 1.f - v - w;

    return (float3)(u, v, w);
}


kernel void rasterizer(global Fragment* const frame_buf, global triplet* const sst) {
   
    uint id = get_global_id(0);
    global triplet* s = &(sst[id]);

    float2 a = (s->v0.i.x, s->v0.i.y);
    float2 b = (s->v1.i.x, s->v1.i.y);
    float2 c = (s->v2.i.x, s->v2.i.y);

    //computeBaryCentric(#, a, b, c);
    
    uint p_id = s->v0.i.x + s->v0.i.y * SCREEN_WIDTH; 
    frame_buf[p_id].r = (uchar)255;

    p_id = s->v1.i.x + s->v1.i.y * SCREEN_WIDTH;
    frame_buf[p_id].g = 255;

    p_id = s->v2.i.x + s->v2.i.y * SCREEN_WIDTH;
    frame_buf[p_id].b = 255;
    //printf("%d = %d %d\n", p_id, s->v2.i.x, s->v2.i.y);
}

