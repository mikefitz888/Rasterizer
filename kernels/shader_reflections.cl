#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

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
                               int screen_height) {

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

    // Do stuff
    //float3 camdir_v = (float3)(camdir.x, camdir.y, camdir.z);
    float3 world_pos = (float3)(fragWpos.x, fragWpos.y, fragWpos.z);
    float3 cam_pos = (float3)(campos.x, campos.y, campos.z);

    float3 view_vec = world_pos - cam_pos;
    float3 normal_v = (float3)(fragNml.nx, fragNml.ny, fragNml.nz);
    view_vec = normalize(view_vec);
    normal_v = normalize(normal_v);
    float3 refl_vec = view_vec - 2.0f*dot(view_vec, normal_v)*normal_v;
    
   // float3 refl_vec = reflect((float3)(camdir.x, camdir.y, camdir.z), (float3)(fragNml.nx, fragNml.ny, fragNml.nz));
    Colour result = textureCube(reflection_map, refl_vec, fragTX);

    // Write out result
    if (fragFX.reflection_val > 0.0f) {
        fragCol.r = result.r*fragFX.reflection_val + fragCol.r*(1.0f- fragFX.reflection_val);
        fragCol.g = result.g*fragFX.reflection_val + fragCol.g*(1.0f - fragFX.reflection_val);
        fragCol.b = result.b*fragFX.reflection_val + fragCol.b*(1.0f - fragFX.reflection_val);
        fragment_buffer_colour[id] = fragCol;
    }

}