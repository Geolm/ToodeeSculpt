#include "palettes.h"
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

//-----------------------------------------------------------------------------
void palette_default(struct palette* output)
{
    output->num_entries = 16;
    output->entries = (uint32_t*) malloc(sizeof(uint32_t) * output->num_entries);
    output->entries[0] = na16_light_grey;
    output->entries[1] = na16_dark_grey;
    output->entries[2] = na16_dark_brown;
    output->entries[3] = na16_brown;
    output->entries[4] = na16_light_brown;
    output->entries[5] = na16_light_green;
    output->entries[6] = na16_green;
    output->entries[7] = na16_dark_green;
    output->entries[8] = na16_orange;
    output->entries[9] = na16_red;
    output->entries[10] = na16_pink;
    output->entries[11] = na16_purple;
    output->entries[12] = na16_light_blue;
    output->entries[13] = na16_blue;
    output->entries[14] = na16_dark_blue;
    output->entries[15] = na16_black;
}

//-----------------------------------------------------------------------------
bool palette_load_from_hex(const char* filename, struct palette* output)
{
    FILE* f = fopen(filename, "r");

    log_debug("trying to open '%s'", filename);

    if (f == NULL)
    {
        log_debug("unable to open file");
        return false;
    }

    output->num_entries = 0;

    // first pass count the number of entries
    uint32_t dummy;
    int scanf_result = fscanf(f, "%x", &dummy);
    
    while (scanf_result==1)
    {
        output->num_entries++;
        scanf_result = fscanf(f, "%x", &dummy);
    }

    if (output->num_entries == 0)
    {
        log_debug("unable to read palette");
        fclose(f);
        return false;
    }

    log_debug("found %d palette entries", output->num_entries);

    output->entries = (uint32_t*) malloc(sizeof(uint32_t) * output->num_entries);
    rewind(f);

    for(uint32_t i=0; i<output->num_entries; ++i)
    {
        uint32_t raw_color;
        fscanf(f, "%x", &raw_color);
        output->entries[i] = (raw_color&0xff)<<16;
        output->entries[i] |= (raw_color>>16)&0xff;
        output->entries[i] |= raw_color&0xff00ff00;
    }

    fclose(f);
    return true;
}

//-----------------------------------------------------------------------------
void palette_serialize(serializer_context* context, struct palette* p)
{
    serializer_write_uint32_t(context, p->num_entries);
    serializer_write_blob(context, p->entries, sizeof(uint32_t) * p->num_entries);
}

//-----------------------------------------------------------------------------
void palette_deserialize(serializer_context* context, struct palette* p)
{
    uint32_t num_entries = serializer_read_uint32_t(context);
    if (num_entries != p->num_entries)
    {
        free(p->entries);
        p->num_entries = num_entries;
        p->entries = (uint32_t*) malloc(sizeof(uint32_t) * num_entries);
    }
    serializer_read_blob(context, p->entries, sizeof(uint32_t) * num_entries);
}

//-----------------------------------------------------------------------------
void palette_free(struct palette* p)
{
    p->num_entries = 0;
    free(p->entries);
    p->entries = NULL;
}