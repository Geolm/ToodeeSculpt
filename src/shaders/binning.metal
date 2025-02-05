#include <metal_stdlib>
#include "common.h"
#include "collision.h"
#include "sdf.h"

// ---------------------------------------------------------------------------------------------------------------------------
// for each tile of the screen, we traverse the list of commands and if the command has an impact on the tile we add the
// command to the linked list of the tile
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
    float2 tile_center = (tile_aabb.min + tile_aabb.max) * .5f;
    
    // grow the bounding box for anti-aliasing
    aabb tile_enlarge_aabb = aabb_grow(tile_aabb, input.aa_width);

    float smooth_border = 0.f;
    bool draw_something = false;
    
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

        const bool filled = primitive_is_filled(input.commands[i].type);
        const primitive_fillmode fillmode =  primitive_get_fillmode(input.commands[i].type);
        bool to_be_added = false;
        constant float* data = &input.draw_data[data_index];
        switch(primitive_get_type(input.commands[i].type))
        {
            case primitive_oriented_box :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float width = data[4];
                aabb tile_rounded = aabb_grow(tile_enlarge_aabb, data[5] + smooth_border);
                to_be_added = intersection_aabb_obb(tile_rounded, p0, p1, width);

                if (to_be_added && fillmode == fill_hollow && is_aabb_inside_obb(p0, p1, width, tile_rounded))
                    to_be_added = false;
                break;
            }
            case primitive_ellipse :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float width = data[4];
                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, smooth_border + (filled ? 0.f : data[5]));
                to_be_added = intersection_ellipse_circle(p0, p1, width, tile_center, length(aabb_get_extents(tile_smooth) * .5f));

                if (to_be_added && !filled && is_aabb_inside_ellipse(p0, p1, width, tile_smooth))
                    to_be_added = false;
                break;
            }
            case primitive_ring :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];
                float2 direction = float2(data[3], data[4]);
                float2 aperture = float2(data[5], data[6]);
                float thickness = data[7];

                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, smooth_border);
                to_be_added = intersection_aabb_arc(tile_smooth, center, direction, aperture, radius, thickness);
                break;
            }
            case primitive_pie :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];
                float2 direction = float2(data[3], data[4]);
                float2 aperture = float2(data[5], data[6]);

                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, smooth_border + (filled ? 0.f : data[7]));
                to_be_added = intersection_aabb_pie(tile_smooth, center, direction, aperture, radius);

                if (to_be_added && !filled && is_aabb_inside_pie(center, direction, aperture, radius, tile_smooth))
                    to_be_added = false;

                break;
            }

            case primitive_disc :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];

                if (fillmode == fill_hollow)
                {
                    float half_width = data[3] + input.aa_width + smooth_border;
                    to_be_added = intersection_aabb_circle(tile_aabb, center, radius, half_width);
                }
                else
                {
                    radius += input.aa_width + smooth_border;
                    to_be_added = intersection_aabb_disc(tile_aabb, center, radius);
                }
                break;
            }
            case primitive_triangle :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float2 p2 = float2(data[4], data[5]);
                aabb tile_rounded = aabb_grow(tile_enlarge_aabb, data[6] + smooth_border);
                to_be_added = intersection_aabb_triangle(tile_rounded, p0, p1, p2);

                if (to_be_added && !filled && is_aabb_inside_triangle(p0, p1, p2, tile_rounded))
                    to_be_added = false;

                break;
            }

            case primitive_uneven_capsule :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float radius0 = data[4];
                float radius1 = data[5];

                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, smooth_border + (filled ? 0.f : data[6]));

                // use sdf because the shape is not a "standard" uneven capsule
                // it preserves the tangent but it's hard to compute the bounding convex object to test against AAABB
                // so we use the bounding sphere of the tile to test instead
                to_be_added = sd_uneven_capsule(tile_center, p0, p1, radius0, radius1) < length(aabb_get_extents(tile_smooth) * .5f);

                break;
            }

            case combination_begin:
            {
                smooth_border = 0.f;
                to_be_added = true;
                break;
            }
            case combination_end:
            {
                smooth_border = data[0];    // we traverse in reverse order, so the end comes first
                to_be_added = true;
                break;
            }
            case primitive_aabox :
            case primitive_char : to_be_added = true; break;
            default : to_be_added = false; break;
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

            if (input.commands[i].type != combination_begin && input.commands[i].type != combination_end)
                draw_something = true;
        }
    }

    // if the tile has some draw command to proceed
    if (draw_something)
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

