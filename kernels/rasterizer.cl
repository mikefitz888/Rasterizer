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
kernel void rasterizer(global AABB* const task_buffer, global triplet* const sst) { 
    uint id = get_global_id(0);
    global triplet* s = &(sst[id]);
    //printf("%d %d\n", s->v0.i.x, s->v0.i.y);

    AABB task;
    int2 a = (int2)(s->v0.i.x, s->v0.i.y);
    int2 b = (int2)(s->v1.i.x, s->v1.i.y);
    int2 c = (int2)(s->v2.i.x, s->v2.i.y);

    task.a = a;
    task.d0 = s->v0.i.z;
    task.d1 = s->v1.i.z;
    task.d2 = s->v2.i.z;
    task.triangle_id = id;

    //computeBaryCentric pre-reqs
    task.v0 = convert_float2(b - a);
    task.v1 = convert_float2(c - a);
    //printf("(%d %d) - (%d %d) = (%f %f)\n", b.x, b.y, a.x, a.y, v0.x, v0.y);
    
    task.inv_denom = 1.f / (task.v0.x * task.v1.y - task.v1.x * task.v0.y); //Shortcut for 2d systems
    //printf("(1.f / (%f * %f - %f * %f)) = %f\n", v0.x, v1.y, v1.x, v0.y, inv_denom);
    //bounding-box
    //printf("%d %d, %d %d, %d %d\n", a.x, a.y, b.x, b.y, c.x, c.y);
    task.minX = min(min(a.x, b.x), c.x);
    task.minY = min(min(a.y, b.y), c.y);
    task.maxX = max(max(a.x, b.x), c.x);
    task.maxY = max(max(a.y, b.y), c.y);

    task_buffer[id] = task;

    //printf("min (%d %d), max (%d %d)\n", minX, minY, maxX, maxY);

    //Spreading this task across more cores
    /*for (uint x = minX; x <= maxX; x++) {
        for (uint y = minY; y <= maxY; y++) {
            //Compute barycentric coordinates
            float2 v2 = convert_float2((int2)(x, y) - a);
            
            float v = (v2.x * v1.y - v1.x * v2.y) * inv_denom;
            float w = (v0.x * v2.y - v2.x * v0.y) * inv_denom;
            float u = 1.f - v - w;


            if (0 || (u <= 1 && v <= 1 && w <= 1 && u >= 0 && v >= 0 && w >= 0)) {
                float depth = (u*s->v0.i.z + v*s->v1.i.z + w*s->v2.i.z);

                //Probably needs to be a lock here, can deal with it when it's an issue
                float d = min((frame_buf[x + y * SCREEN_WIDTH].depth), depth);
                if (d == depth || d == 0) {
                    atomic_xchg(&(frame_buf[x + y * SCREEN_WIDTH].depth), depth);
                    atomic_xchg(&(frame_buf[x + y * SCREEN_WIDTH].triangle_id), id);
                    
                }
            }
        }
    }*/

    //Render vertices
    //uint p_id = s->v0.i.x + s->v0.i.y * SCREEN_WIDTH;
    //frame_buf[p_id].r = (uchar)255;

    //p_id = s->v1.i.x + s->v1.i.y * SCREEN_WIDTH;
    //frame_buf[p_id].g = 255;

    //p_id = s->v2.i.x + s->v2.i.y * SCREEN_WIDTH;
    //frame_buf[p_id].b = 255;
    //printf("%d = %d %d\n", p_id, s->v2.i.x, s->v2.i.y);
}

