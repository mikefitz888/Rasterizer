#include "../Include/RenderUtil.h"

//Honestly got no clue why this is required, compiler whines too much, it should be able to determine this from the .h declarations
cl::Buffer* RENDER::frame_buff;
cl::Buffer* RENDER::triangle_buff;
std::vector<Triangle*> RENDER::triangle_refs;
cl::Device* RENDER::device;
cl::Context* RENDER::context;
cl::CommandQueue* RENDER::queue;
std::vector<cl::Program*> RENDER::programs;
std::map<std::string, cl::Kernel*> RENDER::kernels;

void RENDER::getOCLDevice() {
    //OpenCl
    std::vector<cl::Platform> all_platforms;
    //cl::Platform::get(&all_platforms);
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0) {
        std::cout << " No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Platform default_platform = all_platforms[1];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if (all_devices.size() == 0) {
        std::cout << " No devices found. Check OpenCL installation!\n";
        exit(1);
    }
    device = new cl::Device(all_devices[0]);
    std::cout << "Using platform: " << device->getInfo<CL_DEVICE_NAME>() << "\n";
}

void RENDER::loadOCLKernels() {
    //also create + builds programs
    std::string kernel_names[] = { "rasterizer" };
    std::string start = "kernels/";
    std::string ext = ".cl";
    for (auto n : kernel_names) {
        cl_int err;
        //cl::Program* nProg = new cl::Program(start + n + ext, false, &err);
        std::ifstream t(start + n + ext);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        std::cout << str << "\n";

        cl::Program* nProg = new cl::Program(*context, str, false, &err);
        programs.emplace_back(nProg);
        if (err) {
            printf("Error: %d\n", err);
            while (1);
        }
    }

    unsigned int n = 0;
    for (auto program : programs) {
        cl_int err;
        program->build({ *device });
        cl::Kernel* nKern = new cl::Kernel(*program, kernel_names[n].c_str(), &err);
        if (err) {
            printf("Error: %d\n", err);
            while (1);
        }
        kernels.emplace(kernel_names[n], nKern);
        n++;
    }
}

void RENDER::getOCLContext() {
    context = new cl::Context({ *device });
}

void RENDER::createOCLCommandQueue() {
    cl_int err;
    queue = new cl::CommandQueue(*context, *device, 0, &err);
}

void RENDER::allocateOCLBuffers() {
    frame_buff = new cl::Buffer(*context, CL_MEM_WRITE_ONLY, sizeof(Pixel) * SCREEN_WIDTH * SCREEN_HEIGHT);
    triangle_buff = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(glm::vec3) * 3 * triangle_refs.size());
}

void RENDER::writeTriangles() {
    //May need to modify this to do transfer in multiple steps if memory requirement is too high
    glm::vec3* triangles = new glm::vec3[3 * triangle_refs.size()];

    size_t count = 0;
    for (auto t : triangle_refs) {
        triangles[count++] = t->v0;
        triangles[count++] = t->v1;
        triangles[count++] = t->v2;
    }

    //queue.enqueueWriteBuffer(triangle_buff, CL_TRUE, 0, sizeof(glm::vec3) * 3 * triangle_refs.size(), &(triangles->x)); //Ideally you would include glm/gtc/type_ptr.hpp and use glm::value_ptr, but it just does this anyway
    delete[] triangles;
}

/*
        Public Methods
*/


void RENDER::initialize() {
    //OCL Setup, Add Triangles first
    getOCLDevice();
    getOCLContext();
    createOCLCommandQueue();
    loadOCLKernels();
    allocateOCLBuffers();

    //Write triangles to device
    writeTriangles();
}

void RENDER::addTriangle(Triangle& triangle) {
    triangle_refs.emplace_back(&triangle);
}

void RENDER::renderFrame(Pixel* frame_buffer) {
    unsigned int number_of_triangles = triangle_refs.size();
    unsigned int screen_w = SCREEN_WIDTH;
    kernels["rasterizer"]->setArg(0, sizeof number_of_triangles, &number_of_triangles);
    kernels["rasterizer"]->setArg(1, sizeof screen_w, &screen_w);
    kernels["rasterizer"]->setArg(2, *triangle_buff);
    kernels["rasterizer"]->setArg(3, *frame_buff);

    cl::NDRange offset = cl::NDRange(0, 0);
    cl::NDRange global = cl::NDRange(SCREEN_WIDTH, SCREEN_HEIGHT);
    cl_int err = queue->enqueueNDRangeKernel(*kernels["rasterizer"], NULL, global);
    if (err) {
        printf("Error: %d\n", err);
        while (1);
    }
    queue->enqueueReadBuffer(*frame_buff, false, 0, sizeof(Pixel) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer);
    queue->finish();
}

void RENDER::release() {
    for (auto kern : kernels) {
        delete kern.second;
    }
    kernels.clear();

    for (auto prog : programs) {
        delete prog;
    }
    programs.clear();

    for (auto t : triangle_refs) {
        delete t;
    }
    triangle_refs.clear();

    delete frame_buff;
    delete triangle_buff;
    delete device;
    delete context;
    delete queue;
}