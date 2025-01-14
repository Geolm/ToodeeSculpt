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
void biarc_from_points_tangents(vec2 p0, vec2 p1, float theta0, float theta1, struct arc* arcs)
{
    // compute the distance and angle between points
    vec2 direction = vec2_sub(p1, p0);
    float d = vec2_length(direction);
    float omega = vec2_atan2(direction);

    // compute the junction angle
    float theta_j = 2.f * omega - (theta0 + theta1) * .5f;

    // compute curvatures
    float kappa0 = (2.0f * sinf((theta_j - theta0) * 0.5f)) / (d * sinf((theta_j - theta0) * 0.5f) / (theta_j - theta0));
    float kappa1 = -(2.0f * sinf((theta1 - theta_j) * 0.5f)) / (d * sinf((theta1 - theta_j) * 0.5f) / (theta1 - theta_j));
    float one_over_kappa0 = 1.0f / kappa0;
    float one_over_kappa1 = 1.0f / kappa1;

    // compute arc lengths
    float len0 = 2.0f * sinf((theta_j - theta0) * 0.5f) * one_over_kappa0;
    float len1 = 2.0f * sinf((theta1 - theta_j) * 0.5f) * one_over_kappa1;

    // compute junction point
    vec2 delta0 = vec2_scale((vec2) {sinf(theta0), -cosf(theta0)}, one_over_kappa0);
    vec2 junction = vec2_add(p0, vec2_mul(vec2_set(sin((theta0 + kappa0 * len0 * 0.5)), cos((theta0 + kappa0 * len0 * 0.5))), delta0));

    // generate arcs
    arcs[0].center = vec2_add(p0, vec2_scale(vec2_set(sinf(theta0), -cosf(theta0)), one_over_kappa0));
    arcs[1].center = vec2_add(p1, vec2_scale(vec2_set(sinf(theta1), -cosf(theta1)), one_over_kappa1));
    arcs[0].radius = fabsf(one_over_kappa0);
    arcs[1].radius = fabsf(one_over_kappa1);
}