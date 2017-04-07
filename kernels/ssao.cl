/// ---------- PROPERTIES -------------
/*
    POWER     - controls the gradient of the AO. (higher intensity inner regions)
    STRENGTH  - controls the overall darkness of the AO. A higher strength will result in more impactful AO, though it will also highlight artifacts more.
    MAX_RANGE - controls the range at which a surface can cast occlusion onto another. If this is too high, you will get an effect called haloing which results in objects casting AO onto things that are far away.
*/
#include "kernels/Textures/Textures.cl"
#define POWER 3.5f
#define STRENGTH 175.5f
#define MAX_SAMPLE_RADIUS 1.25f
#define MAX_RANGE 4.5f

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

/*inline float3 mul3(float3 input, mat3 mat) {
    float3 vec;
    vec.x = input.x*mat.m00 + input.y*mat.m10 + input.z*mat.m20;
    vec.y = input.x*mat.m01 + input.y*mat.m11 + input.z*mat.m21;
    vec.z = input.x*mat.m02 + input.y*mat.m12 + input.z*mat.m22;
    return vec;
}
*/

/*typedef struct __attribute__((packed, aligned(4))) {
    float x, y, z;
} vec3;*/

kernel void ssao(global Fragment* fragment_buffer, global Fragment* ssao_buffer, global float3* sample_kernel, int sample_count, mat4 MVP_MATRIX) {
    uint id = get_global_id(0);
    Fragment frag = fragment_buffer[id];
    Fragment ssao_result = ssao_buffer[id];

    /// ------------------- SSAO PROCESS ---------------------------
    if (frag.depth <= 0) { ssao_buffer[id].r = 0; ssao_buffer[id].g = 0; ssao_buffer[id].b = 0; return; }

    // Depth factor
    //float depthfactor = clamp(30.0f / (frag.depth + 0.01f), 0.3f, MAX_SAMPLE_RADIUS);

    // Sample result accumulator
    float sample_result = 0.0f;

    for (int i = 0; i < sample_count; i++) {
        float3 wpos = (float3)(frag.x, frag.y, frag.z);

        // Perform sample
        wpos += sample_kernel[i]/*depthfactor*/; // Re-scale

        // Project sample pos
        float4 projected_kernel_vector = mul((float4)(wpos.x, wpos.y, wpos.z, 1.0f), MVP_MATRIX);

        // Perspective divide
        projected_kernel_vector /= max(projected_kernel_vector.w, 0.01f);

        // Re-scale from -1 to 1 to 0 to SW, 
        projected_kernel_vector.x *= 0.5f;
        projected_kernel_vector.y *= 0.5f;
        projected_kernel_vector.x += 0.5f;
        projected_kernel_vector.y += 0.5f;
        projected_kernel_vector.x *= 600.0f;
        projected_kernel_vector.y *= 480.0f;

        // get sample depth
        int2 coord = convert_int2((float2)(projected_kernel_vector.x, projected_kernel_vector.y));
        coord.x = coord.x % 600;
        coord.y = coord.y % 480;

        int sample_id = coord.x + coord.y * 600;
        sample_id = clamp(sample_id, 0, 600 * 480 - 1);
        Fragment sample_fragment = fragment_buffer[sample_id];

        // Compare depths
        float4 projected_sample = mul((float4)(sample_fragment.x, sample_fragment.y, sample_fragment.z, 1.0f), MVP_MATRIX);

        // Perspective divide
        projected_sample /= max(projected_sample.w, 0.01f);

        // Re-scale from -1 to 1 to 0 to SW, 
        projected_sample.x *= 0.5f;
        projected_sample.y *= 0.5f;
        projected_sample.x += 0.5f;
        projected_sample.y += 0.5f;
        projected_sample.x *= 600.0f;
        projected_sample.y *= 480.0f;

        // Normal factor
        float3 nmldiff = (float3)(sample_fragment.nx, sample_fragment.ny, sample_fragment.nz)-(float3)(frag.nx, frag.ny, frag.nz);
        float nmlcheck = clamp(fabs(nmldiff.x) + fabs(nmldiff.y) + fabs(nmldiff.z), 0.25f, 1.0f);

        // If kernel vector depth is greated than the sampled result stored in the array, occlusion occurs. We then have to perform the range check to verify the occluding surface is nearby in world-space
        if (projected_sample.z < projected_kernel_vector.z && length((float3)(sample_fragment.x - frag.x, sample_fragment.y - frag.y, sample_fragment.z - frag.z)) < MAX_RANGE) {
            sample_result += length(sample_kernel[i])*nmlcheck;
        }    

    }
    // Normalize result
    sample_result /= (float)sample_count;

    sample_result = clamp(pow(sample_result, POWER)*STRENGTH, 0.0f, 1.0f);//3.8f // Power improves the gradient, strength (the 2.5f) improves the intensity
    // Invert
    sample_result = 1.0f - sample_result;

    ssao_result.r = (int)(sample_result * 255.0f);
    ssao_result.g = (int)(sample_result * 255.0f);
    ssao_result.b = (int)(sample_result * 255.0f);

    // Write out result
    ssao_buffer[id] = ssao_result;

}





