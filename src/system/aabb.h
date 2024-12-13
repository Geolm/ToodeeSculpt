#ifndef __AABB__H__
#define __AABB__H__

#include "vec2.h"
#include <float.h>

typedef struct {vec2 min, max;} aabb;

enum aabb_corners
{
    aabb_top_left,
    aabb_top_right,
    aabb_bottom_left,
    aabb_botton_right
};

static aabb aabb_invalid(void);
static void aabb_grow(aabb* box, vec2 amount);
static void aabb_encompass(aabb* box, vec2 point);
static aabb aabb_merge(aabb box0, aabb box1);
static void aabb_scale(aabb* box, float scale);
static vec2 aabb_get_vertex(const aabb* box, enum aabb_corners corner);
static vec2 aabb_get_size(const aabb* box);
static vec2 aabb_get_uv(const aabb* box, vec2 pos);
static vec2 aabb_get_uv_clamped(const aabb* box, vec2 pos);
static vec2 aabb_bilinear(const aabb* box, vec2 uv);
static vec2 aabb_get_center(const aabb* box);
static aabb aabb_from_bezier(vec2 p0, vec2 p1, vec2 p2);
static aabb aabb_from_capsule(vec2 p0, vec2 p1, float radius);
static aabb aabb_from_obb(vec2 p0, vec2 p1, float width);
static aabb aabb_from_circle(vec2 center, float radius);
static aabb aabb_from_triangle(vec2 v0, vec2 v1, vec2 v2);
static aabb aabb_from_center(vec2 center, vec2 extent);
static bool aabb_test_point(const aabb* box, vec2 point);

//-----------------------------------------------------------------------------
static inline bool aabb_is_valid(aabb box)
{
    return vec2_all_greater(box.max, box.min);
}

//-----------------------------------------------------------------------------
static inline aabb aabb_invalid(void)
{
    return (aabb) {.min = vec2_splat(FLT_MAX), .max = vec2_splat(-FLT_MAX)};
}

//-----------------------------------------------------------------------------
static inline bool aabb_test_point(const aabb* box, vec2 point)
{
    return !(point.x < box->min.x || point.y < box->min.y || point.x > box->max.x || point.y > box->max.y);
}

//-----------------------------------------------------------------------------
static inline void aabb_grow(aabb* box, vec2 amount)
{
    box->min = vec2_sub(box->min, amount);
    box->max = vec2_add(box->max, amount);
}

//-----------------------------------------------------------------------------
static inline void aabb_encompass(aabb* box, vec2 point)
{
    box->min = vec2_min(box->min, point);
    box->max = vec2_max(box->max, point);
}

//-----------------------------------------------------------------------------
static inline void aabb_scale(aabb* box, float scale)
{
    vec2 extent = vec2_scale(vec2_sub(box->max, box->min), .5f * scale);
    vec2 center = vec2_scale(vec2_add(box->max, box->min), .5f);
    box->min = vec2_sub(center, extent);
    box->max = vec2_add(center, extent);
}

//-----------------------------------------------------------------------------
static inline vec2 aabb_get_vertex(const aabb* box, enum aabb_corners corner)
{
    switch(corner)
    {
    case aabb_top_left : return box->min;
    case aabb_top_right : return (vec2) {box->max.x, box->min.y};
    case aabb_bottom_left : return (vec2) {box->min.x, box->max.y};
    default: return box->max;
    }
}

//-----------------------------------------------------------------------------
static inline vec2 aabb_get_size(const aabb* box)
{
    return vec2_sub(box->max, box->min);
}

//-----------------------------------------------------------------------------
static inline vec2 aabb_get_uv(const aabb* box, vec2 pos)
{
    vec2 uv = vec2_sub(pos, box->min);
    return vec2_div(uv, aabb_get_size(box));
}

//-----------------------------------------------------------------------------
static inline vec2 aabb_get_uv_clamped(const aabb* box, vec2 pos)
{
    return vec2_saturate(aabb_get_uv(box, pos));
}

//-----------------------------------------------------------------------------
static inline vec2 aabb_bilinear(const aabb* box, vec2 uv)
{
    return vec2_add(vec2_mul(box->min, vec2_sub(vec2_splat(1.f), uv)), vec2_mul(box->max, uv));
}

//-----------------------------------------------------------------------------
static inline vec2 aabb_get_center(const aabb* box)
{
    return vec2_scale(vec2_add(box->min, box->max), .5f);
}

//-----------------------------------------------------------------------------
static inline aabb aabb_merge(aabb box0, aabb box1)
{
    return (aabb) {.min = vec2_min(box0.min, box1.min), .max = vec2_max(box0.max, box1.max)};
}

