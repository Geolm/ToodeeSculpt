#include <metal_stdlib>
#include "common.h"


// ---------------------------------------------------------------------------------------------------------------------------
float2 transform_point(constant view_project& vp, constant sdform& sdform, float2 half_window_size, float2 point)
{
    // sdform translation + rotation
    float2x2 rotation = float2x2(sdform.translation_rotation.z, -sdform.translation_rotation.w, sdform.translation_rotation.w, sdform.translation_rotation.z);
    point = rotation * point;
    point += sdform.translation_rotation.xy;

    // viewproject
    point -= vp.camera.zw;
    point *= vp.camera.xy;
    point *= vp.projection.xy;
    point += vp.projection.zw;
    point = (point + 1.f) * half_window_size;

    return point;
}

// ---------------------------------------------------------------------------------------------------------------------------
float transform_distance(constant view_project& vp, float2 half_window_size, float distance)
{
    float2 ortho_scale = vp.projection.xy * half_window_size;
    return distance * max(ortho_scale.x, ortho_scale.y);
}

// ---------------------------------------------------------------------------------------------------------------------------
kernel void transform(constant transform_arguments& input [[buffer(0)]],
                      ushort index [[thread_position_in_grid]])
{
    if (index >= input.num_commands)
        return;

    constant draw_command& cmd = input.commands[index];

    // prevent out of buffer access
    if (cmd.viewproject_index >= MAX_VIEWPROJECT || cmd.sdform_index >= MAX_SDFORMS)
        return;

    constant view_project& vp = input.vp[cmd.viewproject_index];
    constant sdform& sdform = input.forms[cmd.sdform_index];
    command_type type = primitive_get_type(cmd.type);
    device float* data = &input.draw_data[cmd.data_index];
    float2 half_window_size = input.window_size * .5f;

    switch(type)
    {
    case primitive_disc :
        {
            float2 center = transform_point(vp, sdform, half_window_size, float2(data[0], data[1]));
            float radius = transform_distance(vp, half_window_size, data[2]);
            break;
        }
    case primitive_oriented_box :
        {
            float2 p0 = transform_point(vp, sdform, half_window_size, float2(data[0], data[1]));
            float2 p1 = transform_point(vp, sdform, half_window_size, float2(data[2], data[3]));
            float width = transform_distance(vp, half_window_size, data[4]);

            
            break;
        }
    }
}