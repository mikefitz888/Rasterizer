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
    fvec3 f;
    ivec3 i;
} ufvec3;

typedef struct __attribute__((packed)) {
    ufvec3 v0, v1, v2;
    uint id;
} triplet;

kernel void projection(global triplet* const wst, global triplet* const sst, const float3 campos) { //world-space triangle, screen-space triangles, camera-position
    uint id = get_global_id(0);
    global triplet* s = &(sst[id]);
    global triplet* t = &(wst[id]);
    //printf("Addresses: %d %d %d\n", s, t, sizeof(triplet));

    //for (int i = 0; i < 3; i++) {
        //printf("%f %f %f => %d %d\n", t->vertices[i].f[0], t->vertices[i].f[1], t->vertices[i].f[2], s->vertices[i].i[0], s->vertices[i].i[1]);
        //How to read this mess: s[i] is a vertex, '.i' or '.f' interprets it as int/float respectively, and then [0/1/2] accesses x/y/z
        //s->vertices[i].i[0] = (uint)((t->vertices[i].f[0] - campos.x) / ((t->vertices[i].f[2] - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
        //s->vertices[i].i[1] = (uint)((t->vertices[i].f[1] - campos.y) / ((t->vertices[i].f[2] - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
        //float3 vertex = (t->vertices[i].f[0], t->vertices[i].f[1], t->vertices[i].f[2]);
        //s->vertices[i].i[2] = fast_distance(vertex, campos);  
    //}

    s->v0.i.x = (uint)(((t->v0.f.x - campos.x) / (t->v0.f.z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    s->v0.i.y = (uint)(((t->v0.f.y - campos.y) / (t->v0.f.z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
    s->v0.i.z = distance((float3)(t->v0.f.x, t->v0.f.y, t->v0.f.z), campos);
    printf("%f %f %f => %d %d\n", t->v0.f.x, t->v0.f.y, t->v0.f.z, s->v0.i.x, s->v0.i.y);

    s->v1.i.x = (uint)(((t->v1.f.x - campos.x) / (t->v1.f.z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    s->v1.i.y = (uint)(((t->v1.f.y - campos.y) / (t->v1.f.z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
    s->v1.i.z = distance((float3)(t->v1.f.x, t->v1.f.y, t->v1.f.z), campos);
    printf("%f %f %f => %d %d\n", t->v1.f.x, t->v1.f.y, t->v1.f.z, s->v1.i.x, s->v1.i.y);

    s->v2.i.x = (uint)(((t->v2.f.x - campos.x) / (t->v2.f.z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    s->v2.i.y = (uint)(((t->v2.f.y - campos.y) / (t->v2.f.z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);
    s->v2.i.z = distance((float3)(t->v2.f.x, t->v2.f.y, t->v2.f.z), campos);
    printf("%f %f %f => %d %d\n", t->v2.f.x, t->v2.f.y, t->v2.f.z, s->v2.i.x, s->v2.i.y);



    //screen_space_vertices[id] = (uint)(((vertices[id] - campos.x) / (vertices[id+2] - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH);
    //screen_space_vertices[id+1] = (uint)(((vertices[id+1] - campos.y) / (vertices[id+2] - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT);

    //float3 t = (vertices[id], vertices[id+1], vertices[id+2]);
    //screen_space_vertices[id+2] = (uint)fast_distance(t, campos); //Used for depth-buffering

    //printf("%f %f %f => %d %d\n", vertices[id], vertices[id+1], vertices[id+2], screen_space_vertices[id], screen_space_vertices[id+1]);
}