#include <metal_stdlib>
#include "common.h"
#include "collision.h"

// ---------------------------------------------------------------------------------------------------------------------------
kernel void bin(constant draw_cmd_arguments& input [[buffer(0)]],
                device tiles_data& output [[buffer(1)]],
                device counters& counter [[buffer(2)]],
                device output_command_buffer& indirect_draw [[buffer(3)]],
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
        uint32_t data_index = input.commands[i].data_index;
        bool to_be_added;

        // TODO : add clip test

        switch(input.commands[i].type)
        {
            case shape_rect_filled :
            {
                float2 p0 = float2(input.draw_data[data_index], input.draw_data[data_index+1]);
                float2 p1 = float2(input.draw_data[data_index+2], input.draw_data[data_index+3]);
                float width = input.draw_data[data_index+4];
                to_be_added = intersection_aabb_obb(tile_enlarge_aabb, p0, p1, width);
                break;
            }
            case shape_circle_filled :
            {
                float2 center = float2(input.draw_data[data_index], input.draw_data[data_index+1]);
                float radius = input.draw_data[data_index+2] + input.aa_width;
                float sq_radius = radius * radius;
                to_be_added = intersection_aabb_disc(tile_aabb, center, sq_radius);
                break;
            }
            default : to_be_added = false;
        }

        if (to_be_added)
        {
            // allocate one node
            uint new_node_index = atomic_fetch_add_explicit(&counter.num_nodes, 1, memory_order_relaxed);

            // avoid access beyond the end of the buffer
            if (new_node_index<input.max_nodes)
            {
                // insert in the linked list the new node
                output.nodes[new_node_index] = output.head[tile_index];
                output.head[tile_index].command_index = i;
                output.head[tile_index].next = new_node_index;
            }
        }
    }

    // if the tile has some draw command to proceed
    if (output.head[tile_index].command_index != INVALID_INDEX)
    {
        uint pos = atomic_fetch_add_explicit(&counter.num_tiles, 1, memory_order_relaxed);

        // add tile index
        output.tile_indices[pos] = tile_index;
    }

    // wait until all thread have incremented the number of tiles to be drawn
    threadgroup_barrier(mem_flags::mem_device);

    // only the first thread write the indirect draw call buffer
    if (all(index.xy == ushort2(0,0)))
    {
        render_command cmd(indirect_draw.cmd_buffer, 0);

        uint num_tiles = atomic_load_explicit(&counter.num_tiles, memory_order_relaxed);

        cmd.set_vertex_buffer(&input, 0);
        cmd.set_vertex_buffer(output.tile_indices, 1);
        cmd.set_fragment_buffer(&input, 0);
        cmd.set_fragment_buffer(&output, 1);
        cmd.draw_primitives(primitive_type::triangle_strip, 0, 4, num_tiles, 0);

    }
}
