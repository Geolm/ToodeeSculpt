#include "format.h"
#include <stdarg.h>
#include <stdio.h>

#define FORMAT_BUFFER_SIZE (2048)

char buffer[FORMAT_BUFFER_SIZE];

const char* format(const char* string, ...)
{
    va_list args;
    va_start(args, string);
    vsnprintf(buffer, FORMAT_BUFFER_SIZE, string, args);
    va_end(args);
    return buffer;
}
