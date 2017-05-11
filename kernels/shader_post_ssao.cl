/// ---------- PROPERTIES -------------
/*
    POWER     - controls the gradient of the AO. (higher intensity inner regions)
    STRENGTH  - controls the overall darkness of the AO. A higher strength will result in more impactful AO, though it will also highlight artifacts more.
    MAX_RANGE - controls the range at which a surface can cast occlusion onto another. If this is too high, you will get an effect called haloing which results in objects casting AO onto things that are far away.
*/
#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

#define POWER 4.1f
#define STRENGTH 800.5f
#define MAX_SAMPLE_RADIUS 2.25f
#define MAX_RANGE 4.5f



kernel void shader_post_ssao(global FragmentWPos* fragment_buffer_wpos,
                             global FragmentNormal* fragment_buffer_normal, 
                             global FragmentTData* fragment_buffer_tdata,

                             global FragmentColour* ssao_buffer_colour,

                             global float3* sample_kernel, 
                             int sample_count, 
                             mat4 MVP_MATRIX, 
                             int screen_width, 
                             int screen_height) {

    uint id = get_global_id(0);
    FragmentWPos   fragWpos   = fragment_buffer_wpos[id];
    FragmentTData  fragTdata  = fragment_buffer_tdata[id];
    FragmentNormal fragNormal = fragment_buffer_normal[id];
    FragmentColour ssao_result;

    /// ------------------- SSAO PROCESS ---------------------------
    if (fragTdata.depth <= 0) { ssao_buffer_colour[id].r = 0; ssao_buffer_colour[id].g = 0; ssao_buffer_colour[id].b = 0; return; }

    // Depth factor
    //float depthfactor = clamp(30.0f / (frag.depth + 0.01f), 0.3f, MAX_SAMPLE_RADIUS);

    // Sample result accumulator
    float sample_result = 0.0f;

    for (int i = 0; i < sample_count; i++) {
        float3 wpos = (float3)(fragWpos.x, fragWpos.y, fragWpos.z);

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
        projected_kernel_vector.x *= (float)screen_width;
        projected_kernel_vector.y *= (float)screen_height;

        // get sample depth
        int2 coord = convert_int2((float2)(projected_kernel_vector.x, projected_kernel_vector.y));
        coord.x = coord.x % screen_width;
        coord.y = coord.y % screen_height;

        int sample_id = coord.x + coord.y * screen_width;
        sample_id = clamp(sample_id, 0, screen_width * screen_height - 1);

        FragmentWPos   sample_fragment_wpos = fragment_buffer_wpos[sample_id];
        FragmentNormal sample_fragment_nml  = fragment_buffer_normal[sample_id];

        // Compare depths
        float4 projected_sample = mul((float4)(sample_fragment_wpos.x, sample_fragment_wpos.y, sample_fragment_wpos.z, 1.0f), MVP_MATRIX);

        // Perspective divide
        projected_sample /= max(projected_sample.w, 0.01f);

        // Re-scale from -1 to 1 to 0 to SW, 
        projected_sample.x *= 0.5f;
        projected_sample.y *= 0.5f;
        projected_sample.x += 0.5f;
        projected_sample.y += 0.5f;
        projected_sample.x *= (float)screen_width;
        projected_sample.y *= (float)screen_height;

        // Normal factor
        float3 nmldiff = (float3)(sample_fragment_nml.nx, sample_fragment_nml.ny, sample_fragment_nml.nz)-(float3)(fragNormal.nx, fragNormal.ny, fragNormal.nz);
        float nmlcheck = clamp(fabs(nmldiff.x) + fabs(nmldiff.y) + fabs(nmldiff.z), 0.25f, 1.0f);

        // If kernel vector depth is greated than the sampled result stored in the array, occlusion occurs. We then have to perform the range check to verify the occluding surface is nearby in world-space
        if (projected_sample.z < projected_kernel_vector.z && length((float3)(sample_fragment_wpos.x - fragWpos.x, sample_fragment_wpos.y - fragWpos.y, sample_fragment_wpos.z - fragWpos.z)) < MAX_RANGE) {
            sample_result += length(sample_kernel[i])*nmlcheck / MAX_SAMPLE_RADIUS;
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
    ssao_buffer_colour[id] = ssao_result;

}





