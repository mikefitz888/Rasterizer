typedef struct {
    //float m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33;//
    float m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33;
} mat4;


typedef struct {
    //float m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33;//
    float m00, m01, m02, m10, m11, m12, m20, m21, m22;
} mat3;

inline float4 mul(float4 input, mat4 mat) {
    float4 vec;
    vec.x = input.x*mat.m00 + input.y*mat.m10 + input.z*mat.m20 + input.w*mat.m30;
    vec.y = input.x*mat.m01 + input.y*mat.m11 + input.z*mat.m21 + input.w*mat.m31;
    vec.z = input.x*mat.m02 + input.y*mat.m12 + input.z*mat.m22 + input.w*mat.m32;
    vec.w = input.x*mat.m03 + input.y*mat.m13 + input.z*mat.m23 + input.w*mat.m33;
    return vec;
}

typedef struct {
    float x, y, z;

} fvec3;

inline fvec3 set3(float x, float y, float z) {
    fvec3 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    return ret;
}

typedef struct {
    float x, y, z, w;


} fvec4;

inline fvec4 set4(float x, float y, float z, float w) {
    fvec4 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    ret.w = w;
    return ret;
}

inline fvec4 mulf4(fvec4 input, mat4 mat) {
    fvec4 vec;
    vec.x = input.x*mat.m00 + input.y*mat.m10 + input.z*mat.m20 + input.w*mat.m30;
    vec.y = input.x*mat.m01 + input.y*mat.m11 + input.z*mat.m21 + input.w*mat.m31;
    vec.z = input.x*mat.m02 + input.y*mat.m12 + input.z*mat.m22 + input.w*mat.m32;
    vec.w = input.x*mat.m03 + input.y*mat.m13 + input.z*mat.m23 + input.w*mat.m33;
    return vec;
}

typedef struct {
    uint x, y;
    float z, w;
} ivec4;


typedef union {
    fvec4 f;
    ivec4 i;
} ufvec4;

typedef struct __attribute__((packed)) {
    ufvec4 v0, v1, v2/*, v3*/;
    uint id;
} triplet;

inline float3 mul3(float3 input, mat3 mat) {
    float3 vec;
    vec.x = input.x*mat.m00 + input.y*mat.m10 + input.z*mat.m20;
    vec.y = input.x*mat.m01 + input.y*mat.m11 + input.z*mat.m21;
    vec.z = input.x*mat.m02 + input.y*mat.m12 + input.z*mat.m22;
    return vec;
}