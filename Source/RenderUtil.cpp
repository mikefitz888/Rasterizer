#include "../Include/RenderUtil.h"

//Honestly got no clue why this is required, compiler whines too much, it should be able to determine this from the .h declarations
cl::Buffer* RENDER::triangle_buff;
cl::Buffer* RENDER::triangle_buf_alldata;
cl::Buffer* RENDER::screen_space_buff;
cl::Buffer* RENDER::aabb_buffer;
cl::Buffer* RENDER::material_buffer;
cl::Buffer* RENDER::material_buffer_properties;

cl::Buffer* RENDER::ssao_sample_kernel;
int RENDER::sample_count = 0;
glm::vec3* RENDER::ssao_sample_kernel_vals;

std::vector<Triangle*> RENDER::triangle_refs;
triplet* RENDER::triangles;
AABB* RENDER::local_aabb_buff;

cl::Device* RENDER::device;
cl::Context* RENDER::context;
cl::CommandQueue* RENDER::queue;
std::vector<cl::Program*> RENDER::programs;
std::map<std::string, cl::Kernel*> RENDER::kernels;

// Texutres
Texture* default_tex;
Texture* normal_map;
Texture* specular_map;


// Material array
Material materials[MaterialType::__MATERIALS_MAX];


bool RENDER::scene_changed = false;
bool USING_GPU = false;

void RENDER::getOCLDevice() {
    //OpenCl
    std::vector<cl::Platform> all_platforms;
    //cl::Platform::get(&all_platforms);
    cl::Platform::get(&all_platforms);

    //cl::Platform default_platform;
    //cl::Platform::get(&default_platform);

    std::cout << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << all_platforms.size() << " Platforms found!" << std::endl;
    int platform_id = 0;
    if (all_platforms.size() == 0) {
        std::cout << " No platforms found. Check OpenCL installation!\n";
        exit(1);
    } else {
        
        std::cout << "Multiple platforms available, please choose one: " << std::endl;
        for (int i = 0; i < all_platforms.size(); i++) {
            std::cout << i << " - " << all_platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;
        }

        int a = 0;
        std::cin >> a;

        if (a >= 0 && a < all_platforms.size()) {
            platform_id = a;
        } else {
            std::cout << "Invalid platform selection!" << std::endl;
            exit(-1);
        }
    }

    cl::Platform default_platform = all_platforms[platform_id];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if (all_devices.size() == 0) {

        // Check for CPU devices
        default_platform.getDevices(CL_DEVICE_TYPE_CPU, &all_devices);
        
        if (all_devices.size() == 0) {
            std::cout << " No devices found. Check OpenCL installation!\n";
            exit(1);
        }
        std::cout << "No GPU found, running on CPU!" << std::endl;
    } else {
        USING_GPU = true;
    }
    device = new cl::Device(all_devices[0]);
    std::cout << "Using device: " << device->getInfo<CL_DEVICE_NAME>() << "\n";

    std::cout << "PRIVATE MEM: " << device->getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>() << std::endl;
}

void RENDER::loadOCLKernels() {
    //also create + builds programs
    std::vector<std::string> kernel_names = {   "rasterizer", 
                                                "projection", 
                                                "fragment_main", 
                                                "aabb", 
                                                "shader_post_ssao", 
                                                "shader_directional_light_shadow",
                                                "shader_point_light",
                                                "accumulate_buffers"};
    std::string start = "kernels/";
    std::string ext = ".cl";
    for (auto n : kernel_names) {
        cl_int err;
        //cl::Program* nProg = new cl::Program(start + n + ext, false, &err);
        std::ifstream t(start + n + ext);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        //std::cout << str << "\n";

        cl::Program* nProg = new cl::Program(*context, str, false, &err);
        programs.emplace_back(nProg);
        if (err) {
            printf("%d Error: %d\n", __LINE__, err);
            while (1);
        }
    }

    unsigned int n = 0;
    for (auto program : programs) {
        cl_int err;
        char options[128];
        if (USING_GPU) {
            sprintf(options, "-D SCREEN_WIDTH=%d -D SCREEN_HEIGHT=%d -D DEVICE_GPU=1", SCREEN_WIDTH, SCREEN_HEIGHT);
        } else {
            sprintf(options, "-D SCREEN_WIDTH=%d -D SCREEN_HEIGHT=%d -D DEVICE_CPU=1", SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        program->build({ *device }, options);
        

        cl::Kernel* nKern = new cl::Kernel(*program, kernel_names[n].c_str(), &err);
        if (err) {  

            std::ofstream myfile;
            myfile.open("kernel compile errors.txt");
            myfile << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device);
            myfile.close();

            std::cout << "------------------ COMPILE ERRORS ------------------" << std::endl;
            std::cout << " Error building: " << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device) << std::endl;
            std::cout << "-----------------------------------------------------" << std::endl;
            printf("%d Error: %d, building %s\n", __LINE__, err, kernel_names[n].c_str());
            while (1);
        } else {
            std::ofstream myfile;
            myfile.open("kernel compile errors.txt");
            myfile << "Compilation Successful!" << std::endl;
            myfile.close();
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
    //TODO: move these to a map for better organisation
    triangle_buff = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(triplet) * triangle_refs.size());
    triangle_buf_alldata = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(Triangle) * triangle_refs.size());

    screen_space_buff = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(triplet) * triangle_refs.size());
    aabb_buffer = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(AABB) * triangle_refs.size());
    
    // LOAD TEXTURES:
    //std::string str = "Resource/T1.bmp";
    default_tex  = new Texture("Resources/T1.bmp", context, queue);
    normal_map   = new Texture("Resources/N1.bmp", context, queue);
    specular_map = new Texture("Resources/S1.bmp", context, queue);


    Texture* cargo_tex_diffuse  = new Texture("Resources/Cargo_diffuse.bmp", context, queue);
    Texture* cargo_tex_normal   = new Texture("Resources/Cargo_normal.bmp", context, queue);
    Texture* cargo_tex_specular = new Texture("Resources/Cargo_spec.bmp", context, queue);



    ///// ---------------- PREPARE MATERIALS ---------------- 

    // Tiled floor
    materials[MaterialType::TILED_FLOOR].diffuse_texture   = default_tex;
    materials[MaterialType::TILED_FLOOR].normalmap_texture = normal_map;
    materials[MaterialType::TILED_FLOOR].specular_texture  = specular_map;

    materials[MaterialType::TILED_FLOOR].specularity  = 1.0f;
    materials[MaterialType::TILED_FLOOR].glossiness   = 1.0f;
    materials[MaterialType::TILED_FLOOR].reflectivity = 0.0f;

    // Cargo Metal
    materials[MaterialType::CARGO_METAL].diffuse_texture = cargo_tex_diffuse;
    materials[MaterialType::CARGO_METAL].normalmap_texture = cargo_tex_normal;
    materials[MaterialType::CARGO_METAL].specular_texture = cargo_tex_specular;

    materials[MaterialType::CARGO_METAL].specularity = 0.5f;
    materials[MaterialType::CARGO_METAL].glossiness = 1.0f;
    materials[MaterialType::CARGO_METAL].reflectivity = 1.0f;


    // Allocate Materials buffer
    material_buffer            = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(Colour) * 349526 * 3 * MaterialType::__MATERIALS_MAX);
    material_buffer_properties = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(GPUMaterial) * MaterialType::__MATERIALS_MAX);

    std::cout << "MAT BUFF: " << material_buffer << std::endl;
}

