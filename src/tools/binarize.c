#include "../renderer/shader_reader.h"
#include "../system/bin2h.h"
#include "../system/file_buffer.h"
#include "../system/format.h"
#include "../system/spng.h"
#include "bc4_encoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UNUSED_VARIABLE(a) (void)(a)
#define SHADERS_PATH "../src/shaders/"
#define IMAGES_PATH "../images/"

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
    size_t file_size, raw_image_size;
    void* buffer = read_file(format("%s%s.png", IMAGES_PATH, font_filename), &file_size);

    if (buffer != NULL)
    {
        fprintf(stdout, "opening \"%s%s\" size : %zu bytes\n", IMAGES_PATH, font_filename, file_size);
        spng_ctx *ctx = spng_ctx_new(0);

        if (spng_set_png_buffer(ctx, buffer, file_size))
            return false;

        struct spng_ihdr ihdr;
        if (spng_get_ihdr(ctx, &ihdr))
        {
            fprintf(stdout, "can't get PNG info\n");
            return false;
        }

        fprintf(stdout, "\t%ux%u %u bit depth image\n", ihdr.width, ihdr.height, ihdr.bit_depth);

        if (ihdr.color_type != SPNG_COLOR_TYPE_GRAYSCALE || ihdr.bit_depth != 8)
        {
            fprintf(stdout, "wrong format, only support 8 bits grayscale image\n");
            return false;
        }

        if (ihdr.width%4 || ihdr.height%4)
        {
            fprintf(stdout, "not supported image dimensions\n");
            return false;
        }

        if (spng_decoded_image_size(ctx, SPNG_FMT_G8, &raw_image_size))
        {
            fprintf(stdout, "can't get image size\n");
            return false;
        }

        uint8_t* raw_image = (uint8_t*) malloc(raw_image_size);
        if (spng_decode_image(ctx, raw_image, raw_image_size, SPNG_FMT_G8, 0))
        {
            fprintf(stdout, "error while decoding image\n");
            free(raw_image);
            return false;
        }

        fprintf(stdout, "\tcompressing image in BC4\n");

        size_t bc4_image_size = (ihdr.width/4 * ihdr.height/4) * 8;
        uint8_t* bc4_image = malloc(bc4_image_size);

        bc4_encode(raw_image, bc4_image, ihdr.width, ihdr.height);

        fprintf(stdout, "%s", format("writting \"../src/renderer/%s.h\"", font_filename));

        bool result = bin2h(format("../src/renderer/%s.h", font_filename), font_filename, bc4_image, bc4_image_size);

        free(bc4_image);
        free(raw_image);
        spng_ctx_free(ctx);

        return result;
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

    if (!bin_font("commitmono_21_31"))
        return false;

    fprintf(stdout, "\n\nall good");

    return 0;
}