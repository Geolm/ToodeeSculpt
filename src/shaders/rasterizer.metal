#include <metal_stdlib>
#include "common.h"
#include "sdf.h"
#include "operators.h"


struct vs_out
{
    float4 pos [[position]];
    uint16_t tile_index [[flat]];
};

// ---------------------------------------------------------------------------------------------------------------------------
vertex vs_out tile_vs(uint instance_id [[instance_id]],
                      uint vertex_id [[vertex_id]],
                      constant draw_cmd_arguments& input [[buffer(0)]],
                      constant uint16_t* tile_indices [[buffer(1)]])
{
    vs_out out;

    uint16_t tile_index = tile_indices[instance_id];
    uint16_t tile_x = tile_index % input.num_tile_width;
    uint16_t tile_y = tile_index / input.num_tile_width;
    
    float2 screen_pos = float2(vertex_id&1, vertex_id>>1);
    screen_pos += float2(tile_x, tile_y);
    screen_pos *= TILE_SIZE;

    float2 clipspace_pos = screen_pos * input.screen_div;
    clipspace_pos = (clipspace_pos * 2.f) - 1.f;
    clipspace_pos.y = -clipspace_pos.y;

    out.pos = float4(clipspace_pos.xy, 0.f, 1.0f);
    out.tile_index = tile_index;

    return out;
}

//-----------------------------------------------------------------------------
// based on https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-23-high-speed-screen-particles
// a specific blend equation is required
half4 accumulate_color(half4 color, half4 backbuffer)
{
    half4 output;
    half one_minus_alpha = 1.h - color.a;
    output.rgb = (color.rgb * color.a) + (backbuffer.rgb * one_minus_alpha);
    output.a = backbuffer.a * one_minus_alpha;
    return output;
}

// ---------------------------------------------------------------------------------------------------------------------------
fragment half4 tile_fs(vs_out in [[stage_in]],
                       constant draw_cmd_arguments& input [[buffer(0)]],
                       device tiles_data& tiles [[buffer(1)]])
{
    half4 output = half4(0.h, 0.h, 0.h, 1.h);

    tile_node node = tiles.head[in.tile_index];
    while (node.next != INVALID_INDEX)
    {
        constant draw_command& cmd = input.commands[node.command_index];
        constant clip_rect& clip = input.clips[cmd.clip_index];
        uint32_t data_index = cmd.data_index;

        // check if the pixel is in the clip rect
        if (all(float4(in.pos.xy, clip.max_x, clip.max_y) >= float4(clip.min_x, clip.min_y, in.pos.xy)))
        {
            float distance = 10.f;
            constant float* data = &input.draw_data[data_index];

            switch(cmd.type)
            {
            case shape_circle :
                {
                    float2 center = float2(data[0], data[1]);
                    float radius = data[2];
                    float half_width = data[3];
                    distance = abs(sd_disc(in.pos.xy, center, radius)) - half_width;
                    break;
                }
            case shape_circle_filled :
                {
                    float2 center = float2(data[0], data[1]);
                    float radius = data[2];
                    distance = sd_disc(in.pos.xy, center, radius);
                    break;
                }
            case shape_line:
                {
                    float2 p0 = float2(data[0], data[1]);
                    float2 p1 = float2(data[2], data[3]);
                    float width = data[4];
                    distance = sd_oriented_box(in.pos.xy, p0, p1, width);
                    break;
                }
            case shape_box:
                {
                    float2 min = float2(data[0], data[1]);
                    float2 max = float2(data[2], data[3]);
                    if (all(in.pos.xy >= min && in.pos.xy <= max))
                        distance = 0.f;
                    break;
                }
            case shape_char:
                {
                    float2 top_left = float2(data[0], data[1]);
                    float2 pos = (in.pos.xy - top_left) / input.font_scale;
                    if (all(pos >= 0.f && pos <= input.font_size))
                    {
                        ushort2 pixel_pos = (ushort2) pos;
                        ushort bitfield = input.font[cmd.custom_data * (ushort) input.font_size.x + pixel_pos.x];
                        if (bitfield&(1<<pixel_pos.y))
                            distance = 0.f;
                    }
                }
            }

            half4 color = unpack_unorm4x8_to_half(cmd.color.packed_data);
            half alpha_factor = 1.h - smoothstep(0.h, half(input.aa_width), half(distance));    // anti-aliasing
            color.a *= alpha_factor;
            output = accumulate_color(color, output);
        }

        node = tiles.nodes[node.next];
    }

    if (all(output == half4(0.h, 0.h, 0.h, 1.h)))
        discard_fragment();

    return output;
}

