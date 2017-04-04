#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
    uint triangle_id;
    float depth; //If depth == 0, fragment is not initialized
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

typedef struct __attribute__((packed)) { 
    int minX, minY, maxX, maxY;
    float2 v0, v1;
    int2 a;
    float inv_denom, d0, d1, d2;
    size_t triangle_id, offset;
} AABB;


//kernel void rasterizer(global Fragment* const frame_buf, global triplet* const sst) {
kernel void rasterizer(global AABB* task_buffer, global triplet* const sst) { 
    uint id = get_global_id(0);
    triplet s = sst[id];
    //printf("%d %d\n", s->v0.i.x, s->v0.i.y);

    AABB task;
    int2 a = (int2)((int)s.v0.f.x, (int)s.v0.f.y);
    int2 b = (int2)((int)s.v1.f.x, (int)s.v1.f.y);
    int2 c = (int2)((int)s.v2.f.x, (int)s.v2.f.y);

    task.a = a;
    task.d0 = s.v0.i.z;
    task.d1 = s.v1.i.z;
    task.d2 = s.v2.i.z;
    task.triangle_id = id;

    //computeBaryCentric pre-reqs
    task.v0 = convert_float2(b - a);
    task.v1 = convert_float2(c - a);
    
    task.inv_denom = 1.0f / (task.v0.x * task.v1.y - task.v1.x * task.v0.y); //Shortcut for 2d systems
    task.minX = min(min(a.x, b.x), c.x);
    task.minY = min(min(a.y, b.y), c.y);
    task.maxX = max(max(a.x, b.x), c.x);
    task.maxY = max(max(a.y, b.y), c.y);

 /*   if (id == 0) {
        printf("%d %d %d %d \n", task.minX, task.minY, task.maxX, task.maxY);
    }*/

    task_buffer[id] = task;
}

