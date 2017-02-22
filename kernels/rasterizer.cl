#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
} Pixel;


kernel void rasterizer(const unsigned int triangle_count, const unsigned int screen_w, global float3* const triangle_buff, global Pixel* const frame_buf) {
    unsigned int pixel_id = get_global_id(0) + get_global_id(1) * screen_w;
    frame_buf[pixel_id].r = 255;
    frame_buf[pixel_id].g = 0;
    frame_buf[pixel_id].b = 0;
    frame_buf[pixel_id].a = 255;
}

