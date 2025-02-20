#include "file_buffer.h"
#include <stdio.h>
#include <stdlib.h>

void* read_file(const char* filename, size_t* file_size)
{
    FILE* f = fopen(filename, "rb");
    if (f != NULL)
    {
        fseek(f, 0L, SEEK_END);
        *file_size = ftell(f);
        fseek(f, 0L, SEEK_SET);

        void* buffer = malloc(*file_size);
        if (fread(buffer, *file_size, 1, f) == 1)
            return buffer;

        free(buffer);
    }
    return NULL;
}