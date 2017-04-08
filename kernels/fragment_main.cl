#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

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
    // BILINEAR FILTERING
    // Tmp calculate aniso factor 
    Colour tex_col = texture2D_bilinear(default_tex, frag.uvx, frag.uvy, frag);
    //Colour tex_col = texture2D_LOD(default_tex, frag.uvx, frag.uvy, 0);
    frag.r = tex_col.r;
    frag.g = tex_col.g;
    frag.b = tex_col.b;
    frag.a = 255; // Specularity

    /// ---------------------------------------------------------- ///
    // Merge normal + specular map with fragment buffer
    /*
        By simply modifying the fragment buffer normals and specular values here,
        we can apply normal mapping to all subsequent lighting calculations :)

        Reference:
        https://learnopengl.com/#!Advanced-Lighting/Normal-Mapping
    */
    // Specular
    Colour spec_col = texture2D_bilinear(specular_tex, frag.uvx, frag.uvy, frag);
    frag.a = spec_col.r;

    // Normal mapping
    Colour norm_col = texture2D_bilinear(normal_tex, frag.uvx, frag.uvy, frag);
    float3 normal_t = (float3)(((float)norm_col.r / 255.0f)*2.0f - 1.0f,
                             ((float)norm_col.g / 255.0f)*2.0f - 1.0f,
                             ((float)norm_col.b / 255.0f)*2.0f - 1.0f);
    
    float f = 1.0f / (/*deltaUV1.x*/dudx * /*deltaUV2.y*/dvdy - /*deltaUV2.x*/dvdx * /*deltaUV1.y*/dudy);
    
    float3 edge1, edge2;
    edge1 = (float3)(t.v1.x, t.v1.y, t.v1.z) - (float3)(t.v0.x, t.v0.y, t.v0.z);
    edge2 = (float3)(t.v2.x, t.v2.y, t.v2.z) - (float3)(t.v0.x, t.v0.y, t.v0.z);

    float3 tangent, bitangent;
    tangent.x = f * (dvdy * edge1.x - dudy * edge2.x);
    tangent.y = f * (dvdy * edge1.y - dudy * edge2.y);
    tangent.z = f * (dvdy * edge1.z - dudy * edge2.z);
    tangent = normalize(tangent);

    bitangent.x = f * (-dvdx * edge1.x + dudx * edge2.x);
    bitangent.y = f * (-dvdx * edge1.y + dudx * edge2.y);
    bitangent.z = f * (-dvdx * edge1.z + dudx * edge2.z);
    bitangent = normalize(bitangent);

    mat3 TBN;
    TBN.m00 = tangent.x;
    TBN.m01 = tangent.y;
    TBN.m02 = tangent.z;

    TBN.m10 = bitangent.x;
    TBN.m11 = bitangent.y;
    TBN.m12 = bitangent.z;

    TBN.m20 = frag.nx;
    TBN.m21 = frag.ny;
    TBN.m22 = frag.nz;

    normal_t = normalize(mul3(normal_t, TBN));

    // Assign to frag
    frag.nx = normal_t.x;
    frag.ny = normal_t.y;
    frag.nz = normal_t.z;


    
    // Write out result:
    fragment_buffer[id] = frag;
}