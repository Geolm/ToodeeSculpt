#include "../renderer/shader_reader.h"
#include "../system/bin2h.h"
#include "../system/format.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UNUSED_VARIABLE(a) (void)(a)
#define SHADERS_PATH "../src/shaders/"

// ---------------------------------------------------------------------------------------------------------------------------
bool bin_shader(const char* shader_name, const char* extension)
{
    bool result = false;
    char* shader_buffer = read_shader_include(SHADERS_PATH, format("%s.%s", shader_name, extension));

    fprintf(stdout, "opening %s%s.%s ", SHADERS_PATH, shader_name, extension);

    if (shader_buffer != NULL)
    {
        fprintf(stdout, "ok\n");
        char* shader_fullname = strdup(format("%s%s.h", SHADERS_PATH, shader_name));
        fprintf(stdout, "writting %s ", shader_fullname);
        result = bin2h(shader_fullname, format("%s_shader", shader_name), shader_buffer, strlen(shader_buffer)+1);
        fprintf(stdout, result ? "ok\n" : "error\n");

        free(shader_buffer);
        free(shader_fullname);
    }
    else
        fprintf(stdout, "error\n");

    return result;
}

// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    fprintf(stdout, "binarize shaders\n");

    if (!bin_shader("binning", "metal"))
        return -1;

    if (!bin_shader("rasterizer", "metal"))
        return -1;

    if (!bin_shader("shadertoy_boilerplate", "glsl"))
        return -1;

    fprintf(stdout, "\n\nall good");

    return 0;
}