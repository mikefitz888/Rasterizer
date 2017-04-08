#include "kernels/Textures/Textures.cl"

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





kernel void fragment_main(global Fragment* fragment_buffer, 
                          global Triangle* triangle_buffer_all, 
                          global Colour* default_tex, 
                          global Colour* normal_tex,
                          global Colour* specular_tex,
                          int screen_width, 
                          int screen_height){

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
    float3 nml;
    nml.x = -(t.n0.x*u + t.n1.x*v + t.n2.x*w);
    nml.y = -(t.n0.y*u + t.n1.y*v + t.n2.y*w);
    nml.z = -(t.n0.z*u + t.n1.z*v + t.n2.z*w);
    nml = normalize(nml);
    frag.nx = nml.x;
    frag.ny = nml.y;
    frag.nz = nml.z;

    // Get interpolated position
    frag.x = u*t.v0.x + v*t.v1.x + w*t.v2.x;
    frag.y = u*t.v0.y + v*t.v1.y + w*t.v2.y;
    frag.z = u*t.v0.z + v*t.v1.z + w*t.v2.z;

    // Barrier now that interpolation is complete
    fragment_buffer[id] = frag;
#ifdef DEVICE_GPU
    barrier(CLK_GLOBAL_MEM_FENCE); // < This crashes on the CPU openCL for some reason
#endif

    // Calculate gradient information
    Fragment frag_left = fragment_buffer[clamp((int)id - 1, (int)0, (int)(screen_width * screen_height -1))];
    Fragment frag_up = fragment_buffer[clamp((int)id - (int)screen_width, (int)0, (int)(screen_width*screen_height-1))];
    float dudx = frag.uvx - frag_left.uvx;
    float dvdx = frag.uvy - frag_left.uvy;
    float dudy = frag.uvx - frag_up.uvx;
    float dvdy = frag.uvy - frag_up.uvy;

    // Store gradient informationfrag.
    frag.dudx = dudx;
    frag.dvdx = dvdx;
    frag.dudy = dudy;
    frag.dvdy = dvdy;

    // ----------------------------------------------------------- //


    /// ---------------------------------------------------------- ///
    // TEMP - visualise normals
   /* frag.r = 255 * (frag.nx*0.5 + 0.5);
    frag.g = 255 * (frag.ny*0.5 + 0.5);
    frag.b = 255 * (frag.nz*0.5 + 0.5);*/
    

    // TEMP BILINEAR FILTERING
    // Tmp calculate aniso factor 
    Colour tex_col = texture2D_bilinear(default_tex, frag.uvx, frag.uvy, frag);
    //Colour tex_col = texture2D_LOD(default_tex, frag.uvx, frag.uvy, 0);
    frag.r = tex_col.r;
    frag.g = tex_col.g;
    frag.b = tex_col.b;
    frag.a = 255; // Specularity

    /// ---------------------------------------------------------- ///
    // Merge normal + specular map with fragment buffer
    Colour norm_col = texture2D_bilinear(normal_tex, frag.uvx, frag.uvy, frag);
    Colour spec_col = texture2D_bilinear(specular_tex, frag.uvx, frag.uvy, frag);

    frag.a = spec_col.r;

    
  /*  frag.r = frag.x*10.0f;
    frag.g = frag.y*10.0f;
    frag.b = frag.z*10.0f;*/

    // Reset depth
    //frag.depth = 0.0f;

    
    // Write out result:
    fragment_buffer[id] = frag;
}