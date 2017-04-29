#include "kernels/Textures/Textures.cl"
#include "kernels/Utility/Matrix.cl"

kernel void accumulate_buffers(global FragmentColour* fragment_buffer_colour,
                               global FragmentColour* ssao_buffer_colour,
                               global FragmentColour* shadow_buffer_colour){ 

    uint id = get_global_id(0);
    FragmentColour fragment = fragment_buffer_colour[id];
    FragmentColour ssao     = ssao_buffer_colour[id];
    FragmentColour shadow   = shadow_buffer_colour[id];
    
    // Get initial colour
    float r, g, b;
    r = fragment.r;
    g = fragment.g;
    b = fragment.b;

    // Combine the buffers into a final result:
    r *= (float)(shadow.r) / 255.0f;
    g *= (float)(shadow.g) / 255.0f;
    b *= (float)(shadow.b) / 255.0f;

    // Add specular component
    r += shadow.a;
    g += shadow.a;
    b += shadow.a;

    // Apply SSAO
    r *= (float)(ssao.r) / 255.0f;
    g *= (float)(ssao.g) / 255.0f;
    b *= (float)(ssao.b) / 255.0f;

    // Output result back to main fragment buffer
    fragment.r = clamp((int)(r), 0, 255);
    fragment.g = clamp((int)(g), 0, 255);
    fragment.b = clamp((int)(b), 0, 255);
    fragment_buffer_colour[id] = fragment;
}
