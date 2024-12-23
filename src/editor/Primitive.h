#ifndef __PRIMITIVE_BASE__H__
#define __PRIMITIVE_BASE__H__

#include <assert.h>
#include "../system/aabb.h"
#include "../system/color.h"
#include "../shaders/common.h"
#include "../system/serializer.h"
#include "../system/palettes.h"
#include "../system/biarc.h"

enum {PRIMITIVE_MAXPOINTS = 3};

#define primitive_point_radius (6.f)
#define primitive_max_thickness (100.f)
#define primitive_curve_max_tessellation (6)
#define primitive_colinear_threshold (0.1f)

// almost in sync with primitive_type to not invalid old files
enum primitive_shape
{
    shape_invalid = INVALID_INDEX,
    shape_oriented_box = 2,
    shape_disc = 3,
    shape_oriented_ellipse = 5,
    shape_triangle = 4,
    shape_pie = 6,
    shape_arc = 7,
    shape_curve = 8
};

struct primitive
{
    struct arc m_Arcs[1<<primitive_curve_max_tessellation];
    aabb m_AABB;
    vec2 m_Points[PRIMITIVE_MAXPOINTS];
    vec2 m_Direction;
    vec2 m_Center;
    float m_Width; 
    float m_Aperture;
    float m_Roundness;
    float m_Thickness;
    float m_Radius;
    enum primitive_shape m_Shape;
    int m_Filled;
    uint32_t m_NumArcs;
    enum sdf_operator m_Operator;
    color4f m_Color;
};

extern struct palette primitive_palette;

#ifdef __cplusplus
extern "C" {
#endif

struct mu_Context;

void primitive_init(struct primitive* p, enum primitive_shape type, enum sdf_operator op, color4f color, float roundness, float width);
bool primitive_test_mouse_cursor(struct primitive const* primitive, vec2 mouse_position, bool test_vertices);
float primitive_distance_to_nearest_point(struct primitive const* primitive, vec2 reference);
void primitive_update_aabb(struct primitive* primitive);
int primitive_property_grid(struct primitive* primitive, struct mu_Context* gui_context);
vec2 primitive_compute_center(struct primitive const* primitive);
int primitive_contextual_property_grid(struct primitive* primitive, struct mu_Context* gui_context, bool *over_popup);
void primitive_deserialize(struct primitive* primitive, serializer_context* context, uint16_t major, uint16_t minor);
void primitive_serialize(struct primitive const* primitive, serializer_context* context);
void primitive_translate(struct primitive* p, vec2 translation);
void primitive_rotate(struct primitive* p, float angle);
void primitive_scale(struct primitive* p, float scale);
void primitive_normalize(struct primitive* p, const aabb* box);
void primitive_expand(struct primitive* p, const aabb* box);
void primitive_draw(struct primitive* p, void* renderer, float roundness, draw_color color, enum sdf_operator op);
void primitive_draw_gizmo(struct primitive* p, void* renderer, draw_color color);
void primitive_draw_alpha(struct primitive* p, void* renderer, float alpha);
void primitive_draw_aabb(struct primitive* p, void* renderer, draw_color color);
void primitive_draw_curve(void * renderer, vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color);


#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------------------------------------------------------
static inline uint32_t primitive_get_num_points(enum primitive_shape shape)
{
    switch(shape)
    {
    case shape_disc: return 1;
    case shape_oriented_ellipse:
    case shape_pie:
    case shape_oriented_box: return 2;
    case shape_arc:
    case shape_curve:
    case shape_triangle: return 3;
    default: return 0;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
static inline void primitive_set_points(struct primitive* p, uint32_t index, vec2 point) 
{
    assert(index < primitive_get_num_points(p->m_Shape)); 
    p->m_Points[index] = point;
}

//----------------------------------------------------------------------------------------------------------------------------
static inline vec2 primitive_get_points(struct primitive* p, uint32_t index) 
{
    assert(index < primitive_get_num_points(p->m_Shape)); 
    return p->m_Points[index];
}

//----------------------------------------------------------------------------------------------------------------------------
static inline void primitive_set_invalid(struct primitive* p)
{
    p->m_Shape = shape_invalid;
}

//----------------------------------------------------------------------------------------------------------------------------
static inline bool primitive_is_valid(struct primitive* p)
{
    return p->m_Shape != shape_invalid;
}

#endif