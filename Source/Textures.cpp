#include "../Include/textures.h"

Texture::Texture(const std::string &filename, const cl::Context *context, const cl::CommandQueue *queue ) {
    bmp = new bitmap_image(filename);
    if (bmp->width() > 0 && bmp->height() > 0) {
        if (bmp->width() == 512 && bmp->height() == 512) {
            // Copy texture data
            //this->texture->width = 1024;//bmp->width;
           // this->texture->height = 1024;//bmp->height;
            this->texture = new clTexture();

            for (int i = 0; i < 512/*this->texture->width*/; i++) {
                for (int j = 0; j < 512/*this->texture->height*/; j++) {
                    //  RGB col;
                    
                    unsigned char r, g, b;
                    bmp->get_pixel(i, j, r, g, b);

                    this->texture->colours[i + j * 512].a = 255;
                    this->texture->colours[i + j * 512].r = r;
                    this->texture->colours[i + j * 512].g = g;
                    this->texture->colours[i + j * 512].b = b;
                }
            }

            // Allocate on GPU
            this->gpu_mem_ptr = new cl::Buffer(*context, CL_MEM_READ_ONLY, /*sizeof(int) * 2 + */sizeof(Colour) * 512 * 512/*this->texture->width*this->texture->height*/);
            cl_int err = queue->enqueueWriteBuffer(*(this->gpu_mem_ptr), CL_TRUE, 0, /*sizeof(int) * 2 + */sizeof(Colour) * 512 * 512/*this->texture->width*this->texture->height*/, this->getClTexture());
            if (err != 0) {
                std::cout << "ERROR!!!" << std::endl;
                int a;
                std::cin >> a;
            }
            queue->finish();
        } else {
            std::cout << "Error Loading Texture!! Size MUST be 512x512 " << filename << std::endl;
            int a;
            std::cin >> a;
        }
    } else {
        std::cout << "Error Loading Texture!! " << filename << std::endl;
        int a;
        std::cin >> a;
    }
}

Colour* Texture::getClTexture() {
    return this->texture->colours;
}

cl::Buffer* Texture::getGPUPtr() {
    return this->gpu_mem_ptr;
}