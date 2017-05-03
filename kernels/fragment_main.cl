#include "kernels/Textures/Textures.cl"
#include "kernels/Textures/Materials.cl"
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

    // Material
    int material_id;

} Triangle;





kernel void fragment_main(global FragmentColour* fragment_buffer_col,
                          global FragmentTData* fragment_buffer_tdata,
                          global FragmentWPos* fragment_buffer_wpos,
                          global FragmentNormal* fragment_buffer_normal,
                          global FragmentTX* fragment_buffer_tex,
                          global FragmentFX* fragment_buffer_fx,

                          global Triangle* triangle_buffer_all, 
                          /*global Colour* default_tex, 
                          global Colour* normal_tex,
                          global Colour* specular_tex,*/
                          global Colour* material_textures,
                          global MaterialProperties* material_properties,
                          int screen_width, 
                          int screen_height){

    uint id = get_global_id(0);

    FragmentColour fragCol   = fragment_buffer_col[id];
    FragmentTData  fragTdata = fragment_buffer_tdata[id];
    FragmentWPos   fragWpos  = fragment_buffer_wpos[id];
    FragmentNormal fragNml   = fragment_buffer_normal[id];
    FragmentTX     fragTX    = fragment_buffer_tex[id];
    FragmentFX     fragFX = fragment_buffer_fx[id];

    uint td = fragTdata.triangle_id;
    Triangle t = triangle_buffer_all[td];

    // Pull material properties
    MaterialProperties mp = material_properties[t.material_id];
    fragFX.glossiness = mp.glossiness;
    

    if (fragTdata.depth == 0) {
        fragCol.r = 0;
        fragCol.g = 0;
        fragCol.b = 0;

        fragment_buffer_col[id] = fragCol;
		return; // <-- ignore empty pixels
	}

    //// ---------- Calculate interpolated values ------------- ////
    float u = fragTdata.va;
    float v = fragTdata.vb;
    float w = fragTdata.vc;

    // Get TX coord
    float tx = t.uv0.x*u + t.uv1.x*v + t.uv2.x*w;

    float ty = t.uv0.y*u + t.uv1.y*v + t.uv2.y*w;
    fragTX.uvx = tx;
    fragTX.uvy = ty;

    // Get interpolated Normal
    float3 nml;
    nml.x = -(t.n0.x*u + t.n1.x*v + t.n2.x*w);
    nml.y = -(t.n0.y*u + t.n1.y*v + t.n2.y*w);
    nml.z = -(t.n0.z*u + t.n1.z*v + t.n2.z*w);
    nml = normalize(nml);
    fragNml.nx = nml.x;
    fragNml.ny = nml.y;
    fragNml.nz = nml.z;

    // Get interpolated position
    fragWpos.x = u*t.v0.x + v*t.v1.x + w*t.v2.x;
    fragWpos.y = u*t.v0.y + v*t.v1.y + w*t.v2.y;
    fragWpos.z = u*t.v0.z + v*t.v1.z + w*t.v2.z;

    // Barrier now that interpolation is complete
    //  - Optimisation, only need to store the texture coordinates now
    //      - as thats all that is needed by neighbouring cells.
    //fragment_buffer_col[id] = fragCol;
    //fragment_buffer_tdata[id] = fragTdata;
    fragment_buffer_wpos[id] = fragWpos;
    //fragment_buffer_normal[id] = fragNml;
    fragment_buffer_tex[id] = fragTX;

#ifdef DEVICE_GPU
    barrier(CLK_GLOBAL_MEM_FENCE); // < This crashes on the CPU openCL for some reason
#endif

    // Calculate gradient information
    FragmentTX frag_left = fragment_buffer_tex[clamp((int)id - 1, (int)0, (int)(screen_width * screen_height -1))];
    FragmentTX frag_up   = fragment_buffer_tex[clamp((int)id - (int)screen_width, (int)0, (int)(screen_width*screen_height-1))];
    float dudx = fragTX.uvx - frag_left.uvx;
    float dvdx = fragTX.uvy - frag_left.uvy;
    float dudy = fragTX.uvx - frag_up.uvx;
    float dvdy = fragTX.uvy - frag_up.uvy;

    // Store gradient informationfrag.
    fragTX.dudx = dudx;
    fragTX.dvdx = dvdx;
    fragTX.dudy = dudy;
    fragTX.dvdy = dvdy;

    // ----------------------------------------------------------- //


    /// ---------------------------------------------------------- ///
    // BILINEAR FILTERING
    // Tmp calculate aniso factor 
    Colour tex_col = texture2D_bilinear(material_textures, t.material_id, 0, fragTX.uvx, fragTX.uvy, fragTX);
    //Colour tex_col = texture2D_LOD(materials, 0, 0, fragTX.uvx, fragTX.uvy, 0);
    fragCol.r = tex_col.r;
    fragCol.g = tex_col.g;
    fragCol.b = tex_col.b;
    fragCol.a = 255; // Specularity

    /// ---------------------------------------------------------- ///
    // Merge normal + specular map with fragment buffer
    /*
        By simply modifying the fragment buffer normals and specular values here,
        we can apply normal mapping to all subsequent lighting calculations :)

        Reference:
        https://learnopengl.com/#!Advanced-Lighting/Normal-Mapping
    */
    // Specular
    Colour spec_col = texture2D_bilinear(material_textures, t.material_id, 2, fragTX.uvx, fragTX.uvy, fragTX);
   // Colour spec_col = texture2D_LOD(materials, 0, 2, fragTX.uvx, fragTX.uvy, 0);
    fragCol.a = spec_col.r*mp.specularity;
    fragFX.reflection_val = mp.reflectivity*(spec_col.r/255.0f);

    // Normal mapping
    Colour norm_col = texture2D_bilinear(material_textures, t.material_id, 1, fragTX.uvx, fragTX.uvy, fragTX);
    //Colour norm_col = texture2D_LOD(materials, 0, 1, fragTX.uvx, fragTX.uvy, 0);
    float3 normal_t = (float3)(((float)norm_col.r / 255.0f)*2.0f - 1.0f,
                             ((float)norm_col.g / 255.0f)*2.0f - 1.0f,
                             ((float)norm_col.b / 255.0f)*2.0f - 1.0f);
    
    /*float f = 1.0f / (dudx * dvdy - dvdx * dudy);
    
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

    TBN.m20 = fragNml.nx;
    TBN.m21 = fragNml.ny;
    TBN.m22 = fragNml.nz;

    normal_t = normalize(mul3(normal_t, TBN));*/

    // Perform Normal Perturbation
    FragmentWPos frag_left_w = fragment_buffer_wpos[clamp((int)id - 1, (int)0, (int)(screen_width * screen_height - 1))];
    FragmentWPos frag_up_w = fragment_buffer_wpos[clamp((int)id - (int)screen_width, (int)0, (int)(screen_width*screen_height - 1))];
    float dxdx = fragWpos.x - frag_left_w.x;
    float dydx = fragWpos.y - frag_left_w.y;
    float dzdx = fragWpos.z - frag_left_w.z;

    float dxdy = fragWpos.x - frag_up_w.x;
    float dydy = fragWpos.y - frag_up_w.y;
    float dzdy = fragWpos.z - frag_up_w.z;

    float3 dp1 = (float3)(dxdx, dydx, dzdx);
    float3 dp2 = (float3)(dxdy, dydy, dzdy);
    float2 duv1 = (float2)(dudx, dvdx);//ddx(uv);
    float2 duv2 = (float2)(dudy, dvdy);

    /// solve the linear system
    float3 dp2perp = cross(dp2, nml);
    float3 dp1perp = cross(nml, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale - invariant frame
    float invmax = rsqrt(max(dot(T, T), dot(B, B)));

    // build TBN matrix
    T = T * invmax;
    B = B * invmax;
    float3 N = nml;

    mat3 TBN;
    TBN.m00 = T.x;
    TBN.m01 = T.y;
    TBN.m02 = T.z;

    TBN.m10 = B.x;
    TBN.m11 = B.y;
    TBN.m12 = B.z;

    TBN.m20 = N.x;
    TBN.m21 = N.y;
    TBN.m22 = N.z;

    normal_t = normalize(mul3(normal_t, TBN));



    // Assign to frag
    fragNml.nx = normal_t.x*mp.offset_strength + fragNml.nx*(1.0f- mp.offset_strength);
    fragNml.ny = normal_t.y*mp.offset_strength + fragNml.ny*(1.0f - mp.offset_strength);
    fragNml.nz = normal_t.z*mp.offset_strength + fragNml.nz*(1.0f - mp.offset_strength);


    
    // Write out result:
    //fragment_buffer[id] = frag;
    fragment_buffer_col[id] = fragCol;
    //fragment_buffer_wpos[id] = fragWpos;
    fragment_buffer_normal[id] = fragNml;
    fragment_buffer_tex[id] = fragTX;
    fragment_buffer_fx[id] = fragFX;
}