#include "../Include/RenderUtil.h"

//Honestly got no clue why this is required, compiler whines too much, it should be able to determine this from the .h declarations
cl::Buffer* RENDER::frame_buff;
cl::Buffer* RENDER::triangle_buff;
cl::Buffer* RENDER::triangle_buf_alldata;
cl::Buffer* RENDER::screen_space_buff;
cl::Buffer* RENDER::material_buffer;
cl::Buffer* RENDER::aabb_buffer;

cl::Buffer* RENDER::ssao_buff;
cl::Buffer* RENDER::ssao_sample_kernel;
int RENDER::sample_count = 0;
glm::vec3* RENDER::ssao_sample_kernel_vals;

std::vector<Triangle*> RENDER::triangle_refs;
triplet* RENDER::triangles;
Material* RENDER::materials;
AABB* RENDER::local_aabb_buff;

cl::Device* RENDER::device;
cl::Context* RENDER::context;
cl::CommandQueue* RENDER::queue;
std::vector<cl::Program*> RENDER::programs;
std::map<std::string, cl::Kernel*> RENDER::kernels;

bool RENDER::scene_changed = false;

void RENDER::getOCLDevice() {
    //OpenCl
    std::vector<cl::Platform> all_platforms;
    //cl::Platform::get(&all_platforms);
    cl::Platform::get(&all_platforms);

    //cl::Platform default_platform;
    //cl::Platform::get(&default_platform);

    std::cout << all_platforms.size() << " Platforms found!" << std::endl;
    if (all_platforms.size() == 0) {
        std::cout << " No platforms found. Check OpenCL installation!\n";
        exit(1);
    }


    cl::Platform default_platform = all_platforms[0];
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
    }
    device = new cl::Device(all_devices[0]);
    std::cout << "Using platform: " << device->getInfo<CL_DEVICE_NAME>() << "\n";
}

void RENDER::loadOCLKernels() {
    //also create + builds programs
    std::vector<std::string> kernel_names = { "rasterizer", "projection", "shaedars", "aabb", "ssao" };
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
            printf("%d Error: %d\n", __LINE__, err);
            while (1);
        }
    }

    unsigned int n = 0;
    for (auto program : programs) {
        cl_int err;
        char options[128];
        sprintf(options, "-D SCREEN_WIDTH=%d -D SCREEN_HEIGHT=%d", SCREEN_WIDTH, SCREEN_HEIGHT);
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
    frame_buff = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(Pixel) * SCREEN_WIDTH * SCREEN_HEIGHT);
    triangle_buff = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(triplet) * triangle_refs.size());
    triangle_buf_alldata = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(Triangle) * triangle_refs.size());

    screen_space_buff = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(triplet) * triangle_refs.size());
    material_buffer = new cl::Buffer(*context, CL_MEM_READ_ONLY, sizeof(Material) * triangle_refs.size());
    aabb_buffer = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(AABB) * triangle_refs.size());

    // SSAO
    ssao_buff = new cl::Buffer(*context, CL_MEM_READ_WRITE, sizeof(Pixel) * SCREEN_WIDTH * SCREEN_HEIGHT);
    
}