void RENDER::writeTriangles() {
    //May need to modify this to do transfer in multiple steps if memory requirement is too high
    triangles = new triplet[triangle_refs.size()];

    
    for (int i = 0; i < triangle_refs.size(); i++) {
        auto& t = triangle_refs[i];

        triangles[i].v0.f = glm::vec4(t->v0.x, t->v0.y, t->v0.z, 1.0f);
        triangles[i].v1.f = glm::vec4(t->v1.x, t->v1.y, t->v1.z, 1.0f);
        triangles[i].v2.f = glm::vec4(t->v2.x, t->v2.y, t->v2.z, 1.0f);
        triangles[i].id = i;
        //printf("%f %f %f, %f %f %f, %f %f %f\n", t->v0.x, t->v0.y, t->v0.z, t->v1.x, t->v1.y, t->v1.z, t->v2.x, t->v2.y, t->v2.z);
    }
    //printf("%p %p %p, %d\n", &(triangles[0].v0.f.x), &(triangles[0].v0.f.y), &(triangles[0].v0.f.z), sizeof(triplet));
    scene_changed = true;

    cl_int err = queue->enqueueWriteBuffer(*triangle_buff, CL_TRUE, 0, sizeof(triplet) * triangle_refs.size(), triangles);
    if (err != 0) {
        printf("No. of Triangles = %d @ %p", triangle_refs.size(), triangles);
        printf("ERROR %d %d\n", __LINE__, err);
        while (1);
    }

    //// -------------- COPY MATERIAL BUFFER TO GPU ------------------------ //
    // Prepare material buffer
    Colour *materials_all = new Colour[349526 * 3* MaterialType::__MATERIALS_MAX];
    GPUMaterial *material_properties_all = new GPUMaterial[MaterialType::__MATERIALS_MAX];

    for (int i = 0; i < MaterialType::__MATERIALS_MAX; i++) {
        materials[i].prepareGPUMaterial();
        material_properties_all[i] = materials[i].gpumaterial;

        if (materials[i].diffuse_texture != nullptr) {
            memcpy(&materials_all[(349526 * 3)*i], materials[i].diffuse_texture->getClTexture(), sizeof(Colour) * 349526);
        }
        if (materials[i].normalmap_texture != nullptr) {
            memcpy(&materials_all[(349526 * 3)*i + 349526], materials[i].normalmap_texture->getClTexture(), sizeof(Colour) * 349526);
        }
        if (materials[i].specular_texture != nullptr) {
            memcpy(&materials_all[(349526 * 3)*i + 349526 * 2], materials[i].specular_texture->getClTexture(), sizeof(Colour) * 349526);
        }
    }
    std::cout << "Copy material buffer to GPU" << std::endl;
    err = queue->enqueueWriteBuffer(*material_buffer, CL_TRUE, 0, sizeof(Colour)*349526*3* MaterialType::__MATERIALS_MAX, materials_all);
    if (err != 0) {
        printf("ERROR %d %d\n", __LINE__, err);
        while (1);
    }
    queue->finish();
    err = queue->enqueueWriteBuffer(*material_buffer_properties, CL_TRUE, 0, sizeof(GPUMaterial) * MaterialType::__MATERIALS_MAX, material_properties_all);
    if (err != 0) {
        printf("ERROR %d %d\n", __LINE__, err);
        while (1);
    }
    queue->finish();
    std::cout << "Copied material buffer to GPU" << std::endl;
    // ---------------------------------------------------------------------- //


    Triangle* triangles_FULL = new Triangle[triangle_refs.size()];
    for (int i = 0; i < triangle_refs.size(); i++) {
        triangles_FULL[i] = *triangle_refs[i];
    }
    err = queue->enqueueWriteBuffer(*triangle_buf_alldata, CL_TRUE, 0, sizeof(Triangle) * triangle_refs.size(), triangles_FULL);
    if (err != 0) {
        printf("ERROR %d %d\n", __LINE__, err);
        while (1);
    }
}

//msb-radix sort, based off rosetta stone code. Improved with c++11 lambda notation. Is tested, appears fine
void RENDER::sortTriangles(triplet* first, triplet* last, int msb) {
    if (first != last && msb >= 0) {
        triplet *mid = std::partition(first, last, [msb](triplet& t) -> bool {
            return msb == 31 ?  t.minY() < 0 : !(t.minY() & (1 << msb));
        });
        msb--; // decrement most-significant-bit
        sortTriangles(first, mid, msb); // sort left partition
        sortTriangles(mid, last, msb); // sort right partition
    }
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
    kernels["projection"]->setArg(0, *triangle_buff);
    kernels["projection"]->setArg(1, *screen_space_buff);

    local_aabb_buff = new AABB[triangle_refs.size()];
}

