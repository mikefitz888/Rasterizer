#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

// Properties
/*
    SHADOW BIAS - controls the depth offset check to avoid shadow acne caused by equal depth tests
    PCF_SAMPLES - the number of samples to take per shadow pixel (results in soft/smooth shadows)
    PCF_SAMPLE_RADIUS -  the distance at which the samples should be taken from the centre
*/
#define SHADOW_BIAS 0.0001
#define PCF_SAMPLES 4
#define PCF_SAMPLE_RADIUS 2

kernel void shadows_directional(global Fragment* fragment_buffer,
                 global Fragment* light_buffer,
                 global Fragment* shadow_buffer,
                 mat4 LIGHT_VIEW_PROJECTION_MATRIX,
                 int sb_width, 
                 int sb_height) {

    /*
        Algorithm:
        - get point at given pixel (from point of view of camera)
        - project point into light space, so we can work out the 2D point in the light buffer
        - Sample the same 2D point in light buffer.
        - Compare the depths of the two points, if the sampled one has a smaller depth than the projected one,
            this means the projected point is obscured by some object and thus the point is in shadow
    
    */
    
    // Fetch fragment we are testing
    uint id = get_global_id(0);
    Fragment frag = fragment_buffer[id];
    Fragment shadow_result;

    // Discard un-rendered fragments
    if(frag.depth <= 0) { 
        shadow_result.r = 0; 
        shadow_result.g = 0; 
        shadow_result.b = 0; 
        shadow_buffer[id] = shadow_result; 
        return; 
    }

    // Transform fragment position into light-space
    float4 world_pos = (float4)(frag.x, frag.y, frag.z, 1.0f);
    float4 light_space_proj = mul(world_pos, LIGHT_VIEW_PROJECTION_MATRIX);
    
    // Perspective divide
    light_space_proj /= max(light_space_proj.w, 0.01f);

    // Re-scale from -1 to 1 to 0 to SW, 
    light_space_proj.x *= 0.5f;
    light_space_proj.y *= 0.5f;
    light_space_proj.x += 0.5f;
    light_space_proj.y += 0.5f;
    light_space_proj.x *= (float)sb_width;
    light_space_proj.y *= (float)sb_height;

    //// Sample point in light buffer:
    // get sample depth
    int2 coord = convert_int2((float2)(light_space_proj.x, light_space_proj.y));


    float accum_result = 0.0f;
    for (int ii = -PCF_SAMPLES/2; ii < PCF_SAMPLES/2; ii++) {
        for (int jj = -PCF_SAMPLES / 2; jj < PCF_SAMPLES/2; jj++) {
            int2 sample_coord;
            sample_coord.x = clamp(coord.x+ii*PCF_SAMPLE_RADIUS, 0, sb_width - 1);//coord.x % sb_width;
            sample_coord.y = clamp(coord.y+jj*PCF_SAMPLE_RADIUS, 0, sb_height - 1);//coord.y % sb_height;

            int sample_id = sample_coord.x + sample_coord.y * sb_width;
            sample_id = clamp(sample_id, 0, sb_width * sb_height - 1);
            Fragment sample_fragment = light_buffer[sample_id];

            // Compare depths
            float4 projected_sample = mul((float4)(sample_fragment.x, sample_fragment.y, sample_fragment.z, 1.0f), LIGHT_VIEW_PROJECTION_MATRIX);

            // Perspective divide
            projected_sample /= max(projected_sample.w, 0.01f);

            // Re-scale from -1 to 1 to 0 to SW, 
            projected_sample.x *= 0.5f;
            projected_sample.y *= 0.5f;
            projected_sample.x += 0.5f;
            projected_sample.y += 0.5f;
            projected_sample.x *= (float)sb_width;
            projected_sample.y *= (float)sb_height;

            // If sample z is smaller than projected z, point is in shadow. (We also verify that projected sample depth is larger than 0)
            if (projected_sample.z < light_space_proj.z-SHADOW_BIAS && projected_sample.z > 0) {
            //if (sample_fragment.depth < /*light_space_proj.z*/depth - SHADOW_BIAS && sample_fragment.depth > 0) {
                // In Shadow
                accum_result += 0.75f;

            }
        }
    }
    // Divide result
    accum_result /= PCF_SAMPLES*PCF_SAMPLES;
    accum_result = 1.0f - accum_result;

    shadow_result.r = (int)(accum_result * 255.0f);
    shadow_result.g = (int)(accum_result * 255.0f);
    shadow_result.b = (int)(accum_result * 255.0f);

    // Store result
    shadow_buffer[id] = shadow_result;
}
