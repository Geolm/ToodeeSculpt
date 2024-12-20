#include "export.h"

#include <stdio.h>

#include "primitive.h"
#include "../system/arc.h"

void primitive_export_shadertoy(struct primitive * const p, uint32_t index, float smooth_value)
{
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
        default:break;
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