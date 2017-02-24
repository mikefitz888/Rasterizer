#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.hpp>
#include <map>
#include "Triangle.h"
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <algorithm>

struct Pixel {
    //Core pixel data
    uint8_t r, g, b, a;
    //unsigned short x, y;
};

class RENDER {
    //static Pixel frame_buff[SCREEN_WIDTH * SCREEN_HEIGHT];
    static cl::Buffer* frame_buff;
    static cl::Buffer* triangle_buff;
    static cl::Buffer* screen_space_buff;

    static std::vector<Triangle*> triangle_refs;
    static glm::vec3* triangles;

    static cl::Device* device;
    static cl::Context* context;
    static cl::CommandQueue* queue;
    static std::vector<cl::Program*> programs;
    static std::map<std::string, cl::Kernel*> kernels;

    /*
        Utility variables
    */

    static bool scene_changed;

    static void getOCLDevice();

    static void loadOCLKernels();

    static void getOCLContext();

    static void createOCLCommandQueue();

    static void allocateOCLBuffers();

    static void sortTriangles();

    static void writeTriangles();

public:
    static void initialize();

    static void addTriangle(Triangle& triangle);

    static void renderFrame(Pixel* frame_buffer, glm::vec3 campos);

    static void release();
};

#endif