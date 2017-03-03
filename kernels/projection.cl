#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

//#define SCREEN_WIDTH 500 //For testing compilation with Nsight, actually provided by c++ program

//#define SCREEN_HEIGHT 500

typedef struct {
    float x, y, z;
} fvec3;

typedef struct {
    uint x, y;
    float z;
} ivec3;

typedef union {
    float3 f;
    int3 i;
} ufvec3;

typedef struct __attribute__((packed)) {
    ufvec3 v0, v1, v2;
    uint id;
} triplet;

kernel void projection(global triplet* const wst, global triplet* /*const*/ sst, const float3 campos) { //world-space triangle, screen-space triangles, camera-position

    uint id = get_global_id(0);

    triplet s; /*= sst[id];*/
    triplet t = wst[id];

    //triplet now consists of float3 and int3 so vector instructions can be used
    s.v0.i.x = (int)(((t.v0.f.x - campos.x) / (t.v0.f.z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    s.v0.i.y = (int)(((t.v0.f.y - campos.y) / (t.v0.f.z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
    s.v0.i.z = distance(t.v0.f, campos);
    //printf("%f %f %f => %d %d\n", t->v0.f.x, t->v0.f.y, t->v0.f.z, s->v0.i.x, s->v0.i.y);

    s.v1.i.x = (int)(((t.v1.f.x - campos.x) / (t.v1.f.z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    s.v1.i.y = (int)(((t.v1.f.y - campos.y) / (t.v1.f.z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
    s.v1.i.z = distance(t.v1.f, campos);
    //printf("%f %f %f => %d %d\n", t->v1.f.x, t->v1.f.y, t->v1.f.z, s->v1.i.x, s->v1.i.y);

    s.v2.i.x = (int)(((t.v2.f.x - campos.x) / (t.v2.f.z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    s.v2.i.y = (int)(((t.v2.f.y - campos.y) / (t.v2.f.z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
    s.v2.i.z = distance(t.v2.f, campos);
    //printf("%f %f %f => %d %d\n", t->v2.f.x, t->v2.f.y, t->v2.f.z, s->v2.i.x, s->v2.i.y);

    // Write result to global memory
    sst[id] = s;
}