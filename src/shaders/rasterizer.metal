#include <metal_stdlib>
#include "common.h"


struct vs_out
{
    float4 pos [[position]];
};

vertex vs_out tile_vs(uint instance_id [[instance_id]],
                      uint vertex_id [[vertex_id]],
                      constant draw_cmd_arguments& input [[buffer0]]
                      constant uint16_t* tile_indices [[buffer1]])
{
    vs_out out;

    uint tile_index = tile_indices[instance_id];
    uint tile_x = instance_id % num_tile_width;
    uint tile_y = instance_id / num_tile_width;
    
    float2 screen_pos = float2(vertex_id&1, vertex_id>>1);
    screen_pos *= input.tile_size;
    screen_pos += float2(tile_x, tile_y);

    float2 clipspace_pos = screen_pos * input.screen_div;
    clipspace_pos = (clipspace_pos * 2.f) - 1.f;
    clipspace_pos.y = -clipspace_pos.y;
    out.pos = float4(clipspace_pos.xy, 0.f, 1.0f);

    return out;
}