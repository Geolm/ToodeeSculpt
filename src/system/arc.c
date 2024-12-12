#include "arc.h"

//-----------------------------------------------------------------------------
// https://www.xarg.org/2018/02/create-a-circle-out-of-three-points/
void circle_from_points(vec2 p0, vec2 p1, vec2 p2, vec2* center, float* radius)
{
    float a = p0.x * (p1.y - p2.y) - p0.y * (p1.x - p2.x) + p1.x * p2.y - p2.x * p1.y;
    vec2 max = vec2_max3(p0, p1, p2);
    float threshold = float_max(max.x, max.y) * .01f;

    // colinear points
    if (fabsf(a) < threshold)
    {
        *radius = -1.f;
    }
    else
    {
        float b = (p0.x * p0.x + p0.y * p0.y) * (p2.y - p1.y) 
                + (p1.x * p1.x + p1.y * p1.y) * (p0.y - p2.y)
                + (p2.x * p2.x + p2.y * p2.y) * (p1.y - p0.y);

        float c = (p0.x * p0.x + p0.y * p0.y) * (p1.x - p2.x) 
                + (p1.x * p1.x + p1.y * p1.y) * (p2.x - p0.x) 
                + (p2.x * p2.x + p2.y * p2.y) * (p0.x - p1.x);

        center->x = -b / (2.f * a);
        center->y = -c / (2.f * a);
        *radius = vec2_distance(*center, p0);
    }
}

//-----------------------------------------------------------------------------
void arc_from_points(vec2 p0, vec2 p1, vec2 p2, vec2* center, vec2* direction, float* aperture, float* radius)
{
    circle_from_points(p0, p1, p2, center, radius);

    if (*radius >= 0.f)
    {
        vec2 center_p0 = vec2_sub(p0, *center); vec2_normalize(&center_p0);
        vec2 center_p1 = vec2_sub(p1, *center); vec2_normalize(&center_p1);
        vec2 center_p2 = vec2_sub(p2, *center); vec2_normalize(&center_p2);
 
        *direction = vec2_add(center_p0, center_p2);
        vec2_normalize(direction);

        *aperture = acosf(vec2_dot(center_p0, *direction));

        // if p1 is out of cone, reverse cone in order to get all 3 points on the arc
        if (vec2_dot(*direction, center_p1) < vec2_dot(*direction, center_p2))
        {
            *direction = vec2_scale(*direction, -1.f);
            *aperture = VEC2_PI - *aperture;
        }
    }
}