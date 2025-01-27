#include "format.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define FORMAT_BUFFER_SIZE (2048)
char buffer[FORMAT_BUFFER_SIZE];

//-----------------------------------------------------------------------------
const char* format(const char* string, ...)
{
    va_list args;
    va_start(args, string);
    vsnprintf(buffer, FORMAT_BUFFER_SIZE, string, args);
    va_end(args);
    return buffer;
}

//-----------------------------------------------------------------------------
struct string_buffer string_buffer_init(size_t size)
{
    struct string_buffer b;

    b.buffer = (char*) malloc(sizeof(char) * size);
    b.size = size;
    b.current = b.buffer;
    b.remaining = size;

    return b;
}

//-----------------------------------------------------------------------------
void bprintf(struct string_buffer* b, const char* string, ...)
{
    va_list args;
    va_start(args, string);
    int num_char_written = vsnprintf(b->current, b->remaining, string, args);
    va_end(args);

    if (num_char_written > 0)
    {
        b->current += num_char_written;
        b->remaining -= num_char_written;
    }
}

//-----------------------------------------------------------------------------
void string_buffer_terminate(struct string_buffer* b)
{
    free(b->buffer);
}