void RENDER::addTriangle(Triangle& triangle) {
    triangle_refs.emplace_back(&triangle);
}
void RENDER::renderFrame(FrameBuffer* frame_buffer, glm::vec3 campos, glm::vec3 camdir) {

    int WIDTH = frame_buffer->getWidth();
    int HEIGHT = frame_buffer->getHeight();

    float rendered_triangle_count = 0;

    //Project Triangles to screen-space
    float campos2[] = { campos.x, campos.y, campos.z };

    // Camera Properties
    float znear = 0.5f;
    float zfar = 250.0f;
    float FOV = 70.0f;
    float aspect = (float)WIDTH / (float)HEIGHT;

    // Construct matrices
    glm::mat4 VIEW_MATRIX       = glm::lookAt(campos, campos + camdir, glm::vec3(0, 1, 0));
    glm::mat4 PROJECTION_MATRIX = glm::perspective(FOV*glm::pi<float>() / 180.0f, -((float)WIDTH / (float)HEIGHT), znear, zfar);
    glm::mat4 CLIP_MATRIX       = glm::mat4((float)WIDTH / 2.0f, 0, 0, 0, 0, (float)HEIGHT / 2.0f, 0, 0, 0, 0, 0.5f, 0, (float)WIDTH / 2.0f, (float)HEIGHT / 2.0f, 0.5, 1);
    //CLIP_MATRIX = glm::transpose(CLIP_MATRIX);

    //kernel void projection(global float3* const vertices, const float3 campos)
    kernels["projection"]->setArg(2, sizeof(cl_float)*16, glm::value_ptr(VIEW_MATRIX));
    kernels["projection"]->setArg(3, sizeof(cl_float)*16, glm::value_ptr(PROJECTION_MATRIX));
    kernels["projection"]->setArg(4, sizeof(cl_float)*16, glm::value_ptr(CLIP_MATRIX));
    
    float sw = (float)WIDTH;
    float sh = (float)HEIGHT;
    kernels["projection"]->setArg(5, sizeof(float), &sw);
    kernels["projection"]->setArg(6, sizeof(float), &sh);

    unsigned int number_of_triangles = triangle_refs.size();

    if(VERBOSE) std::cout << "NUMBER OF TRIANGLES: " << number_of_triangles << std::endl;

    //Converts world space triangles to screen space
    //The z value of each "2D" triangle is used to retain the depth information from world space (not correctly implemented yet)
    //This section only runs if the "triangle" buffer has been written to after this section has last been run
    cl_int err = 0;

    clock_t begin = clock();
    /// PROJECTION STAGE (GPU) ---------------------------------------------------------------------------------------------------//
    const cl::NDRange offset = cl::NDRange(0);
    const cl::NDRange global = cl::NDRange(number_of_triangles);
    err = queue->enqueueNDRangeKernel(*kernels["projection"], offset, global);
    queue->finish();
    if (err) { printf("%d Error: (Projection kernel) %d\n", __LINE__, err); while (1); }

    // Dont need to copy this if running rasterization step
    queue->enqueueReadBuffer(*screen_space_buff, CL_TRUE, 0, sizeof(triplet) * triangle_refs.size(), triangles);
    queue->finish();



    clock_t end = clock();
    double elapsed_secs = 1000*double(end - begin) / CLOCKS_PER_SEC;
    if(VERBOSE) std::cout << "Projection stage: " << elapsed_secs << "ms" << std::endl;
    ///---------------------------------------------------------------------------------------------------------------------//

    /// AABB STAGE (GPU) ---------------------------------------------------------------------------------------------------//
 /*   kernels["rasterizer"]->setArg(0, *aabb_buffer);
    kernels["rasterizer"]->setArg(1, *screen_space_buff);

    err = queue->enqueueNDRangeKernel(*kernels["rasterizer"], NULL, global);
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }

    // Read AABB data from the rasterization step
    queue->enqueueReadBuffer(*aabb_buffer, CL_TRUE, 0, sizeof(AABB) * triangle_refs.size(), local_aabb_buff);*/
    ///---------------------------------------------------------------------------------------------------------------------//
    

    // AABB step CPU
    clock_t begin2 = clock();
#pragma omp parallel for
    for (int i = 0; i < triangle_refs.size(); i++) {
        triplet s = triangles[i];

        bool skip = false;

        // Naive depth clipping
        ufvec4 c0, c1, c2, c3;
        int isQuad = 0;
        /*if (s.v0.f.z <= 0.0 || s.v1.f.z <= 0.0 || s.v2.f.z <= 0.0) {
            skip = true;
        }*/
        const bool b1 = s.v0.f.z <= znear;
        const bool b2 = s.v1.f.z <= znear;
        const bool b3 = s.v2.f.z <= znear;
        if (b1 && b2 && b3) { //all behind
            skip = true;
        }else if(b1 ? (b2 || b3) : (b2 && b3)){ //2 behind
            if (!b1) {
                c0 = s.v0;
                c1 = s.v0; c1.f += (s.v1.f - s.v0.f) * ((c0.f.z - znear) / (c0.f.z - c1.f.z)); //Intersection 1
                c2 = s.v0; c2.f += (s.v2.f - s.v0.f) * ((c0.f.z - znear) / (c0.f.z - c2.f.z)); //Intersection 2
            }
            else if (!b2) {
                c1 = s.v1;
                c0 = s.v1; c0.f += (s.v0.f - s.v1.f) * ((c1.f.z - znear) / (c1.f.z - c0.f.z)); //Intersection 1
                c2 = s.v1; c2.f += (s.v2.f - s.v1.f) * ((c1.f.z - znear) / (c1.f.z - c2.f.z)); //Intersection 2
            }
            else if (!b3) {
                c2 = s.v2;
                c1 = s.v2; c1.f += (s.v1.f - s.v2.f) * ((c2.f.z - znear) / (c2.f.z - c1.f.z)); //Intersection 1
                c0 = s.v2; c0.f += (s.v0.f - s.v2.f) * ((c2.f.z - znear) / (c2.f.z - c0.f.z)); //Intersection 2
            }
        }else if (!(!b1 && !b2 && !b3)) { //1 behind
            isQuad = 1;
            skip = true;
            // c0 + c3 are always the intersection points
            if (b1) {
                c1 = s.v1;
                c2 = s.v2;
                c0.f = c1.f + (s.v0.f - c1.f) * ((c1.f.z - znear) / (c1.f.z - s.v0.f.z));
                c3.f = c2.f + (s.v0.f - c2.f) * ((c2.f.z - znear) / (c2.f.z - s.v0.f.z));
            }
            else if (b2) {
                c1 = s.v0;
                c2 = s.v2;
                c0.f = c1.f + (s.v1.f - c1.f) * ((c1.f.z - znear) / (c1.f.z - s.v1.f.z));
                c3.f = c2.f + (s.v1.f - c2.f) * ((c2.f.z - znear) / (c2.f.z - s.v1.f.z));
                /*c0 = s.v0;
                c2 = s.v2;
                c1.f = c0.f + (s.v1.f - c0.f) * ((c0.f.z - znear) / (c0.f.z - s.v1.f.z));
                c3.f = c2.f + (s.v1.f - c2.f) * ((c2.f.z - znear) / (c2.f.z - s.v1.f.z));*/
            }
            else if (b3) {
                c1 = s.v0;
                c2 = s.v1;
                c0.f = c1.f + (s.v2.f - c1.f) * ((c1.f.z - znear) / (c1.f.z - s.v2.f.z));
                c3.f = c2.f + (s.v2.f - c2.f) * ((c2.f.z - znear) / (c2.f.z - s.v2.f.z));
                /*c0 = s.v0;
                c1 = s.v1;
                c2.f = c0.f + (s.v2.f - c0.f) * ((c0.f.z - znear) / (c0.f.z - s.v2.f.z));
                c3.f = c1.f + (s.v2.f - c1.f) * ((c1.f.z - znear) / (c1.f.z - s.v2.f.z));*/
            }
        }
        else {
            c0 = s.v0;
            c1 = s.v1;
            c2 = s.v2;
        }

        // Backface culling
        glm::vec3 cam_to_face = triangle_refs[i]->v0 - campos;
        if (glm::dot(triangle_refs[i]->normal, cam_to_face) <= 0.0f) {
            skip = true;
        }

        /// Rasterizer v2 (Half-space, chunked) ---------------------- ///
        /// http://forum.devmaster.net/t/advanced-rasterization/6145   ///
        if (!skip) {
            rendered_triangle_count++;
            glm::ivec2 a(s.v0.i.x, s.v0.i.y);
            glm::ivec2 b(s.v1.i.x, s.v1.i.y);
            glm::ivec2 c(s.v2.i.x, s.v2.i.y);
            const auto v0 = (b - a);
            const auto v1 = (c - a);
            const float inv_denom = 1.f / (v0.x * v1.y - v1.x * v0.y);
            const float v0x_inv_d = v0.x * inv_denom;
            const float v1x_inv_d = v1.x * inv_denom;
            const float v0y_inv_d = v0.y * inv_denom;
            const float v1y_inv_d = v1.y * inv_denom;

            do {
                a = glm::ivec2(c0.i.x, c0.i.y);
                b = glm::ivec2(c1.i.x, c1.i.y);
                c = glm::ivec2(c2.i.x, c2.i.y);

                a <<= 4; b <<= 4; c <<= 4;

                //Delta values
                const auto Dab(a - b);
                const auto Dbc(b - c);
                const auto Dca(c - a);

                //Fixed-point deltas
                const auto FDab = Dab << 4;
                const auto FDbc = Dbc << 4;
                const auto FDca = Dca << 4;

                //Bounding box
                int minX = glm::clamp((glm::min(a.x, glm::min(b.x, c.x)) + 0xF) >> 4, 0, WIDTH - 1);
                int maxX = glm::clamp((glm::max(a.x, glm::max(b.x, c.x)) + 0xF) >> 4, 0, WIDTH - 1);
                int minY = glm::clamp((glm::min(a.y, glm::min(b.y, c.y)) + 0xF) >> 4, 0, HEIGHT - 1);
                int maxY = glm::clamp((glm::max(a.y, glm::max(b.y, c.y)) + 0xF) >> 4, 0, HEIGHT - 1);

                //Blocksize (must be 2^n)
                constexpr int q = 8;

                //start in corner of block
                minX &= ~(q - 1);
                minY &= ~(q - 1);

                //Half-edge constants-ish
                int Ca = Dab.y * a.x - Dab.x * a.y;
                int Cb = Dbc.y * b.x - Dbc.x * b.y;
                int Cc = Dca.y * c.x - Dca.x * c.y;

                //Adjust for fill convention
                if (Dab.y < 0 || (Dab.y == 0 && Dab.x > 0)) Ca++;
                if (Dbc.y < 0 || (Dbc.y == 0 && Dbc.x > 0)) Cb++;
                if (Dca.y < 0 || (Dca.y == 0 && Dca.x > 0)) Cc++;

                //Iterate over chunks
                for (int y = minY; y < maxY; y += q) {
                    bool complete = false;
                    for (int x = minX; x < maxX; x += q) {
                        //Corners
                        const int x0 = x << 4;
                        const int x1 = (x + q - 1) << 4;
                        const int y0 = y << 4;
                        const int y1 = (y + q - 1) << 4;

                        //Half-space functions
                        const bool a00 = Ca + Dab.x * y0 - Dab.y * x0 > 0;
                        const bool a10 = Ca + Dab.x * y0 - Dab.y * x1 > 0;
                        const bool a01 = Ca + Dab.x * y1 - Dab.y * x0 > 0;
                        const bool a11 = Ca + Dab.x * y1 - Dab.y * x1 > 0;
                        int a_ = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);

                        const bool b00 = Cb + Dbc.x * y0 - Dbc.y * x0 > 0;
                        const bool b10 = Cb + Dbc.x * y0 - Dbc.y * x1 > 0;
                        const bool b01 = Cb + Dbc.x * y1 - Dbc.y * x0 > 0;
                        const bool b11 = Cb + Dbc.x * y1 - Dbc.y * x1 > 0;
                        int b_ = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);

                        const bool c00 = Cc + Dca.x * y0 - Dca.y * x0 > 0;
                        const bool c10 = Cc + Dca.x * y0 - Dca.y * x1 > 0;
                        const bool c01 = Cc + Dca.x * y1 - Dca.y * x0 > 0;
                        const bool c11 = Cc + Dca.x * y1 - Dca.y * x1 > 0;
                        int c_ = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);

                        //Skip block if outside triangle
                        if (a_ == 0x0 || b_ == 0x0 || c_ == 0x0) {
                            if (complete) x = maxX; //The remainder of the chunks are non-covered
                            continue;
                        }
                        complete = true; //The next non-covered chunk will signal the rest are non-covered

                        //Accept whole block when fully covered
                        if (a_ == 0xF && b_ == 0xF && c_ == 0xF) {
                            for (int iy = y; iy < y + q; iy++) {
                                for (int ix = x; ix < x + q; ix++) {
                                    glm::ivec2 v2 = glm::ivec2(ix, iy) - glm::ivec2(s.v0.i.x, s.v0.i.y);
                                    float b = (v2.x * v1y_inv_d - v1x_inv_d * v2.y);
                                    float c = (v0x_inv_d * v2.y - v2.x * v0y_inv_d);
                                    float a = 1.0f - b - c;

                                    // Correct barycentric coords for perspective:
                                    float zf = a / s.v0.f.z + b / s.v1.f.z + c / s.v2.f.z;
                                    a /= (s.v0.f.z*zf);
                                    b /= (s.v1.f.z*zf);
                                    c /= (s.v2.f.z*zf);

                                    // Perspective-correct depth;
                                    float depth = (a*s.v0.f.z + b*s.v1.f.z + c*s.v2.f.z);

                                    const auto tdata = frame_buffer->getCPUBuffer_tdata() + ix + iy * WIDTH;
                                    if ((tdata->depth == 0 || depth < tdata->depth) && depth > znear && depth < zfar) {
                                        // Store triangle
                                        tdata->triangle_id = i;
                                        // Store barycentric coordinates
                                        tdata->va = a;
                                        tdata->vb = b;
                                        tdata->vc = c;

                                        // Write interpolated depth
                                        tdata->depth = depth;
                                    }
                                }
                            }
                        }
                        else {
                            //Partially covered
                            int CYa = Ca + Dab.x * y0 - Dab.y * x0;
                            int CYb = Cb + Dbc.x * y0 - Dbc.y * x0;
                            int CYc = Cc + Dca.x * y0 - Dca.y * x0;

                            for (int iy = y; iy < y + q; iy++) {
                                int CXa = CYa, CXb = CYb, CXc = CYc;
                                for (int ix = x; ix < x + q; ix++) {
                                    if (CXa > 0 && CXb > 0 && CXc > 0) {
                                        glm::ivec2 v2 = glm::ivec2(ix, iy) - glm::ivec2(s.v0.i.x, s.v0.i.y);
                                        float b = (v2.x * v1y_inv_d - v1x_inv_d * v2.y);
                                        float c = (v0x_inv_d * v2.y - v2.x * v0y_inv_d);
                                        float a = 1.0f - b - c;

                                        // Correct barycentric coords for perspective:
                                        float zf = a / s.v0.f.z + b / s.v1.f.z + c / s.v2.f.z;
                                        a /= (s.v0.f.z*zf);
                                        b /= (s.v1.f.z*zf);
                                        c /= (s.v2.f.z*zf);

                                        // Perspective-correct depth;
                                        float depth = (a*s.v0.f.z + b*s.v1.f.z + c*s.v2.f.z);

                                        const auto tdata = frame_buffer->getCPUBuffer_tdata() + ix + iy * WIDTH;
                                        if ((tdata->depth == 0 || depth < tdata->depth) && depth > znear && depth < zfar) {
                                            // Store triangle
                                            tdata->triangle_id = i;
                                            // Store barycentric coordinates
                                            tdata->va = a;
                                            tdata->vb = b;
                                            tdata->vc = c;

                                            // Write interpolated depth
                                            tdata->depth = depth;
                                        }
                                    }
                                    CXa -= FDab.y;
                                    CXb -= FDbc.y;
                                    CXc -= FDca.y;
                                }
                                CYa += FDab.x;
                                CYb += FDbc.x;
                                CYc += FDca.x;
                            }
                        }
                    }
                }

                c1 = c3;
            } while (isQuad--);
        }   
    }

    /// ---------------------------------------------------------- ///

    clock_t end2 = clock();
    elapsed_secs = 1000 * double(end2 - begin2) / CLOCKS_PER_SEC;
    if(VERBOSE) std::cout << "AABB+raster stage: " << elapsed_secs << "ms" << std::endl;
   
    /// FRAGMENT STEP ---------------------------------------- ///
    clock_t begin3 = clock();
    kernels["fragment_main"]->setArg(0, *frame_buffer->getGPUBuffer_colour());
    kernels["fragment_main"]->setArg(1, *frame_buffer->getGPUBuffer_tdata());
    kernels["fragment_main"]->setArg(2, *frame_buffer->getGPUBuffer_wpos());
    kernels["fragment_main"]->setArg(3, *frame_buffer->getGPUBuffer_normal());
    kernels["fragment_main"]->setArg(4, *frame_buffer->getGPUBuffer_tx());

    kernels["fragment_main"]->setArg(5, *triangle_buf_alldata);
    /*kernels["fragment_main"]->setArg(6, *default_tex->getGPUPtr());
    kernels["fragment_main"]->setArg(7, *normal_map->getGPUPtr());
    kernels["fragment_main"]->setArg(8, *specular_map->getGPUPtr());*/
    kernels["fragment_main"]->setArg(6, /**default_tex->getGPUPtr()*/*material_buffer);
    kernels["fragment_main"]->setArg(7, *material_buffer_properties);
    int swi = WIDTH;
    int shi = HEIGHT;
    kernels["fragment_main"]->setArg(8, sizeof(int), &swi);
    kernels["fragment_main"]->setArg(9, sizeof(int), &shi);

    const cl::NDRange screen = cl::NDRange(WIDTH * HEIGHT);
    const cl::NDRange offseta = cl::NDRange(0);

    queue->enqueueWriteBuffer(*frame_buffer->getGPUBuffer_tdata(), CL_TRUE, 0, sizeof(PixelTData) * HEIGHT * WIDTH, frame_buffer->getCPUBuffer_tdata());
    queue->finish();
    if(VERBOSE) std::cout << "enqueuing kernel" << std::endl;
    err = queue->enqueueNDRangeKernel(*kernels["fragment_main"], offseta, screen);
    queue->finish();
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }
    
    //std::cout << "copying back results" << std::endl;
    //queue->enqueueReadBuffer(*frame_buffer->getGPUBuffer(), CL_TRUE, 0, sizeof(Pixel) * HEIGHT * WIDTH, frame_buffer->getCPUBuffer());
    //queue->finish();
    clock_t end3 = clock();
    elapsed_secs = 1000 * double(end3 - begin3) / CLOCKS_PER_SEC;
    if (VERBOSE) {
        std::cout << "Fragment: " << elapsed_secs << "ms" << std::endl;
        std::cout << "Triangles Rendered: " << rendered_triangle_count << std::endl;

        /// ---------------------------------------------------------- ///

        std::cout << "Triangles Rendered: " << rendered_triangle_count << std::endl;
    }


    //TODO: generate fragments, maybe cpu is best for this, gpus normally do it in hardware
    //qsort(triangles, triangle_refs.size(), sizeof(glm::vec3) * 3, compare); //sorts triangles by lowest y value first (top of screen)
    //sortTriangles(triangles, triangles + triangle_refs.size());
    

    

    //printf("triangles = %d\n", triangle_refs.size());
    /*for (int i = 0; i < triangle_refs.size(); i++) {
            unsigned int& x = triangles[i].v0.i.x;
            unsigned int& y = triangles[i].v0.i.y;

            if (!(x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)) {
                frame_buffer[x + y * SCREEN_WIDTH].r = 255;
            }

            x = triangles[i].v1.i.x;
            y = triangles[i].v1.i.y;

            if (!(x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)) {
                frame_buffer[x + y * SCREEN_WIDTH].r = 255;
            }

            x = triangles[i].v2.i.x;
            y = triangles[i].v2.i.y;

            if (!(x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)) {
                frame_buffer[x + y * SCREEN_WIDTH].r = 255;
            }
    }*/
    

    /*unsigned int number_of_triangles = triangle_refs.size();
    unsigned int screen_w = SCREEN_WIDTH;
    unsigned int screen_h = SCREEN_HEIGHT;
    kernels["rasterizer"]->setArg(0, sizeof number_of_triangles, &number_of_triangles);
    kernels["rasterizer"]->setArg(1, *triangle_buff);
    kernels["rasterizer"]->setArg(2, *frame_buff);
    kernels["rasterizer"]->setArg(3, sizeof screen_w, &screen_w);
    kernels["rasterizer"]->setArg(4, sizeof screen_h, &screen_h);

    cl::NDRange offset = cl::NDRange(0);
    cl::NDRange global = cl::NDRange(number_of_triangles);
    cl_int err = queue->enqueueNDRangeKernel(*kernels["rasterizer"], NULL, global);
    if (err) {
        printf("Error: %d\n", err);
        while (1);
    }
    queue->enqueueReadBuffer(*frame_buff, false, 0, sizeof(Pixel) * SCREEN_WIDTH * SCREEN_HEIGHT, frame_buffer);
    
    queue->finish();*/
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

    /*for (auto& t : triangle_refs) {
        delete t;
    }*/
    triangle_refs.clear();

    delete[] triangles;
    //delete[] materials;
    //delete material_buffer;
    //delete frame_buff;
    //delete ssao_buff;
    delete ssao_sample_kernel;

    delete triangle_buff;
    delete screen_space_buff;
    delete device;
    delete context;
    delete queue;
    delete[] local_aabb_buff;
}

