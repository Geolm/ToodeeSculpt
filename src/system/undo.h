#ifndef __UNDO_H__
#define __UNDO_H__

#include <stddef.h>
#include <stdint.h>

//-----------------------------------------------------------------------------------------------------------------------------
// Simple undo system
//   * store states (no compression, no delta)
//   * undo function will return the latest state
//-----------------------------------------------------------------------------------------------------------------------------

struct undo_context;

#ifdef __cplusplus
extern "C" {
#endif

// allocate structures and buffer
struct undo_context* undo_init(size_t buffer_size, uint32_t max_states_count);

// push a new state in the buffer, data is copied
void undo_store_state(struct undo_context* context, const void* data, size_t size); 

// returns the data for the last state, if null the undo failed
// warning : you have to copy the data back as the pointer won't be valid after a new state
void* undo_undo(struct undo_context* context, size_t* output_size);

void undo_stats(struct undo_context* context, float* buffer_usage_percentage, float* states_usage_percentage);

// free memory
void undo_terminate(struct undo_context* context);

#ifdef __cplusplus
}
#endif

#endif