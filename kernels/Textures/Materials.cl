#ifndef _MATERIALS_CLH
#define _MATERIALS_CLH

/*
    Defines material structure

*/

#include "Textures.cl"

typedef struct __attribute__((packed)) {

    // Properties
    float specularity;
    float glossiness;
    float reflectivity;
    float offset_strength;
    float r;
    float g;
    float b;

} MaterialProperties;



#endif