void RENDER::calculateSSAO(FrameBuffer* in_frame_buffer, FrameBuffer* out_ssao_buffer, int WIDTH, int HEIGHT, glm::vec3 campos, glm::vec3 camdir) {

    // Camera Properties
    float znear = 0.5f;
    float zfar = 250.0f;
    float FOV = 70.0f;
    float aspect = (float)WIDTH / (float)HEIGHT;

    // Construct matrices
    glm::mat4 VIEW_MATRIX = glm::lookAt(campos, campos + camdir, glm::vec3(0, 1, 0));
    glm::mat4 PROJECTION_MATRIX = glm::perspective(FOV*glm::pi<float>() / 180.0f, -((float)WIDTH / (float)HEIGHT), znear, zfar);
    glm::mat4 MVP_MATRIX = PROJECTION_MATRIX*VIEW_MATRIX;

    // Set kernel args
    kernels["shader_post_ssao"]->setArg(0, *in_frame_buffer->getGPUBuffer_wpos());
    kernels["shader_post_ssao"]->setArg(1, *in_frame_buffer->getGPUBuffer_normal());
    kernels["shader_post_ssao"]->setArg(2, *in_frame_buffer->getGPUBuffer_tdata());

    kernels["shader_post_ssao"]->setArg(3, *out_ssao_buffer->getGPUBuffer_colour());
    kernels["shader_post_ssao"]->setArg(4, *ssao_sample_kernel);
    kernels["shader_post_ssao"]->setArg(5, sizeof(int), &sample_count);
    kernels["shader_post_ssao"]->setArg(6, sizeof(cl_float) * 16, glm::value_ptr(MVP_MATRIX));

    int sw = WIDTH;
    int sh = HEIGHT;
    kernels["shader_post_ssao"]->setArg(7, sizeof(int), &sw);
    kernels["shader_post_ssao"]->setArg(8, sizeof(int), &sh);

    const cl::NDRange screen = cl::NDRange(WIDTH * HEIGHT);

    // Temp: copy back SSAO buffer (This wont be needed as we can just leave it on the GPU until the combine step
    cl_int err = queue->enqueueNDRangeKernel(*kernels["shader_post_ssao"], NULL, screen);
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }
    queue->finish();

    //queue->enqueueReadBuffer(*out_ssao_buffer->getGPUBuffer(), CL_TRUE, 0, sizeof(Pixel) * WIDTH * HEIGHT, out_ssao_buffer->getCPUBuffer());
   // queue->finish();
}

