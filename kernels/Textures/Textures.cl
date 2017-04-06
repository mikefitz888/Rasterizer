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
#define ANISOTROPIC_LOD_BIAS 0

// ---------------------------------------------- //


// Colour
typedef struct __attribute__((packed)) {
    uchar r, g, b, a;
} Colour;

inline Colour texture2D(global Colour* tex, float uvx, float uvy) {
    const int width = 512;
    const int height = 512;

    

    // Convert uv to int
    uvx *= width;
    uvy *= height;

    int uvx_i = (int)floor(uvx);
    int uvy_i = (int)floor(uvy);

    // TEXTURE MODE (always assume wrapping)
    //TODO: Improve to avoid negative modulo
    uvx_i = (uvx_i) % width;
    uvy_i = (uvy_i) % height;

    // Sample texture
    // TODO: Incorperate texture filtering
    int sample_index = uvx_i + uvy_i*width;
    if (sample_index < 0 || sample_index >= 512 * 512) {
        printf("SAMPLING INDEX ERROR!! \n"); // Need a more elegant way of handling n
        Colour c;
        return c;
    }
    return tex[sample_index];
}

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
#endif