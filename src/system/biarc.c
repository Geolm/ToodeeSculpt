#include "biarc.h"

#include "log.h"

static inline float relative_epsilon(vec2 a, vec2 b, float epsilon) {return vec2_distance(a, b) * epsilon;}

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
    ctrl_pt = vec2_scale(ctrl_pt, 1.f / (2.f * om_t * t));

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

    if (fabsf(det) < relative_epsilon(p0, p1, 1e-3f)) 
        return false;

    float t = ((p1.x - p0.x) * d1.y - (p1.y - p0.y) * d1.x) / det;
    *intersection_point = vec2_add(p0, vec2_scale(d0, t));
    return true;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// generate a bi-arcs curve from a quadratic bezier curve
// based on https://dlacko.org/blog/2016/10/19/approximating-bezier-curves-by-biarcs/
void biarc_from_path(vec2 p0, vec2 p1, vec2 p2, struct arc* arcs)
{
    vec2 c[3];
    bezier_from_path(p0, p1, p2, c);
    biarc_from_bezier(c[0], c[1], c[2], arcs);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void biarc_from_bezier(vec2 c0, vec2 c1, vec2 c2, struct arc* arcs)
{
    vec2 incenter = triangle_incenter(c0, c1, c2);
    vec2 tangent = vec2_sub(c1, c0);
    vec2 middle = vec2_scale(vec2_add(c0, incenter), .5f);

    if (intersection_line_line(c0, vec2_skew(tangent), middle, vec2_skew(vec2_sub(incenter, c0)), &arcs[0].center))
    {
        arcs[0].direction = vec2_normalized(vec2_sub(middle, arcs[0].center)); 
        arcs[0].radius = vec2_distance(arcs[0].center, c0);
        arcs[0].aperture = acosf(vec2_dot(arcs[0].direction, vec2_normalized(vec2_sub(incenter, arcs[0].center))));
    }
    else
    {
        // colinear vectors put the segment coordinates to be draw instead of an arc
        arcs[0].center = c0;
        arcs[0].direction = middle;
        arcs[0].radius = -1.f;
    }

    tangent = vec2_sub(c2, c1);
    middle = vec2_scale(vec2_add(c2, incenter), .5f);

    if (intersection_line_line(c2, vec2_skew(tangent), middle, vec2_skew(vec2_sub(incenter, c2)), &arcs[1].center))
    {
        arcs[1].direction = vec2_normalized(vec2_sub(middle, arcs[1].center));
        arcs[1].radius = vec2_distance(arcs[1].center, c2);
        arcs[1].aperture = acosf(vec2_dot(arcs[1].direction, vec2_normalized(vec2_sub(incenter, arcs[1].center))));
    }
    else
    {
        // colinear vectors put the segment coordinates to be draw instead of an arc
        arcs[1].center = c2;
        arcs[1].direction = middle;
        arcs[1].radius = -1.f;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void biarc_recursive(vec2 c0, vec2 c1, vec2 c2, uint32_t max_subdivision, float max_error, struct arc* arcs, uint32_t *num_arcs, uint32_t current_subdivision)
{
    vec2 incenter = triangle_incenter(c0, c1, c2);
    vec2 split = vec2_add(vec2_add(vec2_scale(c0, .25f), vec2_scale(c1, .5f)), vec2_scale(c2, .25f));
    vec2 split_tangent = vec2_sub(c2, c0);

    // the relative epsilon take in account the distance between c0 and c2, this has two advantages:
    //   - avoid replacing arc by line in the heart of the curve when segment are short
    //   - if the curve is elongated, it switches the long almost straigh chunk by a line to avoid "impossible" arc to be rasterize
    if (fabsf(vec2_cross(vec2_sub(c0, split), vec2_sub(c2, split))) < relative_epsilon(c0, c2, 1.e-1f))
    {
        arcs[*num_arcs].center = c0;
        arcs[*num_arcs].direction = c2;
        arcs[*num_arcs].radius = -1.f;
        (*num_arcs)++;
        return;
    }

    // measure how far from the bezier curve the approximation
    // if the approximation is perfect the incenter should be near the middle of the curve
    // if we're far and we did not reach the max subdisivion, we subdivise the curve
    if ((vec2_sq_distance(split, incenter) > float_square(max_error)) && (current_subdivision < max_subdivision))
    {
        vec2 mid_left, mid_right;
        
        if (intersection_line_line(c0, vec2_sub(c1, c0), split, split_tangent, &mid_left) &&
            intersection_line_line(split, split_tangent, c2, vec2_sub(c2, c1), &mid_right))
        {
            biarc_recursive(c0, mid_left, split, max_subdivision, max_error, arcs, num_arcs, current_subdivision+1);
            biarc_recursive(split, mid_right, c2, max_subdivision, max_error, arcs, num_arcs, current_subdivision+1);
        }
    }
    else
    {
        biarc_from_bezier(c0, c1, c2, &arcs[*num_arcs]);
        *num_arcs += 2;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
void biarc_tessellate(vec2 p0, vec2 p1, vec2 p2, uint32_t max_subdivision, float max_error, struct arc* arcs, uint32_t *num_arcs)
{
    vec2 c[3];
    bezier_from_path(p0, p1, p2, c);

    *num_arcs = 0;
    biarc_recursive(c[0], c[1], c[2], max_subdivision, max_error, arcs, num_arcs, 1);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
static inline float range(float theta)
{
    while (theta>VEC2_PI)
        theta -= VEC2_TAU;
    while (theta<-VEC2_PI)
        theta += VEC2_TAU;
    return theta;
}

static inline float sinc(float x) 
{
    if (fabsf(x) < 0.002f)
        return 1.f + (float_square(x) / 6.f) * (1.f - (float_square(x) / 20.f));
    return sinf(x) / x;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// based on Interpolating Splines of Biarcs from a Sequence of Planar Points
void biarc_from_points_tangents(vec2 p0, vec2 p1, float angle0, float angle1, struct arc* arcs)
{
    vec2 direction = vec2_sub(p1, p0);
    float d = vec2_length(direction);
    float omega = vec2_atan2(direction);

    float theta0 = omega + range(angle0 - omega);
    float theta1 = omega + range(angle1 - omega);
    float t = 2.f * cosf((theta1 - theta0) / 4.f) / d;
    float theta_j = (2.f * omega) - ((theta0 + theta1) * .5f);
    float delta_theta0 = (theta_j - theta0) * .5f;
    float delta_theta1 = (theta_j - theta1) * .5f;
    float k0 = 2.f * t * sinf(delta_theta0);
    float k1 = -2.f * t * sinf(delta_theta1);

    vec2 junction = vec2_add(p0, vec2_set(cosf((theta_j + theta0) * .5f ) / t, sinf((theta_j + theta0) * .5f ) / t));
    vec2 n0 = vec2_skew(vec2_angle(theta0));
    vec2 n1 = vec2_skew(vec2_angle(theta1));

    // generate arcs
    arcs[0].center = vec2_add(p0, vec2_scale(n0, 1.f / k0));
    arcs[0].radius = fabsf(1.f / k0);
    arcs[1].center = vec2_add(p1, vec2_scale(n1, 1.f / k1));
    arcs[1].radius = fabsf(1.f / k1);

    vec2 center_p = vec2_sub(p0, arcs[0].center);
    vec2 center_junction = vec2_sub(junction, arcs[0].center);

    arcs[0].direction = vec2_normalized(vec2_add(center_p, center_junction));
    arcs[0].aperture = acosf(vec2_dot(vec2_normalized(center_p), arcs[0].direction));
    if (float_sign(vec2_dot(arcs[0].direction, vec2_angle(theta0))) < 0.f)
    {
        arcs[0].direction = vec2_scale(arcs[0].direction, -1.f);
        arcs[0].aperture = VEC2_PI - arcs[0].aperture;
    }

    center_p = vec2_sub(p1, arcs[1].center);
    center_junction = vec2_sub(junction, arcs[1].center);

    arcs[1].direction = vec2_normalized(vec2_add(center_p, center_junction));
    arcs[1].aperture = acosf(vec2_dot(vec2_normalized(center_p), arcs[1].direction));
    if (float_sign(vec2_dot(arcs[1].direction, vec2_angle(theta1))) > 0.f)
    {
        arcs[1].direction = vec2_scale(arcs[1].direction, -1.f);
        arcs[1].aperture = VEC2_PI - arcs[1].aperture;
    }
}
