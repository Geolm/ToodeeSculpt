#include <metal_stdlib>
#include "common.h"


// ---------------------------------------------------------------------------------------------------------------------------
kernel void transform(constant transform_arguments& input [[buffer(0)]],
                      ushort index [[thread_position_in_grid]])
{

}