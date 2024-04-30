#include <metal_stdlib>

struct draw_command
{
    uint8_t type;
    uint8_t modifier;
    uint8_t clip_index;
    uint8_t op;
    uint32_t color;
    uint32_t data_index;
};

struct bin_arguments
{
    constant draw_command* commands [[id(0)]];
    constant float* draw_data[[id(1)]];
    uint32_t num_commands;
    uint16_t num_tile_width;
    uint16_t num_tile_height;
    float tile_size;
    float aa_width;
};

struct tile_data
{
    device uint32_t* head;
    device uint32_t* next;
    uint32_t num_next;          // should be clear to zero each frame start
};

kernel void bin(constant bin_arguments& input [[buffer(0)]],
                device tile_data& output [[buffer(1)]],
                ushort2 index [[thread_position_in_grid]])
{
    // compute tile_index = index.y * input.num_tile_width + index.x;
    // compute tile bounding box

    // grow the bounding box for anti-aliasing

    // loop through draw commands in reverse order (because of the linked list)
    //
    //      extract command data depending of the type
    //      quick aabb_tile vs aabb command
    //      test precise aabb_tile vs command shape
    //      if intersection
    //          get a slot with interlock_increment output.num_next
    //          update output.head[tile_index] to grow the linked-list
}
