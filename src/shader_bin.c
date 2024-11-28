#include "renderer/shader_reader.h"
#include "system/bin2h.h"
#include "system/format.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UNUSED_VARIABLE(a) (void)(a)

// ---------------------------------------------------------------------------------------------------------------------------
bool bin_shader(const char* shader_name)
{
    bool result = false;
    char* shader_buffer = read_shader_include("./src/shaders/", format("%s.metal", shader_name));

    fprintf(stdout, "opening ./src/shaders/%s.metal ", shader_name);

    if (shader_buffer != NULL)
    {
        fprintf(stdout, "ok\n");
        fprintf(stdout, "writting ./src/shaders/%s.h ", shader_name);
        result = bin2h(format("./src/shaders/%s.h", shader_name), format("%s_shader", shader_name), shader_buffer, strlen(shader_buffer));
        fprintf(stdout, result ? "ok\n" : "error\n");
        free(shader_buffer);
    }

    return result;
}

// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    fprintf(stdout, "shader_bin\n");

    if (!bin_shader("binning"))
        return -1;

    if (!bin_shader("rasterizer"))
        return -1;

    fprintf(stdout, "\n\nall good");

    return 0;
}