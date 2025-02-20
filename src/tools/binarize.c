#include "../renderer/shader_reader.h"
#include "../system/bin2h.h"
#include "../system/file_buffer.h"
#include "../system/format.h"
#include "../system/spng.h"
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
bool bin_font(const char* font_filename)
{
    size_t file_size, compressed_image_size;
    void* buffer = read_file(font_filename, &file_size);

    if (buffer != NULL)
    {
        fprintf(stdout, "opening ""%s"" size : %d bytes\n", font_filename, file_size);
        spng_ctx *ctx = spng_ctx_new(0);

        if (spng_set_png_buffer(ctx, buffer, file_size))
            return false;

        struct spng_ihdr ihdr;
        if (spng_get_ihdr(ctx, &ihdr))
        {
            fprintf(stdout, "can't get PNG info\n");
            return false;
        }

        fprintf(stdout, "\t%dx%d %d bit depth image\n", ihdr.width, ihdr.height, ihdr.bit_depth);

        if (ihdr.color_type != SPNG_COLOR_TYPE_GRAYSCALE || ihdr.bit_depth != 8)
        {
            fprintf(stdout, "wrong format, only support 8 bits grayscale image\n");
            return false;
        }

        if (spng_decoded_image_size(ctx, SPNG_FMT_G8, &compressed_image_size))
        {
            fprintf(stdout, "can't get image size\n");
            return false;
        }

        uint8_t* compressed_image = (uint8_t*) malloc(compressed_image_size);
        if (spng_decode_image(ctx, compressed_image, compressed_image_size, SPNG_FMT_G8, 0))
        {
            fprintf(stdout, "error while decoding image\n");
            free(compressed_image);
            return false;
        }

        

        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    fprintf(stdout, "binarizing shaders\n\n");

    if (!bin_shader("binning", "metal"))
        return -1;

    if (!bin_shader("rasterizer", "metal"))
        return -1;

    if (!bin_shader("shadertoy_boilerplate", "glsl"))
        return -1;

    fprintf(stdout, "\nimporting and compressing font\n\n");

    if (!bin_font("../images/commitmono_21_31.png"))
        return -1;

    fprintf(stdout, "\n\nall good");

    return 0;
}