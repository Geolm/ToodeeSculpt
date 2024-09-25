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
        if (any(ushort4(index.xy, cmd_aabb.max_x, cmd_aabb.max_y) < ushort4(cmd_aabb.min_x, cmd_aabb.min_y, index.xy)))
            continue;

        uint32_t data_index = input.commands[i].data_index;
        constant clip_rect& clip = input.clips[input.commands[i].clip_index];

        ushort2 tile_pos = index * TILE_SIZE;
        if (any(tile_pos>ushort2(clip.max_x, clip.max_y)) || any((tile_pos + TILE_SIZE)<ushort2(clip.min_x, clip.min_y)))
            continue;

        bool to_be_added = false;
        constant float* data = &input.draw_data[data_index];
        switch(input.commands[i].type)
        {
            case shape_oriented_box :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float width = data[4];
                aabb tile_rounded = tile_enlarge_aabb;
                tile_rounded.min -= data[5];
                tile_rounded.max += data[5];
                to_be_added = intersection_aabb_obb(tile_rounded, p0, p1, width);
                break;
            }
            case shape_circle :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];
                float half_width = data[3] + input.aa_width;
                to_be_added = intersection_aabb_circle(tile_aabb, center, radius, half_width);
                break;
            }
            case shape_circle_filled :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2] + input.aa_width;
                float sq_radius = radius * radius;
                to_be_added = intersection_aabb_disc(tile_aabb, center, sq_radius);
                break;
            }
            case shape_aabox :
            case shape_char : to_be_added = true; break;
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

