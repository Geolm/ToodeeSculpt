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
void undo_store_state(struct undo_context* context, const void* data, size_t size)
{
    // currently asserting if we're out of states/memory
    // TODO : implement a ring buffer : new states can override oldest states
    assert(context->num_states < context->max_states && (context->current_position + size) < context->buffer_size);

    struct undo_state* state = &context->states[context->num_states++];
    state->start_position = context->current_position;
    state->size = size;

    memcpy(&context->buffer[state->start_position], data, size);
    context->current_position += size;
}

//-----------------------------------------------------------------------------------------------------------------------------
void* undo_undo(struct undo_context* context, size_t* output_size)
{
    return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------
void undo_stats(struct undo_context* context, float* buffer_usage_percentage, float* states_usage_percentage)
{

}

//-----------------------------------------------------------------------------------------------------------------------------
void undo_terminate(struct undo_context* context)
{
    free(context->states);
    free(context);
}
