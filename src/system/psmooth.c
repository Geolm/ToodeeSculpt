#include "psmooth.h"
#include <assert.h>

//-----------------------------------------------------------------------------------------------------------------------------
void psmooth_init(struct psmooth* ctx)
{
    ctx->current_index = 0;
    for(unsigned int i=0; i<PSMOOTH_NUM_VALUES; ++i)
        ctx->values[i] = 0.f;
}

//-----------------------------------------------------------------------------------------------------------------------------
void psmooth_push(struct psmooth* ctx, float value)
{
    assert(ctx->current_index < PSMOOTH_NUM_VALUES);
    ctx->values[ctx->current_index] = value;

    // wrap around
    if (++ctx->current_index >= PSMOOTH_NUM_VALUES)
        ctx->current_index = 0;
}

static const float weights[PSMOOTH_NUM_VALUES] = 
    {1.f/262144.f,  19.f/262144.f,  171.f/262144.f, 969.f/262144.f,  3876.f/262144.f, 
     11628.f/262144.f,  27132.f/262144.f,  50388.f/262144.f, 75582.f/262144.f, 92378.f/262144.f};

//-----------------------------------------------------------------------------------------------------------------------------
float psmooth_average(struct psmooth* ctx)
{
    assert(ctx->current_index < PSMOOTH_NUM_VALUES);

    float average = 0.f;
    unsigned int index = ctx->current_index;
    for(unsigned int i=0; i<PSMOOTH_NUM_VALUES; ++i)
    {
        average += ctx->values[index] * weights[i];
        if (++index >= PSMOOTH_NUM_VALUES)
            index = 0;
    }

    return average;
}