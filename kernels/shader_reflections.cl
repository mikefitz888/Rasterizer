#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

#define NUM_STEPS 48
#define STEP_SIZE 0.55f
#define REFLECTION_INTERSECTION_TOLERANCE 5.0f

kernel void shader_reflections(global FragmentColour* fragment_buffer_colour,
                               global FragmentFX* fragment_buffer_fx,
                               global FragmentNormal* fragment_buffer_normal,
                               global FragmentWPos* fragment_buffer_wpos,
                               global FragmentTX* fragment_buffer_tx,
                               global FragmentTData* fragment_buffer_tdata,

                               global Colour* reflection_map,
                               fvec3 campos,
                               fvec3 camdir,
                               int screen_width,
                               int screen_height,
                               mat4 MVP_MATRIX) {

    // Get global information
    uint id = get_global_id(0);
    FragmentColour fragCol  = fragment_buffer_colour[id];
    FragmentWPos   fragWpos = fragment_buffer_wpos[id];
    FragmentNormal fragNml  = fragment_buffer_normal[id];
    FragmentFX     fragFX   = fragment_buffer_fx[id];
    FragmentTX     fragTX   = fragment_buffer_tx[id];
    FragmentTData  fragTD   = fragment_buffer_tdata[id];

    if (fragTD.depth <= 0) {
        return;
    }

    // Write out result
    if (fragFX.reflection_val > 0.0f) {
        /*
            If surface is reflective, we calculate the reflection vector and sample the cubemap
        
        */
        //////////////////////////////////////////////////////////////////
        // Attempt Screen space reflections (SSR)
        //
        // Algorithm:
        //  vec3 viewVec = |wpos-campos|
        //  vec3 step_vec = reflect(viewVec, Normal); 
        //
        //  vec3 cpos = wpos
        //  for(steps):
        //    cpos += step_vec*STEP_SIZE
        //    vec2 cproj = project2D(cpos)
        //    sampleWpos = fragment_buffer_wpos[cproj.x + cproj.y*WIDTH]
        //    
        //    if( |sampleWpos-campos| < |cpos-campos| ):
        //          we''ve intersected, so set result = fragment_buffer_colour[...]
        //          result_found = true
        //          break;
        //
        Colour result;
        bool result_found = false;

        // Calculate reflection vector
        float3 world_pos = (float3)(fragWpos.x, fragWpos.y, fragWpos.z);
        float3 cam_pos = (float3)(campos.x, campos.y, campos.z);

        float3 view_vec = world_pos - cam_pos;
        float3 normal_v = (float3)(fragNml.nx, fragNml.ny, fragNml.nz);
        view_vec = normalize(view_vec);
        normal_v = normalize(normal_v);
        float3 refl_vec = view_vec - 2.0f*dot(view_vec, normal_v)*normal_v;

        // Perform step process
        float3 cpos = world_pos;
        for (int i = 0; i < NUM_STEPS; i++ ){ 
            cpos += refl_vec*STEP_SIZE;
            
            // Project sample pos
            float4 projected_kernel_vector = mul((float4)(cpos.x, cpos.y, cpos.z, 1.0f), MVP_MATRIX);
            projected_kernel_vector /= max(projected_kernel_vector.w, 0.01f);
            projected_kernel_vector.x *= 0.5f;
            projected_kernel_vector.y *= 0.5f;
            projected_kernel_vector.x += 0.5f;
            projected_kernel_vector.y += 0.5f;
            projected_kernel_vector.x *= (float)screen_width;
            projected_kernel_vector.y *= (float)screen_height;

            int2 coord = convert_int2((float2)(projected_kernel_vector.x, projected_kernel_vector.y));
            coord.x = coord.x % screen_width;
            coord.y = coord.y % screen_height;

            // Get sample world pos
            int sample_id = coord.x + coord.y * screen_width;
            if (coord.x < 0 || coord.x >= screen_width || coord.y < 0 || coord.y >= screen_height) {
                break;
            }
            //sample_id = clamp(sample_id, 0, screen_width * screen_height - 1);
            FragmentWPos sample_fragment_wpos = fragment_buffer_wpos[sample_id];
            FragmentTData sample_tdata = fragment_buffer_tdata[sample_id];

            // Ensure data exists where we are sampling
            if (sample_tdata.depth <= 0) {
                continue;
            }

            // Perform intersection test
            float3 sample_pos = (float3)(sample_fragment_wpos.x, sample_fragment_wpos.y, sample_fragment_wpos.z);

            // if our sampled position is closer to the camera than our ray marched position, then the ray has intersected.
            //  - as an additional measure, we want to verify that the rays are sufficiently close together, so that this is definitely an accurate sample, and
            //  - not a sample on some hovering object
            if ((length(sample_pos - cam_pos) <= length(cpos - cam_pos)) && length(sample_pos-cpos) < REFLECTION_INTERSECTION_TOLERANCE) {
                result.r = fragment_buffer_colour[sample_id].r;
                result.g = fragment_buffer_colour[sample_id].g;
                result.b = fragment_buffer_colour[sample_id].b;
                result_found = true;
                break;
            }
        }

        //////////////////////////////////////////////////////////////////
        // If SSR failed, just sample the cubemap
        if (!result_found) {
            // Sample cubemap for result
            result = textureCube(reflection_map, refl_vec, fragTX);
            //fragFX.reflection_val = 0.0f;
        }

    
        fragCol.r = result.r*fragFX.reflection_val + fragCol.r*(1.0f- fragFX.reflection_val);
        fragCol.g = result.g*fragFX.reflection_val + fragCol.g*(1.0f - fragFX.reflection_val);
        fragCol.b = result.b*fragFX.reflection_val + fragCol.b*(1.0f - fragFX.reflection_val);
        fragment_buffer_colour[id] = fragCol;
    }

}