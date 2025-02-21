#include "export.h"

#include <stdio.h>
#include <stdarg.h>

#include "primitive.h"
#include "../system/arc.h"
#include "../system/log.h"
#include "../shaders/shadertoy_boilerplate.h"

//----------------------------------------------------------------------------------------------------------------------------
void shadertoy_start(struct string_buffer* b)
{
    const uint8_t* boiler_plate = shadertoy_boilerplate_shader;
    while (*boiler_plate != 0 && b->remaining > 0)
    {
        *(b->current++) = *(boiler_plate++);
        b->remaining++;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void shadertoy_export_primitive(struct string_buffer* b, struct primitive * const p, uint32_t index, float smooth_value)
{
    if (p->m_Shape == shape_spline)
    {
        for(uint32_t arc_index=0; arc_index<p->m_NumArcs; ++arc_index)
        {
            bprintf(b, "\tfloat d%d_%d = ", index, arc_index);
            bprintf(b, "sd_oriented_ring(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f, %f);\n",
                    p->m_Arcs[arc_index].center.x, p->m_Arcs[arc_index].center.y,
                    p->m_Arcs[arc_index].direction.x, p->m_Arcs[arc_index].direction.y, sinf(p->m_Arcs[arc_index].aperture), cosf(p->m_Arcs[arc_index].aperture),
                    p->m_Arcs[arc_index].radius, p->m_Thickness);
            bprintf(b, "\tblend = smooth_minimum(max(d, 0.0), d%d_%d, pixel_size);\n", index, arc_index);
            bprintf(b, "\td = blend.x;\n");
            bprintf(b, "\tcolor = mix(color, vec3(%f, %f, %f), blend.y);\n", p->m_Color.red, p->m_Color.green, p->m_Color.blue);
        }
        return;
    }

    bprintf(b, "\tfloat d%d = ", index);

    switch(p->m_Shape)
    {
        case shape_disc : bprintf(b, "sd_disc(p, vec2(%f, %f), %f);\n", p->m_Points[0].x, p->m_Points[0].y, p->m_Roundness);break;

        case shape_oriented_box : bprintf(b, "sd_oriented_box(p, vec2(%f, %f), vec2(%f, %f), %f);\n", 
                                          p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Width);break;

        case shape_triangle : bprintf(b, "sd_triangle(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f));\n",
                                      p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Points[2].x, p->m_Points[2].y);break;

        case shape_oriented_ellipse : bprintf(b, "sd_oriented_ellipse(p, vec2(%f, %f), vec2(%f, %f), %f);\n",
                                             p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Width);break;

        case shape_pie : 
        {
            bprintf(b, "sd_oriented_pie(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f);\n", 
                    p->m_Points[0].x, p->m_Points[0].y, p->m_Direction.x, p->m_Direction.y,
                    sinf(p->m_Aperture), cosf(p->m_Aperture), p->m_Radius); 
            break;
        }

        case shape_arc :
        {
            bprintf(b, "sd_oriented_ring(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f, %f);\n",
                    p->m_Center.x, p->m_Center.y, p->m_Direction.x, p->m_Direction.y, 
                    sinf(p->m_Aperture), cosf(p->m_Aperture), p->m_Radius, p->m_Thickness); 
            break;
        }

        case shape_uneven_capsule:
        {
            bprintf(b, "sd_uneven_capsule(p, vec2(%f, %f), vec2(%f, %f), %f, %f);\n", 
                    p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Roundness, p->m_Radius);
            break;
        }

        default: log_error("shape type %d cannot be exported", p->m_Shape); break;
    }

    switch(p->m_Operator)
    {
        case op_add : 
        {
            bprintf(b, "\tblend = smooth_minimum(max(d, 0.0), d%d, pixel_size);\n", index);
            bprintf(b, "\td = blend.x;\n");
            bprintf(b, "\tcolor = mix(color, vec3(%f, %f, %f), blend.y);\n", p->m_Color.red, p->m_Color.green, p->m_Color.blue);
            break;
        }
        case op_union : 
        {
            bprintf(b, "\tblend = smooth_minimum(max(d, 0.0), d%d, %f);\n", index, smooth_value);
            bprintf(b, "\td = blend.x;\n");
            bprintf(b, "\tcolor = mix(color, vec3(%f, %f, %f), blend.y);\n", p->m_Color.red, p->m_Color.green, p->m_Color.blue);
            break;
        }
        
        case op_subtraction : bprintf(b, "\td = smooth_substraction(d, d%d, 0.0);\n", index);break;
        default:break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void shadertoy_finalize(struct string_buffer* b)
{
    bprintf(b, "\n\treturn vec4(color, d);\n}\n\n");
    bprintf(b, "//--------------\n// Pixel shader\n//--------------\n");
    bprintf(b, "void mainImage( out vec4 fragColor, in vec2 fragCoord)\n");
    bprintf(b, "{\n\tvec2 p = fragCoord/iResolution.y;\n");
    bprintf(b, "\tp.y = 1.0 - p.y;\n");
    bprintf(b, "\tvec4 color_distance = map(p);\n");
    bprintf(b, "\tvec3 col = mix(vec3(1.0), vec3(color_distance.rgb), 1.0-smoothstep(0.0,length(dFdx(p) + dFdy(p)), color_distance.a));\n");
    bprintf(b, "\tfragColor = vec4(pow(col, vec3(1.0/ 2.2)),1.0);\n}\n");
}
