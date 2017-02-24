#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

kernel void projection(global float3* const vertices, global float3* const screen_space_vertices, const float3 campos) {
    uint id = get_global_id(0);

    screen_space_vertices[id].x = ((vertices[id].x - campos.x) / (vertices[id].z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH;
    screen_space_vertices[id].y = ((vertices[id].y - campos.y) / (vertices[id].z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT;
    
    screen_space_vertices[id].z = fast_distance(vertices[id], campos); //Used for depth-buffering
}