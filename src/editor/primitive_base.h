#ifndef __PRIMITIVE_BASE__H__
#define __PRIMITIVE_BASE__H__

#include "../system/aabb.h"
#include "../system/color.h"
#include "../shaders/common.h"
#include "../system/serializer.h"

enum {PRIMITIVE_MAXPOINTS = 3};

struct primitive_base
{
    aabb m_AABB;
    vec2 m_Points[PRIMITIVE_MAXPOINTS];
    float m_Width; 
    float m_Aperture;
    float m_Roundness;
    float m_Thickness;
    enum command_type m_Type;
    int m_Filled;
    enum sdf_operator m_Operator;
    color4f m_Color;
};

extern packed_color primitive_palette[16];

#ifdef __cplusplus
extern "C" {
#endif

struct mu_Context;

void primitive_init(struct primitive_base* p, enum command_type type, enum sdf_operator op, color4f color, float roundness, float width);
bool primitive_test_mouse_cursor(struct primitive_base const* primitive, vec2 mouse_position, bool test_vertices);
float primitive_distance_to_nearest_point(struct primitive_base const* primitive, vec2 reference);
void primitive_update_aabb(struct primitive_base* primitive);
int primitive_property_grid(struct primitive_base* primitive, struct mu_Context* gui_context);
vec2 primitive_compute_center(struct primitive_base const* primitive);
int primitive_contextual_property_grid(struct primitive_base* primitive, struct mu_Context* gui_context);
void primitive_deserialize(struct primitive_base* primitive, serializer_context* context, uint16_t major, uint16_t minor);
void primitive_serialize(struct primitive_base const* primitive, serializer_context* context);
void primitive_translate(struct primitive_base* p, vec2 translation);
void primitive_normalize(struct primitive_base* p, const aabb* box);
void primitive_expand(struct primitive_base* p, const aabb* box);
void primitive_draw(struct primitive_base* p, void* renderer, float roundness, draw_color color, enum sdf_operator op);
void primitive_draw_gizmo(struct primitive_base* p, void* renderer, draw_color color);
void primitive_draw_alpha(struct primitive_base* p, void* renderer, float alpha);


#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------------------------------------------------------
static inline uint32_t primitive_get_num_points(enum command_type type)
{
    switch(type)
    {
    case primitive_disc: return 1;
    case primitive_ellipse:
    case primitive_pie:
    case primitive_oriented_box: return 2;
    case primitive_ring:
    case primitive_triangle: return 3;
    default: return 0;
    }
}

#endif