#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.hpp>
#include <map>
#include <set>
#include "Triangle.h"
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

union ufvec3 {
    glm::vec3 f;
    glm::vec3 i;
    inline ufvec3() {}
};

struct triplet {
    ufvec3 v0, v1, v2;
    //glm::vec3 face_normal, n0, n1, n2;
    unsigned int id;
    inline unsigned int minY() const { return std::min(std::min(v0.i.y, v1.i.y), v2.i.y); }
    inline bool intersections(int y, unsigned int& const c0, unsigned int& const c1) const {
        std::vector<glm::u32vec3> above;
        std::vector<glm::u32vec3> below;
        std::vector<int> intersect_x;

        v0.i.y < y ? below.push_back(v0.i) : above.push_back(v0.i);
        v1.i.y < y ? below.push_back(v1.i) : above.push_back(v1.i);
        v2.i.y < y ? below.push_back(v2.i) : above.push_back(v2.i);
        for (auto& a : above) {
            for (auto& b : below) {
                int c = b.x + (a.x - b.x) * (y - b.y) / (a.y - b.y);
                intersect_x.push_back(c);
            }
        }
        if (intersect_x.size() == 2) {
            c0 = std::min(intersect_x[0], intersect_x[1]);
            c1 = std::max(intersect_x[0], intersect_x[1]);
            return true;
        }
        return false;
    }
};

//TODO: move these to a header file for both cpp and opencl
struct Pixel {
    //Core pixel data
    uint8_t r, g, b, a;
    uint32_t triangle_id;
    float depth;
    //unsigned short x, y;
};

struct Material {
    float r, g, b, a;
};

struct AABB {
    int minX, minY, maxX, maxY;
    glm::vec2 v0, v1;
    glm::ivec2 a;
    float inv_denom, d0, d1, d2;
    size_t triangle_id, offset;
};

class RENDER {
    //static Pixel frame_buff[SCREEN_WIDTH * SCREEN_HEIGHT];
    static cl::Buffer* frame_buff;
    static cl::Buffer* triangle_buff;
    static cl::Buffer* screen_space_buff;
    static cl::Buffer* material_buffer;
    static cl::Buffer* aabb_buffer;

    static std::vector<Triangle*> triangle_refs;
    static triplet* triangles;
    static Material* materials;

    static cl::Device* device;
    static cl::Context* context;
    static cl::CommandQueue* queue;
    static std::vector<cl::Program*> programs;
    static std::map<std::string, cl::Kernel*> kernels;
    static AABB* local_aabb_buff;

    /*
        Utility variables
    */

    static bool scene_changed;

    static void getOCLDevice();

    static void loadOCLKernels();

    static void getOCLContext();

    static void createOCLCommandQueue();

    static void allocateOCLBuffers();

    static void sortTriangles(triplet* first, triplet* last, int msb = 31);

    static void writeTriangles();

public:
    static void initialize();

    static void addTriangle(Triangle& triangle);

    static void renderFrame(Pixel* frame_buffer, glm::vec3 campos);

    static void release();
};

#endif