void RENDER::buildSSAOSampleKernel(int sample_num) {
    glm::vec3* sample_kernel = new glm::vec3[sample_num];

    const int SAMPLE_RADIUS = 1.25f;

    for (int i = 0; i < sample_num; i++) {
        sample_kernel[i] = glm::linearRand(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

       /*
        // This kernel generation function is better, but only works for normal-oriented kernels
       sample_kernel[i] = glm::linearRand(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        sample_kernel[i] = glm::normalize(sample_kernel[i]);*/
        sample_kernel[i] *= glm::linearRand(0.2f, 1.0f);
        

        // Scale sample (A higher radius increases the effect, but reduces precision)
        sample_kernel[i] *= SAMPLE_RADIUS;
    }
    ssao_sample_kernel_vals = sample_kernel;

    // Upload to GPU
    sample_count = sample_num;

    ssao_sample_kernel = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(glm::vec3) * sample_count);
    queue->enqueueWriteBuffer(*ssao_sample_kernel, CL_TRUE, 0, sizeof(glm::vec3) * sample_count, ssao_sample_kernel_vals);
}

// DIRECTIONAL SHADOWS
void RENDER::calculateShadows(FrameBuffer* in_frame_buffer, FrameBuffer* in_light_buffer, FrameBuffer* out_shadow_buffer, glm::vec3 lightpos, glm::vec3 lightdir, glm::vec3 campos) {

    int SHADOW_WIDTH  = in_light_buffer->getWidth();
    int SHADOW_HEIGHT = in_light_buffer->getHeight();
    int CAMERA_WIDTH  = in_frame_buffer->getWidth();
    int CAMERA_HEIGHT = in_frame_buffer->getHeight();

    // Camera Properties
    float znear = 0.5f;
    float zfar = 250.0f;
    float FOV = 70.0f;
    float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;

    // Construct matrices
    glm::mat4 VIEW_MATRIX = glm::lookAt(lightpos, lightpos + lightdir, glm::vec3(0, 1, 0));
    glm::mat4 PROJECTION_MATRIX = glm::perspective(FOV*glm::pi<float>() / 180.0f, -((float)SHADOW_WIDTH / (float)SHADOW_HEIGHT), znear, zfar);
    glm::mat4 LIGHT_MVP_MATRIX = PROJECTION_MATRIX*VIEW_MATRIX;

    // Set kernel args
    kernels["shader_directional_light_shadow"]->setArg(0, *in_frame_buffer->getGPUBuffer_wpos());
    kernels["shader_directional_light_shadow"]->setArg(1, *in_frame_buffer->getGPUBuffer_normal());
    kernels["shader_directional_light_shadow"]->setArg(2, *in_frame_buffer->getGPUBuffer_colour());
    kernels["shader_directional_light_shadow"]->setArg(3, *in_frame_buffer->getGPUBuffer_tdata());
    
    kernels["shader_directional_light_shadow"]->setArg(4, *in_light_buffer->getGPUBuffer_wpos());
    kernels["shader_directional_light_shadow"]->setArg(5, *out_shadow_buffer->getGPUBuffer_colour());
    kernels["shader_directional_light_shadow"]->setArg(6, sizeof(cl_float) * 16, glm::value_ptr(LIGHT_MVP_MATRIX));

    int sw = SHADOW_WIDTH;
    int sh = SHADOW_HEIGHT;
    kernels["shader_directional_light_shadow"]->setArg(7, sizeof(int), &sw);
    kernels["shader_directional_light_shadow"]->setArg(8, sizeof(int), &sh);
    kernels["shader_directional_light_shadow"]->setArg(9, sizeof(cl_float)*3, &campos);
    kernels["shader_directional_light_shadow"]->setArg(10, sizeof(cl_float)*3, &lightdir);

    

    // Enqueue kernel
    const cl::NDRange screen = cl::NDRange(CAMERA_WIDTH * CAMERA_HEIGHT);
    cl_int err = queue->enqueueNDRangeKernel(*kernels["shader_directional_light_shadow"], NULL, screen);
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }
    queue->finish();
}

