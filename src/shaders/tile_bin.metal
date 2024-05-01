#include <metal_stdlib>
#include "common.h"


struct bin_arguments
{
    constant draw_command* commands [[id(0)]];
    constant float* draw_data[[id(1)]];
    uint32_t num_commands;
    uint16_t num_tile_width;
    uint16_t num_tile_height;
    float tile_size;
    float aa_width;
};

struct tile_data
{
    device uint32_t* head;
    device uint32_t* next;
    uint32_t num_next;          // should be clear to zero each frame start
};

float2 skew(float2 v) {return float2(-v.y, v.x);}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_disc(float4 aabb, float2 center, float sq_radius)
{
    float2 nearest_point = metal::clamp(center.xy, aabb.xy, aabb.zw);
    return metal::distance_squared(nearest_point, center) < sq_radius;
}

// ---------------------------------------------------------------------------------------------------------------------------
bool intersection_aabb_obb(float4 aabb, float2 p0, float2 p1, float width)
{
    float2 dir = p1 - p0;
    float2 center = (p0 + p1) * .5f;
    float height = metal::length(dir);
    float2 axis_j = dir / height; 
    float2 axis_i = skew(axis_j);

    // extract obb vertices
    float2 v[4];
    float2 extent_i = axis_i * width;
    float2 extent_j = axis_j * height;
    v[0] = center + extent_i + extent_j;
    v[1] = center - extent_i + extent_j;
    v[2] = center + extent_i - extent_j;
    v[3] = center - extent_i - extent_j;

    // sat : obb vertices against aabb axis
    if (v[0].x > aabb.z && v[1].x > aabb.z && v[2].x > aabb.z && v[3].x > aabb.z)
        return false;

    if (v[0].x < aabb.x && v[1].x < aabb.x && v[2].x < aabb.x && v[3].x < aabb.x)
        return false;

    if (v[0].y < aabb.y && v[1].y < aabb.y && v[2].y < aabb.y && v[3].y < aabb.y)
        return false;

    if (v[0].y > aabb.w && v[1].y > aabb.w && v[2].y > aabb.w && v[3].y > aabb.w)
        return false;

        

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------
kernel void bin(constant bin_arguments& input [[buffer(0)]],
                device tile_data& output [[buffer(1)]],
                ushort2 index [[thread_position_in_grid]])
{
    uint16_t tile_index = index.y * input.num_tile_width + index.x;

    // compute tile bounding box
    float4 tile_aabb = float4(index.x, index.y, index.x + 1, index.y + 1) * input.tile_size;
    
    // grow the bounding box for anti-aliasing
    float4 tile_enlarge_aabb = tile_aabb + float4(-input.aa_width, -input.aa_width, input.aa_width, input.aa_width);

    // loop through draw commands in reverse order (because of the linked list)
    for(uint32_t i=input.num_commands; i-- > 0; )
    {
        const draw_command cmd = input.commands[i];
        uint32_t index = cmd.data_index;
        bool to_be_added = false;

        switch(cmd.type)
        {
            case sdf_box :
            {
                float2 p0 = float2(input.draw_data[index], input.draw_data[index+1]);
                float2 p1 = float2(input.draw_data[index+2], input.draw_data[index+3]);
                float width = input.draw_data[index+4];
                break;
            }
            case sdf_disc :
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
