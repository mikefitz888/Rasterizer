#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
} Pixel;


kernel void rasterizer(const unsigned int width, global Pixel* const frame_buf, const unsigned int screen_w, const unsigned int screen_h) {

}

