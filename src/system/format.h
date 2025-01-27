#ifndef __FORMAT__H__
#define __FORMAT__H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// this function is not threadsafe as it uses the only one global buffer
const char* format(const char* string, ...);


struct string_buffer
{
    char* buffer;
    size_t size;
    char* current;
    size_t remaining;
};

struct string_buffer string_buffer_init(size_t size);
void bprintf(struct string_buffer* b, const char* string, ...);
void string_buffer_terminate(struct string_buffer* b);

#ifdef __cplusplus
}
#endif


#endif