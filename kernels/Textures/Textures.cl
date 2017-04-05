#ifndef _TEXTURES_H
#define _TEXTURES_H

// Colour
typedef struct __attribute__((packed)) {
    uchar r, g, b, a;
} Colour;

// Texture  (Row major)
/*typedef struct __attribute__((packed)) {
    Colour* colours; // 1024*1024
} Texture;*/

// Material
/*typedef struct __attribute__((packed)) {
    Colour* diffuse_tex;
    Colour* normal_tex;
    Colour* specular_tex;
} Material;*/

Colour texture2D(global Colour* tex, float uvx, float uvy) {
    int width = 1024;
    int height = 1024;

    // Convert uv to int
    uvx *= width;
    uvy *= height;

    int uvx_i = (int)uvx;
    int uvy_i = (int)uvy;

    // TEXTURE MODE (always assume wrapping)
    uvx_i = uvx_i % width;
    uvy_i = uvy_i % height;

    // Sample texture
    // TODO: Incorperate texture filtering
   // printf("texture sample: %d, %d \n", uvx_i, uvy_i);
    Colour result = tex[uvx_i + uvy_i*width];

    return result;
}
#endif