void RENDER::writeTriangles() {
    //May need to modify this to do transfer in multiple steps if memory requirement is too high
    triangles = new triplet[triangle_refs.size()];
    materials = new Material[triangle_refs.size()];
    //printf("SIZE = %d\n", triangle_refs.size());
    
    for (int i = 0; i < triangle_refs.size(); i++) {
        auto& t = triangle_refs[i];
        materials[i].r = t->color.r;
        materials[i].g = t->color.g;
        materials[i].b = t->color.b;

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


    Triangle* triangles_FULL = new Triangle[triangle_refs.size()];
    for (int i = 0; i < triangle_refs.size(); i++) {
        triangles_FULL[i] = *triangle_refs[i];
    }
    err = queue->enqueueWriteBuffer(*triangle_buf_alldata, CL_TRUE, 0, sizeof(Triangle) * triangle_refs.size(), triangles_FULL);

    err = queue->enqueueWriteBuffer(*material_buffer, CL_TRUE, 0, sizeof(Material) * triangle_refs.size(), materials);
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
void RENDER::renderFrame(Pixel* frame_buffer, int WIDTH, int HEIGHT, glm::vec3 campos, glm::vec3 camdir) {

   

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



    unsigned int number_of_triangles = triangle_refs.size();
    std::cout << "NUMBER OF TRIANGLES: " << number_of_triangles << std::endl;

    //Converts world space triangles to screen space
    //The z value of each "2D" triangle is used to retain the depth information from world space (not correctly implemented yet)
    //This section only runs if the "triangle" buffer has been written to after this section has last been run
    cl_int err = 0;

    clock_t begin = clock();
    /// PROJECTION STAGE (GPU) ---------------------------------------------------------------------------------------------------//
    const cl::NDRange offset = cl::NDRange(0);
    const cl::NDRange global = cl::NDRange(number_of_triangles);
    err = queue->enqueueNDRangeKernel(*kernels["projection"], NULL, global);

    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }

    // Dont need to copy this if running rasterization step
    queue->enqueueReadBuffer(*screen_space_buff, CL_TRUE, 0, sizeof(triplet) * triangle_refs.size(), triangles);

    clock_t end = clock();
    double elapsed_secs = 1000*double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "Projection stage: " << elapsed_secs << "ms" << std::endl;
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
        /// Projection.cl port (CPU)---------------------------------------- ///
        /*triplet& t = triangles[i];

        // World pos
        glm::vec4 world_pos_0(triangle_refs[i]->v0.x, triangle_refs[i]->v0.y, triangle_refs[i]->v0.z, 1);
        glm::vec4 world_pos_1(triangle_refs[i]->v1.x, triangle_refs[i]->v1.y, triangle_refs[i]->v1.z, 1);
        glm::vec4 world_pos_2(triangle_refs[i]->v2.x, triangle_refs[i]->v2.y, triangle_refs[i]->v2.z, 1);

        // Transform to view position
        glm::vec4 view_pos0, view_pos1, view_pos2;
        view_pos0 = VIEW_MATRIX*world_pos_0;
        view_pos1 = VIEW_MATRIX*world_pos_1;
        view_pos2 = VIEW_MATRIX*world_pos_2;

        // Projection
        glm::vec4 proj_pos0, proj_pos1, proj_pos2;
        proj_pos0 = PROJECTION_MATRIX*view_pos0;
        proj_pos1 = PROJECTION_MATRIX*view_pos1;
        proj_pos2 = PROJECTION_MATRIX*view_pos2;

        // PERSPECTIVE DIVIDE
        proj_pos0 /= proj_pos0.w;
        proj_pos1 /= proj_pos1.w;
        proj_pos2 /= proj_pos2.w;

        // Clipping
        proj_pos0 = CLIP_MATRIX*proj_pos0;
        proj_pos1 = CLIP_MATRIX*proj_pos1;
        proj_pos2 = CLIP_MATRIX*proj_pos2;

        // Collect result
        s.v0.f.x = proj_pos0.x; 
        s.v0.f.y = proj_pos0.y;
        s.v0.f.z = -view_pos0.z; // Use view instead of projected z for depth as it equates to actual distance from camera, rather than re-scaled

        s.v1.f.x = proj_pos1.x;
        s.v1.f.y = proj_pos1.y;
        s.v1.f.z = -view_pos1.z;

        s.v2.f.x = proj_pos2.x;
        s.v2.f.y = proj_pos2.y;
        s.v2.f.z = -view_pos2.z;*/
        /// ---------------------------------------------------------- ///

        /// Rasterizer.cl port CPU)---------------------------------------- ///
        //triplet& s = triangles[i];
        glm::ivec2 a = glm::ivec2(s.v0.i.x, s.v0.i.y);
        glm::ivec2 b = glm::ivec2(s.v1.i.x, s.v1.i.y);
        glm::ivec2 c = glm::ivec2(s.v2.i.x, s.v2.i.y);

        local_aabb_buff[i].a = a;
        local_aabb_buff[i].d0 = s.v0.f.z;
        local_aabb_buff[i].d1 = s.v1.f.z;
        local_aabb_buff[i].d2 = s.v2.f.z;
        local_aabb_buff[i].triangle_id = i;

        local_aabb_buff[i].v0 = (glm::vec2)(b - a);
        local_aabb_buff[i].v1 = (glm::vec2)(c - a);

        local_aabb_buff[i].inv_denom = 1.f / (local_aabb_buff[i].v0.x * local_aabb_buff[i].v1.y - local_aabb_buff[i].v1.x * local_aabb_buff[i].v0.y);

        local_aabb_buff[i].minX = glm::min(glm::min(a.x, b.x), c.x);
        local_aabb_buff[i].minY = glm::min(glm::min(a.y, b.y), c.y);
        local_aabb_buff[i].maxX = glm::max(glm::max(a.x, b.x), c.x);
        local_aabb_buff[i].maxY = glm::max(glm::max(a.y, b.y), c.y);
       /* if (i == 0) {
            printf("CPU(%d %d %d %d) \n", local_aabb_buff[i].minX, local_aabb_buff[i].minY, local_aabb_buff[i].maxX, local_aabb_buff[i].maxY);
        }
        */
        /// ---------------------------------------------------------- ///
        bool skip = false;

        // Naive depth clipping
        if (local_aabb_buff[i].d0 <= 0.0 || local_aabb_buff[i].d1 <= 0.0 || local_aabb_buff[i].d2 <= 0.0) {
            skip = true;
        }
        if (s.v0.f.z <= znear || s.v1.f.z <= znear || s.v2.f.z <= znear) {
            skip = true;
        }
        //if (i > 0) { skip = true; }
        
        // Backface culling
        //triangle_refs[local_aabb_buff[i].triangle_id]->ComputeNormal();
        glm::vec3 cam_to_face = triangle_refs[local_aabb_buff[i].triangle_id]->v0 - campos;
        if (glm::dot(triangle_refs[local_aabb_buff[i].triangle_id]->normal, cam_to_face) <= 0.0f) {
            skip = true;
        }
        /// AABB step CPU ---------------------------------------- ///
        if (!skip) {
            rendered_triangle_count++;

            // (Naive clipping) Clamp to screen region
            int minX = glm::clamp(local_aabb_buff[i].minX, 0, WIDTH -1);
            int minY = glm::clamp(local_aabb_buff[i].minY, 0, HEIGHT-1);
            int maxX = glm::clamp(local_aabb_buff[i].maxX, 0, WIDTH -1);
            int maxY = glm::clamp(local_aabb_buff[i].maxY, 0, HEIGHT-1);

            //if (i == 0) {
            //    std::cout << "W: " << s.v0.f.w << std::endl;
            //}

            for (int x = minX; x <= maxX; x++) {
                for (int y = minY; y <= maxY; y++) {

                   // if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                        glm::ivec2 v2 = glm::ivec2(x, y) - local_aabb_buff[i].a;
                        glm::ivec2 v0 = local_aabb_buff[i].v0;
                        glm::ivec2 v1 = local_aabb_buff[i].v1;

                        float b = (v2.x * v1.y - v1.x * v2.y) * local_aabb_buff[i].inv_denom;
                        float c = (v0.x * v2.y - v2.x * v0.y) * local_aabb_buff[i].inv_denom;
                        float a = 1.0f - b - c;




                        if ((a <= 1 && b <= 1 && c <= 1 && a >= 0 && b >= 0 && c >= 0)) {

                            // Correct barycentric coords for perspective:
                            float zf = a / s.v0.f.z + b / s.v1.f.z + c / s.v2.f.z;
                            a /= (s.v0.f.z*zf);
                            b /= (s.v1.f.z*zf);
                            c /= (s.v2.f.z*zf);

                            // Perspective-correct depth;
                            
                            float depth = (a*local_aabb_buff[i].d0 + b*local_aabb_buff[i].d1  + c*local_aabb_buff[i].d2 );


                            //std::cout << "DEPTH: " << depth << std::endl;
                            if ( (frame_buffer[x + y * WIDTH].depth == 0 || depth < frame_buffer[x + y * WIDTH].depth) && depth > znear && depth < zfar ) {
                                
                                // Store triangle


                                frame_buffer[x + y*WIDTH].triangle_id = i;

                                // Store (PERSPECTIVE CORRECT? Maybe?) barycentric interpolators
                                /*frame_buffer[x + y*WIDTH].va = a / (s.v0.f.z*zf);
                                frame_buffer[x + y*WIDTH].vb = b / (s.v1.f.z*zf);
                                frame_buffer[x + y*WIDTH].vc = c / (s.v2.f.z*zf);*/

                               /* float zz = 1 / (a * 1.0/s.v0.f.z + b * 1.0/s.v1.f.z + c * 1.0/s.v2.f.z);
                                frame_buffer[x + y*WIDTH].va = a*zz/s.v0.f.z;
                                frame_buffer[x + y*WIDTH].vb = b*zz/s.v1.f.z;
                                frame_buffer[x + y*WIDTH].vc = c*zz/s.v2.f.z;*/

                                frame_buffer[x + y*WIDTH].va = a;
                                frame_buffer[x + y*WIDTH].vb = b;
                                frame_buffer[x + y*WIDTH].vc = c;

                                // Write interpolated depth
                                frame_buffer[x + y*WIDTH].depth = depth;

                                // Perspective correct UV:
                                
                                /*float perspCorrU = (a*triangle_refs[i]->uv0.x / s.v0.f.z + b*triangle_refs[i]->uv1.x / s.v1.f.z + c*triangle_refs[i]->uv2.x / s.v2.f.z) / zf;
                                float perspCorrV = (a*triangle_refs[i]->uv0.y / s.v0.f.z + b*triangle_refs[i]->uv1.y / s.v1.f.z + c*triangle_refs[i]->uv2.y / s.v2.f.z) / zf;
                                frame_buffer[x + y*WIDTH].uvx = perspCorrU;
                                frame_buffer[x + y*WIDTH].uvy = perspCorrV;*/
                            }
                        }
                   // }

                }
            }
        }
        /// ---------------------------------------------------------- ///
    }
    clock_t end2 = clock();
    elapsed_secs = 1000 * double(end2 - begin2) / CLOCKS_PER_SEC;
    std::cout << "AABB+raster stage: " << elapsed_secs << "ms" << std::endl;
    


    /// Fragment Step (CPU)  ---------------------------------------- ///
    /// - Ideally this should be done on the GPU, but that requires the triangle_refs[] array
    /// - This is currently done here to ensure it is only run atmost once per pixel. (vs the above which could run many times per pixel)
/*#pragma omp parallel for
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {

            if (frame_buffer[x + y*SCREEN_WIDTH].depth > 0) {

                // Get info
                int i = frame_buffer[x + y*SCREEN_WIDTH].triangle_id;
                float u = frame_buffer[x + y*SCREEN_WIDTH].va;
                float v = frame_buffer[x + y*SCREEN_WIDTH].vb;
                float w = frame_buffer[x + y*SCREEN_WIDTH].vc;

                // Get TX coord
                float tx = triangle_refs[i]->uv0.x*u + triangle_refs[i]->uv1.x*v + triangle_refs[i]->uv2.x*w;
                float ty = triangle_refs[i]->uv0.y*u + triangle_refs[i]->uv1.y*v + triangle_refs[i]->uv2.y*w;
                frame_buffer[x + y*SCREEN_WIDTH].uvx = tx;
                frame_buffer[x + y*SCREEN_WIDTH].uvx = ty;

                // Get interpolated Normal
                float nx = -(triangle_refs[i]->n0.x*u + triangle_refs[i]->n1.x*v + triangle_refs[i]->n2.x*w);
                float ny = -(triangle_refs[i]->n0.y*u + triangle_refs[i]->n1.y*v + triangle_refs[i]->n2.y*w);
                float nz = -(triangle_refs[i]->n0.z*u + triangle_refs[i]->n1.z*v + triangle_refs[i]->n2.z*w);
                frame_buffer[x + y*SCREEN_WIDTH].nx = nx;
                frame_buffer[x + y*SCREEN_WIDTH].ny = ny;
                frame_buffer[x + y*SCREEN_WIDTH].nz = nz;

                // Get interpolated position
                glm::vec3 pos = u*triangle_refs[i]->v0 + v*triangle_refs[i]->v1 + w*triangle_refs[i]->v2;
                frame_buffer[x + y*SCREEN_WIDTH].x = pos.x;
                frame_buffer[x + y*SCREEN_WIDTH].y = pos.y;
                frame_buffer[x + y*SCREEN_WIDTH].z = pos.z;


                frame_buffer[x + y*SCREEN_WIDTH].r = 255 * (frame_buffer[x + y*SCREEN_WIDTH].nx*0.5 + 0.5);
                frame_buffer[x + y*SCREEN_WIDTH].g = 255 * (frame_buffer[x + y*SCREEN_WIDTH].ny*0.5 + 0.5);
                frame_buffer[x + y*SCREEN_WIDTH].b = 255 * (frame_buffer[x + y*SCREEN_WIDTH].nz*0.5 + 0.5);
            }
        }
    }*/
    /// ---------------------------------------------------------- ///
   
    /// FRAGMENT STEP ---------------------------------------- ///
    clock_t begin3 = clock();
    kernels["shaedars"]->setArg(0, *frame_buff);
    kernels["shaedars"]->setArg(1, *material_buffer);
    kernels["shaedars"]->setArg(2, *triangle_buf_alldata);
    const cl::NDRange screen = cl::NDRange(WIDTH * HEIGHT);
    
    queue->enqueueWriteBuffer(*frame_buff, CL_TRUE, 0, sizeof(Pixel) * HEIGHT * WIDTH, frame_buffer);

    err = queue->enqueueNDRangeKernel(*kernels["shaedars"], NULL, screen);
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }


    queue->enqueueReadBuffer(*frame_buff, CL_TRUE, 0, sizeof(Pixel) * HEIGHT * WIDTH, frame_buffer);
    queue->finish();
    clock_t end3 = clock();
    elapsed_secs = 1000 * double(end3 - begin3) / CLOCKS_PER_SEC;
    std::cout << "Fragment: " << elapsed_secs << "ms" << std::endl;
    std::cout << "Triangles Rendered: " << rendered_triangle_count << std::endl;
    /// ---------------------------------------------------------- ///

    std::cout << "Triangles Rendered: " << rendered_triangle_count << std::endl;
    


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
    delete[] materials;
    delete material_buffer;
    delete frame_buff;
    delete ssao_buff;
    delete ssao_sample_kernel;

    delete triangle_buff;
    delete screen_space_buff;
    delete device;
    delete context;
    delete queue;
    delete[] local_aabb_buff;
}

