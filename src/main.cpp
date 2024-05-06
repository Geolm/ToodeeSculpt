
#include "metal.hpp"
#include "shader_reader.h"
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
//    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    
    
    char* shader = read_shader_include("/Users/geolm/Code/Geolm/GPU2DComposer/src/shaders/", "binning.metal");
    
    printf("%s", shader);
    
    free(shader);

    return 0;
}
