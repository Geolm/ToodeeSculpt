#include "biarc.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// based on κ-Curves: Interpolation at Local Maximum Curvature
float compute_t(vec2 p0, vec2 p1, vec2 p2)
{
    vec2 c2c0 = vec2_sub(p2, p0);
    vec2 c0cp = vec2_sub(p0, p1);
    float a = vec2_dot(c2c0, c2c0);
    float b = 3.f * vec2_dot(c2c0, c0cp);
    float c = vec2_dot(vec2_sub(vec2_sub(vec2_scale(p0, 3.f), vec2_scale(p1, 2.f)), p2), c0cp);
    float d = -vec2_dot(c0cp, c0cp);
    float p = (3.f * a * c - b * b) / 3.f / a / a;
	float q = (2.f * b * b * b - 9.f * a * b * c + 27.f * a * a * d) / 27.f / a / a / a;
    if (4.f * p * p * p + 27.f * q * q >= 0)
    {
        // single real root
        return cbrtf(-q / 2.f + sqrtf(q * q / 4.f + p * p * p / 27.f)) + cbrtf(-q / 2.f - sqrtf(q * q / 4.f + p * p * p / 27.f)) - b / 3.f / a;
    }
    else
    {
        // three real roots
        for (int k = 0; k < 3; ++k)
        {
            float t = 2 * sqrtf(-p / 3.f) * cosf(1. / 3.f * acos(3.f * q / 2.f / p * sqrtf(-3.f / p)) - 2.f * VEC2_PI * k / 3.f) - b / 3.f / a;
            if (0.f <= t && t <= 1.f)
                return t;
        }
    }
    // error, return a non-error t
    return 0.5f;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// outputs the control points of the quadratic bezier curve that goes though all points (p0, p1, p2)
void bezier_from_path(vec2 p0, vec2 p1, vec2 p2, vec2* output)
{
    float t = compute_t(p0, p1, p2);
    float om_t = 1.f - t;
    float om_t_sq = om_t * om_t;
    float t_sq = t * t;

    vec2 ctrl_pt = vec2_sub(p1, vec2_scale(p0, om_t_sq));
    ctrl_pt = vec2_sub(ctrl_pt, vec2_scale(p2, t_sq));
    ctrl_pt = vec2_div(ctrl_pt, vec2_splat((2.f * om_t * t)));

    output[0] = p0;
    output[1] = ctrl_pt;
    output[2] = p2;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
vec2 triangle_incenter(vec2 p0, vec2 p1, vec2 p2)
{
    float a = vec2_distance(p2, p1);
    float b = vec2_distance(p2, p0);
    float c = vec2_distance(p1, p0);

    float perimeter = a + b + c;
    vec2 incenter = vec2_scale(p0, a);
    incenter = vec2_add(incenter, vec2_scale(p1, b));
    incenter = vec2_add(incenter, vec2_scale(p2, c));
    return vec2_scale(incenter, 1.f / perimeter);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
bool intersection_line_line(vec2 p0, vec2 d0, vec2 p1, vec2 d1, vec2* intersection_point)
{
    float det = vec2_cross(d0, d1);

    if (fabsf(det) < 1e-3f) 
        return false;

    float t = ((p1.x - p0.x) * d1.y - (p1.y - p0.y) * d1.x) / det;
    *intersection_point = vec2_add(p0, vec2_scale(d0, t));
    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// generate a bi-arcs curve from a quadratic bezier curve
// based on https://dlacko.org/blog/2016/10/19/approximating-bezier-curves-by-biarcs/
bool biarc_generate(vec2 p0, vec2 p1, vec2 p2, struct arc* arcs)
{
    if (fabsf(vec2_cross(vec2_sub(p1, p0), vec2_sub(p2, p1))) < 1.e-3f)
        return false;

    vec2 ctrl_pts[3];
    bezier_from_path(p0, p1, p2, ctrl_pts);

    vec2 incenter = triangle_incenter(ctrl_pts[0], ctrl_pts[1], ctrl_pts[2]);
    vec2 tangent = vec2_sub(ctrl_pts[1], ctrl_pts[0]);
    vec2 middle = vec2_scale(vec2_add(ctrl_pts[0], incenter), .5f);

    if (!intersection_line_line(ctrl_pts[0], vec2_skew(tangent), middle, vec2_skew(vec2_sub(incenter, ctrl_pts[0])), &arcs[0].center))
        return false;
    
    arcs[0].direction = vec2_normalized(vec2_sub(middle, arcs[0].center)); 
    arcs[0].radius = vec2_distance(arcs[0].center, ctrl_pts[0]);
    arcs[0].aperture = acosf(vec2_dot(arcs[0].direction, vec2_normalized(vec2_sub(incenter, arcs[0].center))));

    tangent = vec2_sub(ctrl_pts[2], ctrl_pts[1]);
    middle = vec2_scale(vec2_add(ctrl_pts[2], incenter), .5f);

    if (!intersection_line_line(ctrl_pts[2], vec2_skew(tangent), middle, vec2_skew(vec2_sub(incenter, ctrl_pts[2])), &arcs[1].center))
        return false;

    arcs[1].direction = vec2_normalized(vec2_sub(middle, arcs[1].center));
    arcs[1].radius = vec2_distance(arcs[1].center, ctrl_pts[2]);
    arcs[1].aperture = acosf(vec2_dot(arcs[1].direction, vec2_normalized(vec2_sub(incenter, arcs[1].center))));

    return true;
}
