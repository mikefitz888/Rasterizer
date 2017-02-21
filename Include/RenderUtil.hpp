#include "SDLauxiliary.h"

class RENDER {
    struct Pixel {
        //Core pixel data
        uint8_t r, g, b, a;
        unsigned short x, y;
    };
    static Pixel frame_buff[SCREEN_WIDTH * SCREEN_HEIGHT];

    static cl::Device device;
    static cl::Context context;
    static cl::CommandQueue queue;
    static std::vector<cl::Program> programs;
    static std::vector<cl::Kernel> kernels;

    static inline void getOCLDevice() {
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
        device = all_devices[0];
        std::cout << "Using platform: " << device.getInfo<CL_DEVICE_NAME>() << "\n";
    }

    static inline void loadOCLKernels() {
        //also create + builds programs
        string kernel_names[] = {"rasterizer"};
        string start = "kernels/";
        string ext = ".cl";
        for (auto n : kernel_names) {
            programs.emplace_back(start + n + ext);
        }
        
        unsigned int n = 0;
        for (auto program : programs) {
            program.build({ device });
            kernels.emplace_back(program, kernel_names[n++].c_str());
        }
    }

    static inline void getOCLContext() {
        context = cl::Context({device});
    }

    static inline void createOCLCommandQueue() {
        cl_int err;
        queue = cl::CommandQueue(context, device, 0, &err);
    }

public:
    static inline void initialize() {
        getOCLDevice();
        getOCLContext();
        createOCLCommandQueue();
        loadOCLKernels();
    }
    static inline void addTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 color, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2) {
    
    }

    static inline void renderFrame() {
    
    }
};