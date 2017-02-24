#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 480

kernel void projection(global float3* const vertices, global float3* const screen_space_vertices, const float3 campos) {
    uint id = get_global_id(0);

    screen_space_vertices[id].x = ((vertices[id].x - campos.x) / (vertices[id].z - campos.z) + 1.f) * 0.5f * SCREEN_WIDTH;
    screen_space_vertices[id].y = ((vertices[id].y - campos.y) / (vertices[id].z - campos.z) + 1.f) * 0.5f * SCREEN_HEIGHT;
    //vertices[id].z is unchanged as this can be used later for depth buffering
}