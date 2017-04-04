typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
    uint triangle_id;

    // Interpolators
    float va, vb, vc; // Barycentric coordinates

                      // Stored information
    float depth;
    float x, y, z;
    float nx, ny, nz;
    float uvx, uvy;

} Fragment;

kernel void shaedars(global Fragment* const fragment_buffer, global Material* const material_buffer) {


}





