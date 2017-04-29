#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

kernel void shader_point_light( global FragmentWPos* fragment_buffer_wpos,
                                global FragmentNormal* fragment_buffer_normal,
                                global FragmentTData* fragment_buffer_tdata,
                                global FragmentColour* light_accum_buffer,
                                fvec3 campos,
                                fvec3 lightpos,
                                fvec3 lightcolour,
                                float light_range,
                                float specularity,
                                float glossiness,
                                int swidth) {
    
    // Fetch ID
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Get Fragment
    FragmentWPos   fragWpos = fragment_buffer_wpos[x + y*swidth];
    FragmentNormal fragNml  = fragment_buffer_normal[x + y*swidth];
    FragmentTData  fragTD   = fragment_buffer_tdata[x + y*swidth];
    FragmentColour frag_light = light_accum_buffer[x + y*swidth];

    // Discard un-rendered fragments
    if (fragTD.depth <= 0) {
        frag_light.r = 0;
        frag_light.g = 0;
        frag_light.b = 0;
        frag_light.a = 0;
        light_accum_buffer[x + y*swidth] = frag_light;
        return;
    }

    // Get light dir
    float3 lightdir = (float3)(fragWpos.x - lightpos.x, fragWpos.y - lightpos.y, fragWpos.z - lightpos.z);
    float dist = length(lightdir);
    lightdir = normalize(lightdir);

    // Perform lighting calculations:
    float3 N = (float3)(fragNml.nx, fragNml.ny, fragNml.nz); // Surface normal [WORLD SPACE]
    float3 L = normalize((float3)(lightdir.x, lightdir.y, lightdir.z)); // Direction of light [WORLD SPACE]
    float lambert = clamp(dot(N, L), 0.0f, 1.0f);
    float strength = clamp(1.0f - dist / light_range, 0.0f, 1.0f);
    float diffuse = strength*lambert * 255.0f;

    // Accumulate lighting
    frag_light.r = (int)(clamp(frag_light.r + diffuse*lightcolour.x, 0.0f, 255.0f));
    frag_light.g = (int)(clamp(frag_light.g + diffuse*lightcolour.y, 0.0f, 255.0f));
    frag_light.b = (int)(clamp(frag_light.b + diffuse*lightcolour.z, 0.0f, 255.0f));

    //// --- Specular factor ---
    // NOTE: Specular wont work for coloured lights atm as the specular buffer only has a single channel
    // Blinn-phong
   /* float3 wpos = (float3)(frag.x, frag.y, frag.z);
    float3 E = normalize(wpos - (float3)(campos.x, campos.y, campos.z)); // View direction
    float3 halfwayVector = normalize(L + E);*/

    //float3 R = reflect(L, N);
    //float3 V = normalize(vertex);
   // float spec = clamp(pow(max(dot(N, halfwayVector), 0.0f), glossiness)*specularity, 0.0f, 1.0f);

    // Accum factors
    //frag_light.a = (int)(clamp(frag_light.a + spec*255.0f *  (float)(frag.a / 255.0f), 0.0f, 255.0f));

    // Write out result
    light_accum_buffer[x + y*swidth] = frag_light;
}