#include "export.h"

#include <stdio.h>

#include "primitive.h"
#include "../system/arc.h"

void primitive_export_shadertoy(struct primitive * const p, uint32_t index, float smooth_value)
{
    printf("\tfloat d%d = ", index);

    switch(p->m_Type)
    {
        case primitive_disc : printf("sd_disc(p, vec2(%f, %f), %f);\n", p->m_Points[0].x, p->m_Points[0].y, p->m_Roundness);break;

        case primitive_oriented_box : printf("sd_oriented_box(p, vec2(%f, %f), vec2(%f, %f), %f);\n", 
                                             p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Width);break;

        case primitive_triangle : printf("sd_triangle(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f));\n",
                                         p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Points[2].x, p->m_Points[2].y);break;

        case primitive_ellipse : printf("sd_oriented_ellipse(p, vec2(%f, %f), vec2(%f, %f), %f);\n",
                                        p->m_Points[0].x, p->m_Points[0].y, p->m_Points[1].x, p->m_Points[1].y, p->m_Width);break;

        case primitive_pie : 
        {
            vec2 direction = vec2_sub(p->m_Points[1], p->m_Points[0]);
            float radius = vec2_normalize(&direction);
            direction = vec2_scale(direction, 1.f/radius);
        
            printf("sd_oriented_pie(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f);\n", 
                                    p->m_Points[0].x, p->m_Points[0].y, direction.x, direction.y,
                                    sinf(p->m_Aperture), cosf(p->m_Aperture), radius); 
            break;
        }

        case primitive_ring :
        {
            vec2 center, direction;
            float aperture, radius;
            arc_from_points(p->m_Points[0], p->m_Points[1], p->m_Points[2], &center, &direction, &aperture, &radius);

            printf("sd_oriented_ring(p, vec2(%f, %f), vec2(%f, %f), vec2(%f, %f), %f, %f);\n",
                                     center.x, center.y, direction.x, direction.y, 
                                     sinf(aperture), cosf(aperture), radius, p->m_Thickness); 
            break;
        }
        default:break;
    }

    switch(p->m_Operator)
    {
        case op_add : printf("\td = smooth_minimum(d, d%d, 0.0).x;\n", index);break;
        case op_union : printf("\td = smooth_minimum(d, d%d, %f).x;\n", index, smooth_value);break;
        case op_subtraction : printf("\td = smooth_substraction(d, d%d, 0.0);\n", index);break;
        default:break;
    }
}