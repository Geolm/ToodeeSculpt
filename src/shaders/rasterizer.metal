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

#define LARGE_DISTANCE (100000000.f)
#define BLACK_COLOR (half4(0.h, 0.h, 0.h, 1.h))

// ---------------------------------------------------------------------------------------------------------------------------
fragment half4 tile_fs(vs_out in [[stage_in]],
                       constant draw_cmd_arguments& input [[buffer(0)]],
                       device tiles_data& tiles [[buffer(1)]])
{
    half4 output = BLACK_COLOR;
    float previous_distance;
    half4 previous_color;
    float combination_smoothness;
    bool combining = false;

    half4 outline_color = unpack_unorm4x8_to_half(input.outline_color.packed_data);
    float outline_full = -input.outline_width;
    float outline_start = outline_full - input.aa_width;

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
            command_type type = primitive_get_type(cmd.type);

            if (type == combination_begin)
            {
                previous_color = 0.h;
                previous_distance = LARGE_DISTANCE;
                combination_smoothness = data[0];
                combining = true;
            }
            else
            {
                const primitive_fillmode fillmode =  primitive_get_fillmode(cmd.type);

                switch(type)
                {
                case primitive_disc :
                {
                    float2 center = float2(data[0], data[1]);
                    float radius = data[2];
                    distance = sd_disc(in.pos.xy, center, radius);
                    if (fillmode == fill_hollow)
                        distance = abs(distance) - data[3];
                    break;
                }
                case primitive_oriented_box :
                {
                    float2 p0 = float2(data[0], data[1]);
                    float2 p1 = float2(data[2], data[3]);
                    distance = sd_oriented_box(in.pos.xy, p0, p1, data[4]);
                    if (fillmode == fill_hollow)
                        distance = abs(distance);
                    distance -= data[5];
                    break;
                }
                case primitive_ellipse :
                {
                    float2 p0 = float2(data[0], data[1]);
                    float2 p1 = float2(data[2], data[3]);
                    distance = sd_oriented_ellipse(in.pos.xy, p0, p1, data[4]);
                    if (fillmode == fill_hollow)
                        distance = abs(distance) - data[5];
                    break;
                }
                case primitive_aabox:
                {
                    float2 min = float2(data[0], data[1]);
                    float2 max = float2(data[2], data[3]);
                    if (all(in.pos.xy >= min && in.pos.xy <= max))
                        distance = 0.f;
                    break;
                }
                case primitive_char:
                {
                    float2 top_left = float2(data[0], data[1]);
                    float2 uv = (in.pos.xy - top_left) / input.font_size;

                    if (all(uv >= 0.f && uv <= 1.f))
                    {
                        uv = float2(.1f, .1f) + float2(0.8f, 0.85f) * uv;
                        float2 char_uv = float2(float(FONT_CHAR_WIDTH) / float(FONT_TEXTURE_WIDTH),
                                                float(FONT_CHAR_HEIGHT) / float(FONT_TEXTURE_HEIGHT));
                        uv *= char_uv;
                        uv += float2(float(cmd.custom_data%12), float(cmd.custom_data/12)) * char_uv;

                        constexpr sampler s(address::clamp_to_zero, filter::linear );
                        half texel = 1.h - input.font.sample(s, uv).r;
                        distance = texel * input.aa_width;
                    }
                    break;
                }
                case primitive_triangle:
                {
                    float2 p0 = float2(data[0], data[1]);
                    float2 p1 = float2(data[2], data[3]);
                    float2 p2 = float2(data[4], data[5]);
                    distance = sd_triangle(in.pos.xy, p0, p1, p2);
                    
                    if (fillmode == fill_hollow)
                        distance = abs(distance);
                        
                    distance -= data[6];
                    break;
                }
                case primitive_pie:
                {
                    float2 center = float2(data[0], data[1]);
                    float radius = data[2];
                    float2 direction = float2(data[3], data[4]);
                    float2 aperture = float2(data[5], data[6]);

                    distance = sd_oriented_pie(in.pos.xy, center, direction, aperture, radius);
                    if (fillmode == fill_hollow)
                        distance = abs(distance) - data[7];
                    break;
                }
                case primitive_ring:
                {
                    float2 center = float2(data[0], data[1]);
                    float radius = data[2];
                    float2 direction = float2(data[3], data[4]);
                    float2 aperture = float2(data[5], data[6]);
                    float thickness = data[7];

                    distance = sd_oriented_ring(in.pos.xy, center, direction, aperture, radius, thickness);
                    if (fillmode == fill_hollow)
                        distance = abs(distance) - data[7];
                    break;
                }
                case primitive_uneven_capsule:
                {
                    float2 p0 = float2(data[0], data[1]);
                    float2 p1 = float2(data[2], data[3]);
                    float radius0 = data[4];
                    float radius1 = data[5];

                    distance = sd_uneven_capsule(in.pos.xy, p0, p1, radius0, radius1);
                    if (fillmode == fill_hollow)
                        distance = abs(distance) - data[6];
                    break;
                }
                case primitive_trapezoid:
                {
                    float2 p0 = float2(data[0], data[1]);
                    float2 p1 = float2(data[2], data[3]);
                    float radius0 = data[4];
                    float radius1 = data[5];

                    distance = sd_trapezoid(in.pos.xy, p0, p1, radius0, radius1);
                    if (fillmode == fill_hollow)
                        distance = abs(distance) - data[6];
                    else
                        distance -= data[6];

                    break;
                }
                default: break;
                }

                half4 color;
                if (type == combination_end)
                {
                    combining = false;
                    color = previous_color;
                    distance = previous_distance;
                }
                else
                {
                    color = unpack_unorm4x8_srgb_to_half(cmd.color.packed_data);

                    if (fillmode == fill_outline && distance >= outline_start)
                    {
                        if (distance >= outline_full && distance <= input.aa_width)
                            color.rgb = outline_color.rgb;
                        else if (distance < outline_full)
                            color.rgb = mix(outline_color.rgb, color.rgb, linearstep(half(outline_full), half(outline_start), half(distance)));
                    }
                }

                // blend distance / color and skip writing output
                if (combining)
                {
                    float smooth_factor = (cmd.op == op_union) ? combination_smoothness : input.aa_width;
                    switch(cmd.op)
                    {
                    case op_add :
                    case op_union :
                    {
                        float2 smooth = smooth_minimum(distance, previous_distance, smooth_factor);
                        previous_distance = smooth.x;
                        previous_color = mix(color, previous_color, smooth.y);
                        break;
                    }
                    case op_subtraction :
                    {
                        previous_distance = smooth_substraction(previous_distance, distance, smooth_factor);
                        break;
                    }
                    case op_intersection :
                    {
                        previous_distance = smooth_intersection(previous_distance, distance, smooth_factor);
                        break;
                    }
                    }
                }
                else
                {
                    half alpha_factor;
                    if (fillmode == fill_outline && type == combination_end)
                    {
                        if (distance > input.aa_width)
                            color.rgb = outline_color.rgb;
                        else
                            color.rgb = mix(outline_color.rgb, color.rgb, linearstep(input.aa_width, 0.f, distance));
                        alpha_factor = linearstep(half(input.aa_width*2+input.outline_width), half(input.aa_width+input.outline_width), half(distance));    // anti-aliasing
                    }
                    else
                        alpha_factor = linearstep(half(input.aa_width), 0.h, half(distance));    // anti-aliasing

                    color.a *= alpha_factor;
                    output = accumulate_color(color, output);
                }
            }
        }

        node = tiles.nodes[node.next];
    }

    if (all(output == BLACK_COLOR))
    {
        if (input.culling_debug)
            return half4(0.f, 0.f, 1.0f, 1.0f);
        else
            discard_fragment();
    }

    return output;
}