// Point lights (Deferred)
//  - TODO: only enqueue the range of the kernel needed for points in the light. this allows far away lights to become very efficient.
//  - We can ignore the kernel running if no pixels in view are affected by the light.
void RENDER::calculatePointLight(FrameBuffer* in_frame_buffer, FrameBuffer* out_lighting_accum, glm::vec3 campos, glm::vec3 camdir, glm::vec3 lightpos, glm::vec3 lightcolour, float light_range, float specularity, float glossiness) {
    
    // Camera Properties
    float znear = 0.5f;
    float zfar = 250.0f;
    float FOV = 70.0f;
    float aspect = (float)in_frame_buffer->getWidth() / (float)in_frame_buffer->getHeight();

    // Construct matrices
    glm::mat4 VIEW_MATRIX = glm::lookAt(campos, campos + camdir, glm::vec3(0, 1, 0));
    glm::mat4 PROJECTION_MATRIX = glm::perspective(FOV*glm::pi<float>() / 180.0f, -((float)in_frame_buffer->getWidth() / (float)in_frame_buffer->getHeight()), znear, zfar);
    glm::mat4 MVP_MATRIX = PROJECTION_MATRIX*VIEW_MATRIX;

    // Prepare range
    int xmin = in_frame_buffer->getWidth();
    int ymin = in_frame_buffer->getHeight();
    int xmax = 0;
    int ymax = 0;

    // Project light point into centre of screen
    /*
        We calculate the range of pixels the kernel needs to execute over by projecting the corners
        of the light bounding area into screen-space.

        This saves computational power by not processing the area that a light does not affect.
    */
    std::vector<glm::vec3> points = {
        glm::vec3(lightpos.x - light_range, lightpos.y - light_range, lightpos.z - light_range),
        glm::vec3(lightpos.x - light_range, lightpos.y - light_range, lightpos.z + light_range),
        glm::vec3(lightpos.x - light_range, lightpos.y + light_range, lightpos.z - light_range),
        glm::vec3(lightpos.x - light_range, lightpos.y + light_range, lightpos.z + light_range),
        glm::vec3(lightpos.x + light_range, lightpos.y - light_range, lightpos.z - light_range),
        glm::vec3(lightpos.x + light_range, lightpos.y - light_range, lightpos.z + light_range),
        glm::vec3(lightpos.x + light_range, lightpos.y + light_range, lightpos.z - light_range),
        glm::vec3(lightpos.x + light_range, lightpos.y + light_range, lightpos.z + light_range)
    };

    for (auto &p : points) {
        glm::vec4 light_screenspace = MVP_MATRIX*glm::vec4(p.x, p.y, p.z, 1.0f);
        light_screenspace /= light_screenspace.w;
        light_screenspace.x *= 0.5f;
        light_screenspace.y *= 0.5f;
        light_screenspace.x += 0.5f;
        light_screenspace.y += 0.5f;
        light_screenspace.x *= in_frame_buffer->getWidth();
        light_screenspace.y *= in_frame_buffer->getHeight();

        /*if (light_screenspace.x >= 0 && 
            light_screenspace.y >= 0 && 
            light_screenspace.x < in_frame_buffer->getWidth() &&
            light_screenspace.y < in_frame_buffer->getHeight()) {*/

            xmin = glm::min(xmin, (int)light_screenspace.x);
            ymin = glm::min(ymin, (int)light_screenspace.y);
            xmax = glm::max(xmax, (int)light_screenspace.x);
            ymax = glm::max(ymax, (int)light_screenspace.y);
        //}
    }
    if(VERBOSE) std::cout << "Light (" << xmin << "," << ymin << ") to (" << xmax << "," << ymax << ")" << std::endl;
    xmin = glm::clamp(xmin, 0, in_frame_buffer->getWidth()-1);
    ymin = glm::clamp(ymin, 0, in_frame_buffer->getHeight()-1);
    xmax = glm::clamp(xmax, 0, in_frame_buffer->getWidth()-1);
    ymax = glm::clamp(ymax, 0, in_frame_buffer->getHeight()-1);

    // calculate area
    int area = (xmax - xmin)*(ymax - ymin);

    if (area > 0 && xmax > xmin && ymax > ymin) {



        // Set kernel args
        kernels["shader_point_light"]->setArg(0, *in_frame_buffer->getGPUBuffer_wpos());
        kernels["shader_point_light"]->setArg(1, *in_frame_buffer->getGPUBuffer_normal());
        kernels["shader_point_light"]->setArg(2, *in_frame_buffer->getGPUBuffer_tdata());

        kernels["shader_point_light"]->setArg(3, *out_lighting_accum->getGPUBuffer_colour());
        kernels["shader_point_light"]->setArg(4, sizeof(cl_float) * 3, &campos);
        kernels["shader_point_light"]->setArg(5, sizeof(cl_float) * 3, &lightpos);
        kernels["shader_point_light"]->setArg(6, sizeof(cl_float) * 3, &lightcolour);
        kernels["shader_point_light"]->setArg(7, sizeof(cl_float), &light_range);
        kernels["shader_point_light"]->setArg(8, sizeof(cl_float), &specularity);
        kernels["shader_point_light"]->setArg(9, sizeof(cl_float), &glossiness);

        int width = in_frame_buffer->getWidth();
        kernels["shader_point_light"]->setArg(10, sizeof(cl_int), &width);

        const cl::NDRange screen = cl::NDRange(xmax-xmin, ymax-ymin);
        const cl::NDRange offset = cl::NDRange(xmin, ymin);

        // Enqueue kernel
        cl_int err = queue->enqueueNDRangeKernel(*kernels["shader_point_light"], offset, screen);
        if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }
        queue->finish();
    }

    // TEMP: Transfer results back to CPU (only needed for debugging purposes)
    //queue->enqueueReadBuffer(*out_lighting_accum->getGPUBuffer(), CL_TRUE, 0, sizeof(Pixel) *in_frame_buffer->getWidth()*in_frame_buffer->getHeight(), out_lighting_accum->getCPUBuffer());
    //queue->finish();
}

