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

            // Generate mip-map
            generateMipMapLevels();

            // Allocate on GPU
            this->gpu_mem_ptr = new cl::Buffer(*context, CL_MEM_READ_ONLY, /*sizeof(int) * 2 + */sizeof(Colour) * 349526/*this->texture->width*this->texture->height*/);
            cl_int err = queue->enqueueWriteBuffer(*(this->gpu_mem_ptr), CL_TRUE, 0, /*sizeof(int) * 2 + */sizeof(Colour) * 349526/*this->texture->width*this->texture->height*/, this->getClTexture());
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

void Texture::generateMipMapLevels() {
    int current_resolution = 512;
    int current_offset = current_resolution*current_resolution;
    
    for (int L = 1; L <= 9; L++) {
        current_resolution >>= 1;

        
        for (int j = 0; j < current_resolution; j++) {
            for (int i = 0; i < current_resolution; i++) {

                int sample_count = 512 / current_resolution;
                int accum_r, accum_g, accum_b, accum_a;
                accum_r = 0;
                accum_g = 0;
                accum_b = 0;
                accum_a = 0;

                for (int xx = 0; xx < sample_count; xx++) {
                    for (int yy = 0; yy < sample_count; yy++) {
                        int sample_x = sample_count*i + xx;
                        int sample_y = sample_count*j + yy;
                        
                        Colour c = this->texture->colours[sample_x + sample_y * 512];
                        accum_r += c.r;
                        accum_g += c.g;
                        accum_b += c.b;
                        accum_a += c.a;
                    }
                }

                // Average
                int tot = sample_count*sample_count;
                accum_r /= tot; 
                accum_g /= tot;
                accum_b /= tot;
                accum_a /= tot;

                // STORE
                Colour col;
                col.r = accum_r;
                col.g = accum_g;
                col.b = accum_b;
                col.a = accum_a;

                //std::cout << "OFF: " << current_offset + i + j*current_resolution << " i: " << i << " j: " << j << std::endl;
                this->texture->colours[current_offset + i+j*current_resolution] = col;

                // Increment offset
                
            }
        }
        current_offset += current_resolution*current_resolution;
        //std::cout << "Mipmap level " << L << " generated!" << std::endl;
    }
}