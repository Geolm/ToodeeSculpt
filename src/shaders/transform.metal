#include <metal_stdlib>
#include "common.h"


// ---------------------------------------------------------------------------------------------------------------------------
kernel void transform(constant transform_arguments& input [[buffer(0)]],
                      ushort index [[thread_position_in_grid]])
{
    if (index >= input.num_commands)
        return;

    device draw_command& cmd = input.commands[index];

    // prevent out of buffer access
    if (cmd.viewproject_index >= MAX_VIEWPROJECT || cmd.sdform_index >= MAX_SDFORMS)
        return;

    constant view_project& vp = input.vp[cmd.viewproject_index];
    constant sdform& sdform = input.forms[cmd.sdform_index];
    command_type type = primitive_get_type(cmd.type);
    device float* data = &input.draw_data[cmd.data_index];

    switch(type)
    {
        
    }
}