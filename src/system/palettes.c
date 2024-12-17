#include "palettes.h"
#include <stdio.h>
#include <stdlib.h>

struct palette* palette_load_from_hex(const char* filename)
{
    FILE* f = fopen(filename, "r");

    if (f == NULL)
        return NULL;

    struct palette* output = (struct palette*) malloc(sizeof(struct palette));
    output->num_entries = 0;
    
    // first pass count the number of entries
    uint32_t dummy;
    int scanf_result = fscanf(f, "%x", &dummy);
    
    while (scanf_result)
    {
        output->num_entries++;
        scanf_result = fscanf(f, "%x", &dummy);
    }

    if (output->num_entries == 0)
    {
        free(output);
        fclose(f);
        return NULL;
    }

    output->entries = (uint32_t*) malloc(sizeof(uint32_t) * output->num_entries);
    rewind(f);

    for(uint32_t i=0; i<output->num_entries; ++i)
        fscanf(f, "%x", &output->entries[i]);
    
    fclose(f);

    return output;
}