void RENDER::accumulateBuffers(FrameBuffer* in_frame_buffer, FrameBuffer* in_ssao_buffer, FrameBuffer* in_shadow_buffer, int WIDTH, int HEIGHT) {

    // Set kernel args
    kernels["accumulate_buffers"]->setArg(0, *in_frame_buffer->getGPUBuffer_colour());
    kernels["accumulate_buffers"]->setArg(1, *in_ssao_buffer->getGPUBuffer_colour());
    kernels["accumulate_buffers"]->setArg(2, *in_shadow_buffer->getGPUBuffer_colour());

    // Construct range
    const cl::NDRange screen = cl::NDRange(WIDTH * HEIGHT);

    // Enqueue kernel
    cl_int err = queue->enqueueNDRangeKernel(*kernels["accumulate_buffers"], NULL, screen);
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }
    queue->finish();
}

// Framebuffer
FrameBuffer::FrameBuffer(int width, int height, const cl::Context *context) {
    this->width = width;
    this->height = height;

    this->cpu_buffer_colour = new PixelColour[width*height];
    this->cpu_buffer_tdata  = new PixelTData[width*height];
    this->cpu_buffer_wpos   = new PixelWPos[width*height];
    this->cpu_buffer_normal = new PixelNormal[width*height];
    this->cpu_bufer_tx      = new PixelTX[width*height];

    this->gpu_buffer_colour = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(PixelColour) * width * height);
    this->gpu_buffer_tdata  = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(PixelTData) * width * height);
    this->gpu_buffer_wpos   = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(PixelWPos) * width * height);
    this->gpu_buffer_normal = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(PixelNormal) * width * height);
    this->gpu_buffer_tx     = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(PixelTX) * width * height);
}

