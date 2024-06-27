#include <metal_stdlib>
#include "common.h"
#include "collision.h"

// ---------------------------------------------------------------------------------------------------------------------------
kernel void bin(constant draw_cmd_arguments& input [[buffer(0)]],
                device tiles_data& output [[buffer(1)]],
                device counters& counter [[buffer(2)]],
                ushort2 index [[thread_position_in_grid]])
{
    if (index.x >= input.num_tile_width || index.y >= input.num_tile_height)
        return;

    ushort tile_index = index.y * input.num_tile_width + index.x;

    // compute tile bounding box
    aabb tile_aabb = {.min = float2(index.x, index.y), .max = float2(index.x + 1, index.y + 1)};
    tile_aabb.min *= TILE_SIZE; tile_aabb.max *= TILE_SIZE;
    
    // grow the bounding box for anti-aliasing
    aabb tile_enlarge_aabb = tile_aabb;
    tile_enlarge_aabb.min -= input.aa_width; tile_enlarge_aabb.max += input.aa_width;
    
    // loop through draw commands in reverse order (because of the linked list)
    for(uint32_t i=input.num_commands; i-- > 0; )
    {
        constant quantized_aabb& cmd_aabb = input.commands_aabb[i];
        if (index.x > cmd_aabb.max_x || index.x < cmd_aabb.min_x ||
            index.y > cmd_aabb.max_y || index.y < cmd_aabb.min_y)
            continue;

        uint32_t data_index = input.commands[i].data_index;
        constant clip_rect& clip = input.clips[input.commands[i].clip_index];

        ushort2 tile_pos = index * TILE_SIZE;
        if (tile_pos.x > clip.max_x || (tile_pos.x + TILE_SIZE) < clip.min_x ||
            tile_pos.y > clip.max_y || (tile_pos.y + TILE_SIZE) < clip.min_y)
            continue;

        bool to_be_added;
        switch(input.commands[i].type)
        {
            case shape_line :
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
            case shape_box :
            {
                aabb box_aabb = {.min = float2(input.draw_data[data_index], input.draw_data[data_index+1]),
                                 .max = float2(input.draw_data[data_index+2], input.draw_data[data_index+3])};
                to_be_added = intersection_aabb_aabb(tile_aabb, box_aabb);
                break;
            }
            case shape_char :
            {
                aabb char_aabb;
                char_aabb.min = float2(input.draw_data[data_index], input.draw_data[data_index+1]);
                char_aabb.max = char_aabb.min + input.font_size * input.font_scale;
                to_be_added = intersection_aabb_aabb(tile_aabb, char_aabb);
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
}


// ---------------------------------------------------------------------------------------------------------------------------
kernel void write_icb(device counters& counter [[buffer(0)]],
                      device output_command_buffer& indirect_draw [[buffer(1)]])
{
    render_command cmd(indirect_draw.cmd_buffer, 0);
    cmd.draw_primitives(primitive_type::triangle_strip, 0, 4, atomic_load_explicit(&counter.num_tiles, memory_order_relaxed), 0);
}

