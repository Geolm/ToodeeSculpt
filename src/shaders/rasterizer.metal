#include <metal_stdlib>
#include "common.h"
#include "sdf.h"


struct vs_out
{
    float4 pos [[position]];
    uint16_t tile_index [[flat]];
};

// ---------------------------------------------------------------------------------------------------------------------------
vertex vs_out tile_vs(uint instance_id [[instance_id]],
                      uint vertex_id [[vertex_id]],
                      constant draw_cmd_arguments& input [[buffer0]]
                      constant uint16_t* tile_indices [[buffer1]])
{
    vs_out out;

    uint16_t tile_index = tile_indices[instance_id];
    uint16_t tile_x = instance_id % num_tile_width;
    uint16_t tile_y = instance_id / num_tile_width;
    
    float2 screen_pos = float2(vertex_id&1, vertex_id>>1);
    screen_pos *= input.tile_size;
    screen_pos += float2(tile_x, tile_y);

    float2 clipspace_pos = screen_pos * input.screen_div;
    clipspace_pos = (clipspace_pos * 2.f) - 1.f;
    clipspace_pos.y = -clipspace_pos.y;

    out.pos = float4(clipspace_pos.xy, 0.f, 1.0f);
    out.tile_index = tile_index;

    return out;
}

//-----------------------------------------------------------------------------
half4 unpack_color(uint32_t color)
{
    half4 result;
    result.r = float((color>>16)&0xFF) / 255.f;
    result.g = float((color>>8)&0xFF) / 255.f;
    result.b = float(color&0xFF) / 255.f;
    result.a = float((color>>24)&0xFF) / 255.f;
    return result;
}

//-----------------------------------------------------------------------------
// based on https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-23-high-speed-screen-particles
// a specific blend equation is required
half4 accumulate_color(half4 color, half4 backbuffer)
{
    half4 output;
    output.rgb = (color.rgb * color.a) + (backbuffer.rgb * (1.f - color.a));
    output.a = backbuffer.a * (1.f - color.a);
    return output;
}

// ---------------------------------------------------------------------------------------------------------------------------
fragment half4 tile_fs(vs_out in [[stage_in]],
                       constant draw_cmd_arguments& input [[buffer0]],
                       device tiles_data& tiles [[buffer1]])
{
    half4 output = half4(0.f, 0.f, 0.f, 1.f);

    float2 position = 

    tile_node node = tiles.head[index];
    while (node.next != INVALID_INDEX)
    {
        draw_command& cmd = input.commands[node.command_index];
        uint32_t data_index = cmd.data_index;

        float distance;

        switch(cmd.type)
        {
        case shape_circle_filled :
            {
                float2 center = float2(input.draw_data[data_index], input.draw_data[data_index+1]);
                float radius = input.draw_data[data_index+2];
                float sq_radius = radius * radius;
                distance = sd_disc(in.pos, center, radius);
                break;
            }
        }

        half4 color = unpack_color(cmd.color);
        half alpha_factor = 1.f - smoothstep(0.f, input.aa_width, distance);    // anti-aliasing
        color.a *= alpha_factor;
        output = accumulate_color(color, output);

        node = tiles.nodes[node.next];
    }

    if (all(output == half4(0.f, 0.f, 0.f, 1.f)))
        discard_fragment();

    return output;
}

