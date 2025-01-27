#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>
#include <stddef.h>

struct primitive;

#ifdef __cplusplus
extern "C" {
#endif

char* copy_boiler_plate(char* clipboard_buffer, size_t* remaining_size);
void primitive_export_shadertoy(char** clipboard_buffer, size_t* remaining_size, struct primitive * const p, uint32_t index, float smooth_value);
void finish_shadertoy(char** clipboard_buffer, size_t* remaining_size);


#ifdef __cplusplus
}
#endif

#endif