void RENDER::calculateSSAO(Pixel* out_ssao_buffer, int WIDTH, int HEIGHT, glm::vec3 campos, glm::vec3 camdir) {

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
    kernels["ssao"]->setArg(0, *frame_buff);
    kernels["ssao"]->setArg(1, *ssao_buff);
    kernels["ssao"]->setArg(2, *ssao_sample_kernel);
    kernels["ssao"]->setArg(3, sizeof(int), &sample_count);
    kernels["ssao"]->setArg(4, sizeof(cl_float) * 16, glm::value_ptr(MVP_MATRIX));


    const cl::NDRange screen = cl::NDRange(WIDTH * HEIGHT);

    // Temp: copy back SSAO buffer (This wont be needed as we can just leave it on the GPU until the combine step
    cl_int err = queue->enqueueNDRangeKernel(*kernels["ssao"], NULL, screen);
    if (err) { printf("%d Error: %d\n", __LINE__, err); while (1); }

    queue->enqueueReadBuffer(*ssao_buff, CL_TRUE, 0, sizeof(Pixel) * WIDTH * HEIGHT, out_ssao_buffer);
    queue->finish();
}

void RENDER::buildSSAOSampleKernel(int sample_num) {
    glm::vec3* sample_kernel = new glm::vec3[sample_num];

    const int SAMPLE_RADIUS = 1.0f;

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