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

typedef struct __attribute__((packed)) { 
    float r, g, b, a;
} Material;

typedef struct __attribute__((packed)) {
    float x, y, z;
} vec3;

typedef struct __attribute__((packed)) {
    float x, y;
} vec2;

typedef struct __attribute__((packed)) {
    //Vertices
    vec3 v0, v1, v2;

    //Texture Coordinates
    vec2 uv0, uv1, uv2;

    //Normals
    vec3 n0, n1, n2;

    //Optional Parameters
    vec3 normal;
    vec3 color;
    float transparency;

} Triangle;



kernel void shaedars(global Fragment* fragment_buffer, global Material* const material_buffer, global Triangle* const triangle_buffer_all){
    uint id = get_global_id(0);
    Fragment frag = fragment_buffer[id];
    uint td = frag.triangle_id;
    Triangle t = triangle_buffer_all[td];

    if (frag.depth == 0) {
        frag.r = 0;
        frag.g = 0;
        frag.b = 0;

        fragment_buffer[id] = frag;
		return; // <-- ignore empty pixels
	}

    //// ---------- Calculate interpolated values ------------- ////
    float u = frag.va;
    float v = frag.vb;
    float w = frag.vc;

    // Get TX coord
    float tx = t.uv0.x*u + t.uv1.x*v + t.uv2.x*w;

    float ty = t.uv0.y*u + t.uv1.y*v + t.uv2.y*w;
    frag.uvx = tx;
    frag.uvy = ty;

    // Get interpolated Normal
    float nx = -(t.n0.x*u + t.n1.x*v + t.n2.x*w);
    float ny = -(t.n0.y*u + t.n1.y*v + t.n2.y*w);
    float nz = -(t.n0.z*u + t.n1.z*v + t.n2.z*w);
    frag.nx = nx;
    frag.ny = ny;
    frag.nz = nz;

    // Get interpolated position
    frag.x = u*t.v0.x + v*t.v1.x + w*t.v2.x;
    frag.y = u*t.v0.y + v*t.v1.y + w*t.v2.y;
    frag.z = u*t.v0.z + v*t.v1.z + w*t.v2.z;










    /// ---------------------------------------------------------- ///
    // TEMP - visualise normals
   /* frag.r = 255 * (frag.nx*0.5 + 0.5);
    frag.g = 255 * (frag.ny*0.5 + 0.5);
    frag.b = 255 * (frag.nz*0.5 + 0.5);*/
    frag.r = 255 * frag.uvx;
    frag.g = 255 * frag.uvy;
    

    // Reset depth
    //frag.depth = 0.0f;

    // Write out result:
    fragment_buffer[id] = frag;
}