//-----------------------------------------------------------------------------
static inline aabb aabb_from_circle(vec2 center, float radius)
{
    return (aabb)
    {
        .min = vec2_sub(center, vec2_splat(radius)),
        .max = vec2_add(center, vec2_splat(radius))
    };
}

//-----------------------------------------------------------------------------
static inline aabb aabb_from_triangle(vec2 v0, vec2 v1, vec2 v2)
{
    return (aabb)
    {
        .min = vec2_min3(v0, v1, v2),
        .max = vec2_max3(v0, v1, v2)
    };
}

//-----------------------------------------------------------------------------
static inline aabb aabb_from_center(vec2 center, vec2 extent)
{
    return (aabb) {.min = vec2_sub(center, extent), .max = vec2_add(center, extent)};
}

//-----------------------------------------------------------------------------
// based on https://iquilezles.org/articles/bezierbbox/
static inline aabb aabb_from_bezier(vec2 p0, vec2 p1, vec2 p2)
{
    aabb box;

    box.min = vec2_min(p0, p2);
    box.max = vec2_max(p0, p2);

    if (!aabb_test_point(&box, p1))
    {
        vec2 t = vec2_saturate(vec2_div(vec2_sub(p0, p1), vec2_add(vec2_add(p0, vec2_scale(p1, -2.f)), p2)));
        vec2 s = vec2_sub(vec2_splat(1.f), t);
        vec2 q = vec2_mul(p0, vec2_mul(s, s));
        q = vec2_add(q, vec2_mul(vec2_mul(vec2_scale(s, 2.f), t), p1));
        q = vec2_add(q, vec2_mul(p2, vec2_mul(t, t)));

        box.min = vec2_min(box.min, q);
        box.max = vec2_max(box.max, q);
    }

    return box;
}

//-----------------------------------------------------------------------------
static inline aabb aabb_from_obb(vec2 p0, vec2 p1, float width)
{
    aabb box;

    vec2 direction = vec2_sub(p1, p0);
    vec2 normal = vec2_skew(direction);
    vec2_normalize(&normal);
    normal = vec2_scale(normal, width*.5f);

    vec2 vertices[4];
    vertices[0] = vec2_add(p0, normal);
    vertices[1] = vec2_sub(p0, normal);
    vertices[2] = vec2_sub(p1, normal);
    vertices[3] = vec2_add(p1, normal);

    box.min = vec2_min4(vertices[0], vertices[1], vertices[2], vertices[3]);
    box.max = vec2_max4(vertices[0], vertices[1], vertices[2], vertices[3]);

    return box;
}
//-----------------------------------------------------------------------------
static inline aabb aabb_from_rounded_obb(vec2 p0, vec2 p1, float width, float border)
{
    aabb box;

    vec2 dir = vec2_sub(p1, p0);
    vec2_normalize(&dir);
    vec2 normal = vec2_skew(dir);

    normal = vec2_scale(normal, width*.5f + border);
    dir = vec2_scale(dir, border);
    p0 = vec2_sub(p0, dir);
    p1 = vec2_add(p1, dir);

    vec2 vertices[4];
    vertices[0] = vec2_add(p0, normal);
    vertices[1] = vec2_sub(p0, normal);
    vertices[2] = vec2_sub(p1, normal);
    vertices[3] = vec2_add(p1, normal);

    box.min = vec2_min4(vertices[0], vertices[1], vertices[2], vertices[3]);
    box.max = vec2_max4(vertices[0], vertices[1], vertices[2], vertices[3]);

    return box;
}

//-----------------------------------------------------------------------------
static inline aabb aabb_from_capsule(vec2 p0, vec2 p1, float radius)
{
    aabb box;

    vec2 dir = vec2_sub(p1, p0);
    vec2_normalize(&dir);
    vec2 normal = vec2_skew(dir);

    normal = vec2_scale(normal, radius);
    dir = vec2_scale(dir, radius);
    p0 = vec2_sub(p0, dir);
    p1 = vec2_add(p1, dir);

    vec2 vertices[4];
    vertices[0] = vec2_add(p0, normal);
    vertices[1] = vec2_sub(p0, normal);
    vertices[2] = vec2_sub(p1, normal);
    vertices[3] = vec2_add(p1, normal);

    box.min = vec2_min4(vertices[0], vertices[1], vertices[2], vertices[3]);
    box.max = vec2_max4(vertices[0], vertices[1], vertices[2], vertices[3]);

    return box;
}


#endif
