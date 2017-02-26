typedef struct __attribute__((packed, aligned(4))) {
    uchar r, g, b, a;
    uint triangle_id;
    float depth; //If depth == 0, fragment is not initialized
} Fragment;

typedef struct __attribute__((packed)) { 
    float r, g, b, a;
} Material;

kernel void shaedars(global Fragment* const fragment_buffer, global Material* const material_buffer/*, global uint* const id_to_material_map*/){
    uint id = get_global_id(0);
    uint td = fragment_buffer[id].triangle_id;
   
    if (fragment_buffer[id].depth == 0) return;
    //if (fragment_buffer[id].r == 255) fragment_buffer[id].b = 255;
    fragment_buffer[id].r = (material_buffer[td].r * 255.f);
    fragment_buffer[id].g = (material_buffer[td].g * 255.f);
    fragment_buffer[id].b = (material_buffer[td].b * 255.f);
}