#ifndef _TEXTURES_H
#define _TEXTURES_H
#include "bitmap_image.hpp"
#include <string>
#include <cstring>
#include <CL/cl.hpp>

typedef struct  {
    unsigned char r, g, b, a;
} Colour;

typedef struct clTexture {
   // int width, height;
    Colour colours[349526]; // Hard-coded for 512*512 plus mipmap levels
} clTexture;

// Texture  (Row major)
class Texture {
    clTexture* texture;
    bitmap_image* bmp;
    cl::Buffer* gpu_mem_ptr;

    void generateMipMapLevels();

public:
    Texture(const std::string& filename, const cl::Context *context, const cl::CommandQueue *queue);
    Colour* getClTexture();
    cl::Buffer* getGPUPtr();
};


#endif