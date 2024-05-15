#include <metal_stdlib>
#include "common.h"
#include "collision.h"

// ---------------------------------------------------------------------------------------------------------------------------
kernel void bin(constant bin_arguments& input [[buffer(0)]],
                device tile_node& head [[buffer(1)]],
                device tile_node& nodes [[buffer(2)]],
                device counters& counter [[buffer(3)]],
                ushort2 index [[thread_position_in_grid]])
{
    uint16_t tile_index = index.y * input.num_tile_width + index.x;

    // compute tile bounding box
    aabb tile_aabb = {.min = float2(index.x, index.y), .max = float2(index.x + 1, index.y + 1)};
    tile_aabb.min *= input.tile_size; tile_aabb.max *= input.tile_size;
    
    // grow the bounding box for anti-aliasing
    aabb tile_enlarge_aabb = tile_aabb;
    tile_enlarge_aabb.min -= input.aa_width; tile_enlarge_aabb.max += input.aa_width;
    
    // loop through draw commands in reverse order (because of the linked list)
    for(uint32_t i=input.num_commands; i-- > 0; )
    {
        const draw_command cmd = input.commands[i];
        uint32_t index = cmd.data_index;
        bool to_be_added = false;

        switch(cmd.type)
        {
            case shape_rect_filled :
            {
                float2 p0 = float2(input.draw_data[index], input.draw_data[index+1]);
                float2 p1 = float2(input.draw_data[index+2], input.draw_data[index+3]);
                float width = input.draw_data[index+4];
                to_be_added = intersection_aabb_obb(tile_enlarge_aabb, p0, p1, width);
                break;
            }
            case shape_circle_filled :
            {
                float2 center = float2(input.draw_data[index], input.draw_data[index+1]);
                float radius = input.draw_data[index+2];
                float sq_radius = radius * radius;
                to_be_added = intersection_aabb_disc(tile_enlarge_aabb, center, sq_radius);
                break;
            }
        }
    }
    
    //
    //      extract command data depending of the type
    //      quick aabb_tile vs aabb command
    //      test precise aabb_tile vs command shape
    //      if intersection
    //          get a slot with interlock_increment output.num_next
    //          update output.head[tile_index] to grow the linked-list
}
