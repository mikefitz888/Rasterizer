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
//#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>
#include <iostream>
#include <fstream>
#include <ctime>

union ufvec4 {
    glm::vec4 f;
    glm::vec4 i;
    inline ufvec4() {}
};

struct triplet {
    ufvec4 v0, v1, v2;
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

    // Interpolators
    float va, vb, vc; // Barycentric coordinates

    // Stored information (for effects)
    float depth;
    float x, y, z;
    float nx, ny, nz;
    float uvx, uvy;
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
    static cl::Buffer* ssao_buff;
    static cl::Buffer* ssao_sample_kernel;
    static int sample_count;
    static glm::vec3* ssao_sample_kernel_vals;
    static cl::Buffer* triangle_buff;
    static cl::Buffer* screen_space_buff;
    static cl::Buffer* material_buffer;
    static cl::Buffer* aabb_buffer;
    static cl::Buffer* triangle_buf_alldata;

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

    static void renderFrame(Pixel* frame_buffer, int WIDTH, int HEIGHT, glm::vec3 campos, glm::vec3 camdir);
    static void calculateSSAO(Pixel* out_ssao_buffer, int WIDTH, int HEIGHT, glm::vec3 campos, glm::vec3 camdir);
    static void buildSSAOSampleKernel(int sample_count);
    static void release();
};

#endif