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
    /*Colour result = textureCube(reflection_map, (float3)(camdir.x, camdir.y, camdir.z), fragTX);

    // Write out result
    if (fragFX.reflection_val > 0.0f) {
        fragCol.r = result.r;
        fragCol.g = result.g;
        fragCol.b = result.b;
        fragment_buffer_colour[id] = fragCol;
    }*/

}