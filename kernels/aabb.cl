typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
    uint triangle_id;
    float depth; //If depth == 0, fragment is not initialized
} Fragment;

typedef struct __attribute__((packed)) {
    int minX, minY, maxX, maxY;
    float2 v0, v1;
    int2 a;
    float inv_denom, d0, d1, d2;
    size_t triangle_id, offset;
} AABB;

kernel void aabb(global Fragment* const fragment_buffer, global AABB* const AABB_buffer, uint max_pixel){ //can't put size_t as an argument for some reason
    size_t aabb_pixel_id = get_global_id(0);
    size_t aabb_guess_id = aabb_pixel_id / max_pixel;

    global AABB* guess = &AABB_buffer[aabb_guess_id];

    //Find Task
    size_t max_attempts = 0;
    while (max_attempts < 320) {
        if (guess->offset > aabb_pixel_id) {
            guess--;
        }
        else if (guess->offset + (guess->maxX - guess->minX) * (guess->maxY - guess->minY) < aabb_pixel_id) {
            guess++;
        }
        else {
            break;
        }
        max_attempts++;
    }

    //Perform Task
    int sub_pixel = aabb_pixel_id - guess->offset;
    int x = guess->minX + (sub_pixel % (guess->maxX - guess->minX));
    int y = guess->minY + (sub_pixel / (guess->maxX - guess->minX));
    float2 v2 = convert_float2((int2)(x, y) - guess->a);
    float2 v0 = guess->v0;
    float2 v1 = guess->v1;
    float v = (v2.x * v1.y - v1.x * v2.y) * guess->inv_denom;
    float w = (v0.x * v2.y - v2.x * v0.y) * guess->inv_denom;
    float u = 1.f - v - w;


    if (0 || (u <= 1 && v <= 1 && w <= 1 && u >= 0 && v >= 0 && w >= 0)) {
        float depth = (u*guess->d0 + v*guess->d1 + w*guess->d2);

        //Probably needs to be a lock here, can deal with it when it's an issue
        float d = min((fragment_buffer[x + y * SCREEN_WIDTH].depth), depth);
        if (d == depth || d == 0) {
            atomic_xchg(&(fragment_buffer[x + y * SCREEN_WIDTH].depth), depth);
            atomic_xchg(&(fragment_buffer[x + y * SCREEN_WIDTH].triangle_id), guess->triangle_id);

        }
    }
}