#ifdef __NV_CL_C_VERSION
typedef uchar uint8_custom;
typedef uint uint32_custom;
#define __PACKED__ __attribute__((packed))
#else
typedef uint8_t uint8_custom;
typedef uint32_t uint32_custom;
#define __PACKED__
#endif // __NV_CL_C_VERSION

struct Pixel __PACKED__ {
    //Core pixel data
    uint8_custom r, g, b, a;
    uint32_custom triangle_id, material;
    float depth, u, v, w;
};

struct Material __PACKED__ {
    float r, g, b, a;
};