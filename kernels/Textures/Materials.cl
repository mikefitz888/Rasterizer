#ifndef _MATERIALS_CLH
#define _MATERIALS_CLH

/*
    Defines material structure

*/

#include "Textures.cl"

typedef struct __attribute__((packed)) {

    // Textures
    Colour* diffuse_texture;
    Colour* normalmap_texture;
    Colour* specular_texture;

    // Properties
    float specularity = 0.0f;
    float glossiness = 0.0f;
    float reflectivity = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

} Material;



#endif