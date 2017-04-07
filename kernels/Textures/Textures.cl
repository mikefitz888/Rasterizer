#ifndef _TEXTURES_H
#define _TEXTURES_H

// ---------------------------------------------- //
// Mip level constant
/*
    Defines the offset in the texture array where the mip-map level data starts for each level.
    - Mip map levels go from 512x512 to 1x1 dividing by equal powers of 2 each time.
    - Textures should generate a full set of mip-maps for texture filtering.
*/
__constant int _MIP_LV_OFFSETS[10] = { 0, 262144, 327680, 344064, 348160,
                                        349184, 349440, 349504, 349520, 349524 };

// This is the offset to the default LOD selection used in filtering. A higher number will result in more extreme texture filtering
//  - Lowering this value will result in sharper textures, but more non-filtered artifacts such as shimmering will appear
#define ANISOTROPIC_LOD_BIAS 0

// ---------------------------------------------- //
// Fragment
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

    // Gradient values for texture coordinates
    float dudx, dvdx, dudy, dvdy;

} Fragment;

// Colour
typedef struct __attribute__((packed)) {
    uchar r, g, b, a;
} Colour;

// Same function with LOD level specified
inline Colour texture2D_LOD(global Colour* tex, float uvx, float uvy, int LOD_level) {
    // Determine size
    int size = 512>>LOD_level;

    // Convert uv to int
    uvx *= size;
    uvy *= size;

    int uvx_i = (int)floor(uvx);
    int uvy_i = (int)floor(uvy);

    // TEXTURE MODE (always assume wrapping)
    //TODO: Improve to avoid negative modulo
    uvx_i = (uvx_i) % size;
    uvy_i = (uvy_i) % size;

    // Sample texture
    // TODO: Incorperate texture filtering
    int LOD_offset = _MIP_LV_OFFSETS[LOD_level];
    int sample_index = (uvx_i + uvy_i*size) + LOD_offset;
    if (sample_index < LOD_offset || sample_index >= LOD_offset+size*size) {
        printf("SAMPLING INDEX ERROR!! \n"); // Need a more elegant way of handling n
        Colour c;
        return c;
    }
    return tex[sample_index];
}

inline Colour texture2D_NN(global Colour* tex, float uvx, float uvy) {
    return texture2D_LOD(tex, uvx, uvy, 0);
}

// Texture filtering
// - Uses gradient function of fragment

/*
    This function bilinearly interpolates the value of each pixel when a sample is made between pixel areas. 
    It blends the values of the pixel based on the offset ratio between the pixel

*/
inline Colour texture2D_bilinear(global Colour* tex, float uvx, float uvy, Fragment frag) {

    float px = sqrt(frag.dudx*frag.dudx + frag.dvdx*frag.dvdx);
    float py = sqrt(frag.dudy*frag.dudy + frag.dvdy*frag.dvdy);
    float pmax = max(px, py);
    float pmin = min(px, py);

    // - N is the number of samples we should take
    // - L is the LOD mip-map level we should use.
    int N = min((int)ceil(pmax / pmin), 8);
    int L = (int)log2(N / pmax);
    int LOD_level = clamp(9 - L + ANISOTROPIC_LOD_BIAS, 0, 9);

    // Perform calc
    int size = 512 >> LOD_level;

    // Get coords
    uvx = uvx*size - 0.5f;
    uvy = uvy*size - 0.5f;
    int uvx_i = (int)floor(uvx);
    int uvy_i = (int)floor(uvy);

    // Get sampling ratios
    float u_ratio = uvx - (float)uvx_i;
    float v_ratio = uvy - (float)uvy_i;
    float u_opp = 1.0f - u_ratio;
    float v_opp = 1.0f - v_ratio;

    // Wrap coordinates (the + 1073741824 is to avoid the negative mod issue)
    uvx_i = (uvx_i + 1073741824) % size;
    uvy_i = (uvy_i + 1073741824) % size;
    int uvx1_i = (uvx_i + 1 + 1073741824) % size;
    int uvy1_i = (uvy_i + 1 + 1073741824) % size;

    // Sample multiple points
    int LOD_offset = _MIP_LV_OFFSETS[LOD_level];
    Colour c1 = tex[LOD_offset + uvx_i  + uvy_i*size];
    Colour c2 = tex[LOD_offset + uvx1_i + uvy_i*size];
    Colour c3 = tex[LOD_offset + uvx_i  + uvy1_i*size];
    Colour c4 = tex[LOD_offset + uvx1_i + uvy1_i*size];

    // Calculate result colour
    Colour result;
    result.r = (c1.r*u_opp + c2.r*u_ratio)*v_opp + (c3.r*u_opp + c4.r*u_ratio)*v_ratio;
    result.g = (c1.g*u_opp + c2.g*u_ratio)*v_opp + (c3.g*u_opp + c4.g*u_ratio)*v_ratio;
    result.b = (c1.b*u_opp + c2.b*u_ratio)*v_opp + (c3.b*u_opp + c4.b*u_ratio)*v_ratio;
    result.a = (c1.a*u_opp + c2.a*u_ratio)*v_opp + (c3.a*u_opp + c4.a*u_ratio)*v_ratio;
    return result;
}
#endif