PixelColour* FrameBuffer::getCPUBuffer_colour() {
    return this->cpu_buffer_colour;
}

PixelTData* FrameBuffer::getCPUBuffer_tdata() {
    return this->cpu_buffer_tdata;
}

PixelWPos*  FrameBuffer::getCPUBuffer_wpos() {
    return this->cpu_buffer_wpos;
}

PixelNormal* FrameBuffer::getCPUBuffer_normal() {
    return this->cpu_buffer_normal;
}

PixelTX* FrameBuffer::getCPUBuffer_tx() {
    return this->cpu_bufer_tx;
}

cl::Buffer* FrameBuffer::getGPUBuffer_colour() {
    return this->gpu_buffer_colour;
}

cl::Buffer* FrameBuffer::getGPUBuffer_tdata() {
    return this->gpu_buffer_tdata;
}

cl::Buffer* FrameBuffer::getGPUBuffer_wpos() {
    return this->gpu_buffer_wpos;
}

cl::Buffer* FrameBuffer::getGPUBuffer_normal() {
    return this->gpu_buffer_normal;
}


cl::Buffer* FrameBuffer::getGPUBuffer_tx() {
    return this->gpu_buffer_tx;
}

FrameBuffer::~FrameBuffer() {
    delete[] this->cpu_buffer_colour;
    delete[] this->cpu_buffer_tdata;
    delete[] this->cpu_buffer_wpos;
    delete[] this->cpu_buffer_normal;
    delete[] this->cpu_bufer_tx;

    delete this->gpu_buffer_colour;
    delete this->gpu_buffer_tdata;
    delete this->gpu_buffer_wpos;
    delete this->gpu_buffer_normal;
    delete this->gpu_buffer_tx;
}

int FrameBuffer::getWidth() {
    return this->width;
}

int FrameBuffer::getHeight() {
    return this->height;
}

void FrameBuffer::saveBMP(const std::string filename) {
    bitmap_image* bmp = new bitmap_image(this->width, this->height);
    for (int i = 0; i < this->width; i++) {
        for (int j = 0; j < this->height; j++) {
            bmp->set_pixel(i, j, this->cpu_buffer_colour[i + j*this->width].r,
                                 this->cpu_buffer_colour[i + j*this->width].g,
                                 this->cpu_buffer_colour[i + j*this->width].b);
        }
    }
    bmp->save_image(filename);
}

void FrameBuffer::clear() {
    for (int i = 0; i < this->width; i++) {
        for (int j = 0; j < this->height; j++) {
            this->cpu_buffer_colour[i + j*this->width] = {};
            this->cpu_buffer_tdata[i + j*this->width] = {};
            this->cpu_buffer_wpos[i + j*this->width] = {};
            this->cpu_buffer_normal[i + j*this->width] = {};
            this->cpu_bufer_tx[i + j*this->width] = {};
        }
    }
}
void FrameBuffer::transferGPUtoCPU_Colour() {
    RENDER::queue->enqueueReadBuffer(*this->getGPUBuffer_colour(), CL_TRUE, 0, sizeof(PixelColour) *this->getWidth()*this->getHeight(), this->getCPUBuffer_colour());
    RENDER::queue->finish();
}
void FrameBuffer::transferGPUtoCPU_TData() {
    RENDER::queue->enqueueReadBuffer(*this->getGPUBuffer_tdata(), CL_TRUE, 0, sizeof(PixelTData) *this->getWidth()*this->getHeight(), this->getCPUBuffer_tdata());
    RENDER::queue->finish();
}
void FrameBuffer::transferGPUtoCPU_Wpos() {
    RENDER::queue->enqueueReadBuffer(*this->getGPUBuffer_wpos(), CL_TRUE, 0, sizeof(PixelWPos) *this->getWidth()*this->getHeight(), this->getCPUBuffer_wpos());
    RENDER::queue->finish();
}
void FrameBuffer::transferGPUtoCPU_Normal() {
    RENDER::queue->enqueueReadBuffer(*this->getGPUBuffer_normal(), CL_TRUE, 0, sizeof(PixelNormal) *this->getWidth()*this->getHeight(), this->getCPUBuffer_normal());
    RENDER::queue->finish();
}
void FrameBuffer::transferGPUtoCPU_Tx() {
    RENDER::queue->enqueueReadBuffer(*this->getGPUBuffer_tx(), CL_TRUE, 0, sizeof(PixelTX) *this->getWidth()*this->getHeight(), this->getCPUBuffer_tx());
    RENDER::queue->finish();
}

// MATERIALS
void Material::prepareGPUMaterial() {
    this->gpumaterial.specularity = specularity;
    this->gpumaterial.glossiness = glossiness;
    this->gpumaterial.reflectivity = reflectivity;
    this->gpumaterial.r = r;
    this->gpumaterial.g = g;
    this->gpumaterial.b = b;
}