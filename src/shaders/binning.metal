#include <metal_stdlib>
#include "common.h"
#include "collision.h"
#include "sdf.h"

// ---------------------------------------------------------------------------------------------------------------------------
// for each draw command, test aabb vs aabb of the region and put 1 if visible (otherwise 0)
// ---------------------------------------------------------------------------------------------------------------------------
kernel void predicate(constant draw_cmd_arguments& input [[buffer(0)]],
                      device uint8_t* predicate [[buffer(1)]],
                      uint index [[thread_position_in_grid]])
{
    if (index >= input.num_commands)
        return;

    // reverse order for the tile linked list 
    uint cmd_index = input.num_commands - index - 1;

    quantized_aabb aabb = input.commands_aabb[cmd_index];
    aabb.min_x /= REGION_SIZE; aabb.min_y /= REGION_SIZE;
    aabb.max_x /= REGION_SIZE; aabb.max_y /= REGION_SIZE;

    for(uint y=0; y<input.num_region_height; ++y)
    {
        for(uint x=0; x<input.num_region_width; ++x)
        {
            bool visible = (x >= aabb.min_x && x <= aabb.max_x && y >= aabb.min_y && y <= aabb.max_y);
            uint region_index = y * input.num_region_width + x;
            uint region_offset = region_index * input.num_commands;
            predicate[region_offset + index] = visible ? 1 : 0;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
// one-pass exclusive scan, using simd_prefix, threadgroup memory
// ---------------------------------------------------------------------------------------------------------------------------
kernel void exclusive_scan(constant draw_cmd_arguments& input [[buffer(0)]],
                           device const uint8_t* predicate [[buffer(1)]],
                           device uint16_t* scan [[buffer(2)]],
                           threadgroup uint16_t* simd_totals [[threadgroup(0)]],
                           threadgroup uint16_t* simd_offsets [[threadgroup(1)]],
                           uint tid_in_tg [[thread_index_in_threadgroup]],
                           uint2 tg_size [[threads_per_threadgroup]],
                           uint simd_group_id [[simdgroup_index_in_threadgroup]],
                           uint2 index [[thread_position_in_grid]])
{
    const uint threads_per_line = tg_size.x;
    const uint region_index = index.y;
    const uint region_offset = region_index * input.num_commands;
    const uint thread_index = tid_in_tg;

    // Compute where this thread starts reading/writing
    const uint thread_base_idx = thread_index * input.num_elements_per_thread;

    // Local prefix sum
    uint16_t local_sum = 0;

    for (uint i = 0; i < input.num_elements_per_thread; ++i) 
    {
        uint idx = thread_base_idx + i;
        if (idx < input.num_commands)
        {
            scan[region_offset + idx] = local_sum;
            local_sum += predicate[region_offset + idx];
        }
    }

    // Compute SIMD-group total and store
    uint16_t group_sum = simd_sum(local_sum);
    if (simd_is_first()) 
        simd_totals[simd_group_id] = group_sum;

    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Prefix sum across SIMD group totals
    const uint num_simd_groups = (threads_per_line + SIMD_GROUP_SIZE - 1) / SIMD_GROUP_SIZE;
    if (thread_index < num_simd_groups) 
    {
        uint16_t v = simd_totals[thread_index];
        simd_offsets[thread_index] = simd_prefix_exclusive_sum(v);
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);
    uint16_t simd_offset = simd_offsets[simd_group_id];

    // Final output write
    uint16_t thread_offset = simd_offset + simd_prefix_exclusive_sum(local_sum);
    for (uint i = 0; i < input.num_elements_per_thread; ++i) 
    {
        uint idx = thread_base_idx + i;
        if (idx < input.num_commands) 
            scan[region_offset + idx] += thread_offset;
    }
}


// ---------------------------------------------------------------------------------------------------------------------------
// bin commands for region
// ---------------------------------------------------------------------------------------------------------------------------
kernel void region_bin(constant draw_cmd_arguments& input [[buffer(0)]],
                       device uint16_t* regions_indices [[buffer(1)]],
                       device const uint16_t* scan [[buffer(2)]],
                       device const uint8_t* predicate [[buffer(3)]],
                       uint2 index [[thread_position_in_grid]])
{
    uint cmd_index = index.x;
    uint region_index = index.y;

    if (cmd_index >= input.num_commands)
        return;

    uint region_offset = region_index * input.num_commands;

    if (predicate[region_offset + cmd_index] == 1)
    {
        uint16_t position = scan[region_offset + cmd_index];
        regions_indices[region_offset + position] = input.num_commands - cmd_index - 1;
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
// for each tile of the screen, we traverse the list of commands and if the command has an impact on the tile we add the
// command to the linked list of the tile
// ---------------------------------------------------------------------------------------------------------------------------
kernel void tile_bin(constant draw_cmd_arguments& input [[buffer(0)]],
                device tiles_data& output [[buffer(1)]],
                device counters& counter [[buffer(2)]],
                constant const uint16_t* regions_indices [[buffer(3)]],
                ushort2 index [[thread_position_in_grid]])
{
    if (index.x >= input.num_tile_width || index.y >= input.num_tile_height)
        return;

    uint16_t tile_index = index.y * input.num_tile_width + index.x;

    // compute tile bounding box
    aabb tile_aabb = {.min = float2(index.x, index.y), .max = float2(index.x + 1, index.y + 1)};
    tile_aabb.min *= TILE_SIZE; tile_aabb.max *= TILE_SIZE;

    float smooth_border = 0.f;
    bool draw_something = false;

    ushort2 region_pos = index / REGION_SIZE;
    uint32_t region_index = region_pos.y * input.num_region_width + region_pos.x;
    constant const uint16_t* indices = &regions_indices[region_index * input.num_commands];

    for(uint32_t i=0; i<input.num_commands; ++i)
    {
        uint32_t cmd_index = indices[i];
        if (cmd_index == LAST_COMMAND)
            break;

        constant quantized_aabb& cmd_aabb = input.commands_aabb[cmd_index];
        if (any(ushort4(index.xy, cmd_aabb.max_x, cmd_aabb.max_y) < ushort4(cmd_aabb.min_x, cmd_aabb.min_y, index.xy)))
            continue;

        uint32_t data_index = input.commands[cmd_index].data_index;
        constant clip_rect& clip = input.clips[input.commands[cmd_index].clip_index];

        ushort2 tile_pos = index * TILE_SIZE;
        if (any(tile_pos>ushort2(clip.max_x, clip.max_y)) || any((tile_pos + TILE_SIZE)<ushort2(clip.min_x, clip.min_y)))
            continue;

        // grow the bounding box for anti-aliasing and smooth blend
        aabb tile_enlarge_aabb = aabb_grow(tile_aabb, (input.commands[cmd_index].op == op_union) ? max(input.aa_width, smooth_border) : input.aa_width);

        const bool is_hollow = (primitive_get_fillmode(input.commands[cmd_index].type) == fill_hollow);
        bool to_be_added = false;
        constant float* data = &input.draw_data[data_index];
        command_type type = primitive_get_type(input.commands[cmd_index].type);
        switch(type)
        {
            case primitive_oriented_box :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float width = data[4];
                aabb tile_rounded = aabb_grow(tile_enlarge_aabb, data[5]);
                to_be_added = intersection_aabb_obb(tile_rounded, p0, p1, width);

                if (to_be_added && is_hollow && is_aabb_inside_obb(p0, p1, width, tile_rounded))
                    to_be_added = false;
                break;
            }
            case primitive_ellipse :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float width = data[4];
                float2 tile_center = (tile_aabb.min + tile_aabb.max) * .5f;

                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, (is_hollow ? data[5] : 0.f));
                to_be_added = intersection_ellipse_circle(p0, p1, width, tile_center, length(aabb_get_extents(tile_smooth) * .5f));

                if (to_be_added && is_hollow && is_aabb_inside_ellipse(p0, p1, width, tile_smooth))
                    to_be_added = false;
                break;
            }
            case primitive_arc :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];
                float2 direction = float2(data[3], data[4]);
                float2 aperture = float2(data[5], data[6]);
                float thickness = data[7];
                to_be_added = intersection_aabb_arc(tile_enlarge_aabb, center, direction, aperture, radius, thickness);
                break;
            }
            case primitive_pie :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];
                float2 direction = float2(data[3], data[4]);
                float2 aperture = float2(data[5], data[6]);

                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, (is_hollow ? data[7] : 0.f));
                to_be_added = intersection_aabb_pie(tile_smooth, center, direction, aperture, radius);

                if (to_be_added && is_hollow && is_aabb_inside_pie(center, direction, aperture, radius, tile_smooth))
                    to_be_added = false;

                break;
            }

            case primitive_disc :
            {
                float2 center = float2(data[0], data[1]);
                float radius = data[2];

                if (is_hollow)
                {
                    float half_width = data[3] + max(input.aa_width, smooth_border);
                    to_be_added = intersection_aabb_circle(tile_aabb, center, radius, half_width);
                }
                else
                {
                    radius += max(input.aa_width, smooth_border);
                    to_be_added = intersection_aabb_disc(tile_aabb, center, radius);
                }
                break;
            }
            case primitive_triangle :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float2 p2 = float2(data[4], data[5]);
                aabb tile_rounded = aabb_grow(tile_enlarge_aabb, data[6]);
                to_be_added = intersection_aabb_triangle(tile_rounded, p0, p1, p2);

                if (to_be_added && is_hollow && is_aabb_inside_triangle(p0, p1, p2, tile_rounded))
                    to_be_added = false;

                break;
            }

            case primitive_uneven_capsule :
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float radius0 = data[4];
                float radius1 = data[5];
                float2 tile_center = (tile_aabb.min + tile_aabb.max) * .5f;

                aabb tile_smooth = aabb_grow(tile_enlarge_aabb, (is_hollow ? data[6] : 0.f));

                // use sdf because the shape is not a "standard" uneven capsule
                // it preserves the tangent but it's hard to compute the bounding convex object to test against AAABB
                // so we use the bounding sphere of the tile to test instead
                to_be_added = sd_uneven_capsule(tile_center, p0, p1, radius0, radius1) < length(aabb_get_extents(tile_smooth) * .5f);

                break;
            }

            case primitive_trapezoid:
            {
                float2 p0 = float2(data[0], data[1]);
                float2 p1 = float2(data[2], data[3]);
                float radius0 = data[4];
                float radius1 = data[5];

                aabb tile_rounded = aabb_grow(tile_enlarge_aabb, data[6]);
                to_be_added = intersection_aabb_obb(tile_rounded, p0, p1, radius0, radius1);
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
                output.head[tile_index].command_index = cmd_index;
                output.head[tile_index].next = new_node_index;
            }

            if (type != combination_begin && type != combination_end)
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

