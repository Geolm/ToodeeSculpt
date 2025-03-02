#include "undo.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct undo_state
{
    size_t start_position;
    size_t size;
};

struct undo_context
{
    uint8_t* buffer;
    size_t buffer_size;
    size_t current_position;

    struct undo_state* states;
    uint32_t num_states;
    uint32_t max_states;
};

#define UNUSED_VARIABLE(a) (void)(a)

//-----------------------------------------------------------------------------------------------------------------------------
struct undo_context* undo_init(size_t buffer_size, uint32_t max_states)
{
    struct undo_context* context = (struct undo_context*) malloc(sizeof(struct undo_context));
    context->buffer = (uint8_t*) malloc(buffer_size);
    context->buffer_size = buffer_size;
    context->current_position = 0;
    context->max_states = max_states;
    context->states = (struct undo_state*) malloc(max_states * sizeof(struct undo_state));
    context->num_states = 0;
    return context;
}

//-----------------------------------------------------------------------------------------------------------------------------
void undo_increase_buffer(struct undo_context* context)
{
    context->buffer_size *= 2;
    context->buffer = (uint8_t*) realloc(context->buffer, context->buffer_size);
}

//-----------------------------------------------------------------------------------------------------------------------------
void* undo_begin_snapshot(struct undo_context* context, size_t* max_size)
{
    assert(context->current_position <= context->buffer_size);

    if (context->num_states >= context->max_states)
    {
        context->max_states *= 2;
        context->states = (struct undo_state*) realloc(context->states, context->max_states * sizeof(struct undo_state));
    }

    if (context->current_position == context->buffer_size)
        undo_increase_buffer(context);

    *max_size = context->buffer_size - context->current_position;
    return &context->buffer[context->current_position];
}

//-----------------------------------------------------------------------------------------------------------------------------
void undo_end_snapshot(struct undo_context* context, void* data, size_t size)
{
    UNUSED_VARIABLE(data);
    assert(data >= (void*)&context->buffer[context->current_position]);
    assert(data < (void*)&context->buffer[context->buffer_size]);
    assert(context->num_states < context->max_states);
    struct undo_state* state = &context->states[context->num_states++];
    state->start_position = context->current_position;
    state->size = size;
    context->current_position += size;
}

//-----------------------------------------------------------------------------------------------------------------------------
void* undo_undo(struct undo_context* context, size_t* output_size)
{
    // we need at least two states to backup one
    if (context->num_states > 1)
    {
        context->num_states--;
        context->current_position = context->states[context->num_states].start_position;

        // get the previous state to be restored
        struct undo_state* state = &context->states[context->num_states-1];
        *output_size = state->size;
        return &context->buffer[state->start_position];
    }
    return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------
void undo_stats(struct undo_context* context, float* buffer_usage_percentage, float* states_usage_percentage)
{
    *buffer_usage_percentage = (100.f * (float) context->current_position) / (float) context->buffer_size;
    *states_usage_percentage = (100.f * (float) context->num_states) / (float) context->max_states;
}

//-----------------------------------------------------------------------------------------------------------------------------
uint32_t undo_get_num_states(struct undo_context* context)
{
    return context->num_states;
}

//-----------------------------------------------------------------------------------------------------------------------------
void undo_terminate(struct undo_context* context)
{
    free(context->states);
    free(context);
}
