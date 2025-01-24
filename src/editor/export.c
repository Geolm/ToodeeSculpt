#include "export.h"

#include <stdio.h>
#include <stdarg.h>

#include "primitive.h"
#include "../system/arc.h"
#include "../system/log.h"
#include "../shaders/shadertoy_boilerplate.h"

//----------------------------------------------------------------------------------------------------------------------------
char* copy_boiler_plate(char* clipboard_buffer, size_t* remaining_size)
{
    const char* boiler_plate = shadertoy_boilerplate_shader;
    while (*boiler_plate != 0 && remaining_size > 0)
    {
        *(clipboard_buffer++) = *(boiler_plate++);
        *remaining_size -= 1;
    }
    return clipboard_buffer;
}

//----------------------------------------------------------------------------------------------------------------------------
void print_clipboard(char** clipboard_buffer, size_t* remaining_size, const char* string, ...)
{
    va_list args;
    va_start(args, string);
    int num_char_written = vsnprintf(*clipboard_buffer, *remaining_size, string, args);
    va_end(args);

    if (num_char_written > 0)
    {
        *clipboard_buffer += num_char_written;
        *remaining_size -= num_char_written;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_export_shadertoy(struct primitive * const p, uint32_t index, float smooth_value)
{
    if (p->m_Shape == shape_spline)
    {
        for(uint32_t arc_index=0; arc_index<p->m_NumArcs; ++arc_index)
        {
            printf("\tfloat d%d_%d = ", index, arc_index);
            printf("sd_oriented_ring(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f, %f);\n", p->m_Arcs[arc_index].center.x, p->m_Arcs[arc_index].center.y,
                    p->m_Arcs[arc_index].direction.x, p->m_Arcs[arc_index].direction.y, sinf(p->m_Arcs[arc_index].aperture), cosf(p->m_Arcs[arc_index].aperture),
                    p->m_Arcs[arc_index].radius, p->m_Thickness);
            printf("\tblend = smooth_minimum(max(d, 0.0), d%d_%d, pixel_size);\n", index, arc_index);
            printf("\td = blend.x;\n");
            printf("\tcolor = mix(color, vec3(%f, %f, %f), blend.y);\n", p->m_Color.red, p->m_Color.green, p->m_Color.blue);
        }
        return;
    }

    printf("\tfloat d%d = ", index);

    switch(p->m_Shape)
    {
        case shape_disc : printf("sd_disc(p, vec2(%f, %f), %f);\n", p->m_Points[0].x, p->m_Points[0].y, p->m_Roundness);break;

        case shape_oriented_box : printf("sd_oriented_box(p, vec2(%f, %f), vec2(%f, %f), %f);\n", 
                                             p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Width);break;

        case shape_triangle : printf("sd_triangle(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f));\n",
                                         p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Points[2].x, p->m_Points[2].y);break;

        case shape_oriented_ellipse : printf("sd_oriented_ellipse(p, vec2(%f, %f), vec2(%f, %f), %f);\n",
                                        p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Width);break;

        case shape_pie : 
        {
            printf("sd_oriented_pie(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f);\n", 
                                    p->m_Points[0].x, p->m_Points[0].y, p->m_Direction.x, p->m_Direction.y,
                                    sinf(p->m_Aperture), cosf(p->m_Aperture), p->m_Radius); 
            break;
        }

        case shape_arc :
        {
            printf("sd_oriented_ring(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f, %f);\n",
                                     p->m_Center.x, p->m_Center.y, p->m_Direction.x, p->m_Direction.y, 
                                     sinf(p->m_Aperture), cosf(p->m_Aperture), p->m_Radius, p->m_Thickness); 
            break;
        }

        case shape_uneven_capsule:
        {
            printf("sd_uneven_capsule(p, vec2(%f, %f), vec2(%f, %f), %f, %f);\n", 
                    p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Roundness, p->m_Radius);
            break;
        }

        default: log_error("shape type %d cannot be exported", p->m_Shape); break;
    }

    switch(p->m_Operator)
    {
        case op_add : 
        {
            printf("\tblend = smooth_minimum(max(d, 0.0), d%d, pixel_size);\n", index);
            printf("\td = blend.x;\n");
            printf("\tcolor = mix(color, vec3(%f, %f, %f), blend.y);\n", p->m_Color.red, p->m_Color.green, p->m_Color.blue);
            break;
        }
        case op_union : 
        {
            printf("\tblend = smooth_minimum(max(d, 0.0), d%d, %f);\n", index, smooth_value);
            printf("\td = blend.x;\n");
            printf("\tcolor = mix(color, vec3(%f, %f, %f), blend.y);\n", p->m_Color.red, p->m_Color.green, p->m_Color.blue);
            break;
        }
        
        case op_subtraction : printf("\td = smooth_substraction(d, d%d, 0.0);\n", index);break;
        default:break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void finish_shadertoy(char** clipboard_buffer, size_t* remaining_size)
{
    print_clipboard(clipboard_buffer, remaining_size, "\n\treturn vec4(color, d);\n}\n\n");
    print_clipboard(clipboard_buffer, remaining_size, "//--------------\n// Pixel shader\n//--------------\n");
    print_clipboard(clipboard_buffer, remaining_size, "void mainImage( out vec4 fragColor, in vec2 fragCoord)\n");
    print_clipboard(clipboard_buffer, remaining_size, "{\n\tvec2 p = fragCoord/iResolution.y;\n");
    print_clipboard(clipboard_buffer, remaining_size, "\tp.y = 1.0 - p.y;\n");
    print_clipboard(clipboard_buffer, remaining_size, "\tvec4 color_distance = map(p);\n");
    print_clipboard(clipboard_buffer, remaining_size, "\tvec3 col = mix(vec3(1.0), vec3(color_distance.rgb), 1.0-smoothstep(0.0,length(dFdx(p) + dFdy(p)), color_distance.a));\n");
    print_clipboard(clipboard_buffer, remaining_size, "\tfragColor = vec4(col,1.0);\n}\n");
}
