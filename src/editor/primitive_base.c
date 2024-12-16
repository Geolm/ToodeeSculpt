#include "primitive_base.h"
#include "../system/point_in.h"
#include "../system/arc.h"
#include "../system/palettes.h"
#include "../system/microui.h"
#include "../system/serializer.h"
#include "../renderer/crenderer.h"
#include "color_box.h"

#define point_radius (6.f)

static int g_SDFOperationComboBox = 0;

packed_color primitive_palette[16] = 
{
    na16_light_grey, na16_dark_grey, na16_dark_brown, na16_brown,
    na16_light_brown, na16_light_green, na16_green, na16_dark_green, 
    na16_orange, na16_red, na16_pink, na16_purple,
    na16_light_blue, na16_blue, na16_dark_blue, na16_black
};

//----------------------------------------------------------------------------------------------------------------------------
void primitive_init(struct primitive_base* p, enum command_type type, enum sdf_operator op, color4f color, float roundness, float width)
{
    p->m_Width = width;
    p->m_Roundness = roundness;
    p->m_Thickness = 0.f;
    p->m_Type = type;
    p->m_Filled = 1;
    p->m_Operator = op;
    p->m_Color = color;
    p->m_AABB = aabb_invalid();
}

//----------------------------------------------------------------------------------------------------------------------------
bool primitive_test_mouse_cursor(struct primitive_base const* p, vec2 mouse_position, bool test_vertices)
{
    bool result = false;

    switch(p->m_Type)
    {
    case primitive_triangle: 
        {
            result = point_in_triangle(p->m_Points[0], p->m_Points[1], p->m_Points[2], mouse_position);
            break;
        }
    case primitive_disc:
        {
            result = point_in_disc(p->m_Points[0], p->m_Roundness, mouse_position);
            break;
        }
    case primitive_ellipse:
        {
            result = point_in_ellipse(p->m_Points[0], p->m_Points[1], p->m_Width, mouse_position);
            break;
        }
    case primitive_oriented_box:
        {
            result = point_in_oriented_box(p->m_Points[0], p->m_Points[1], p->m_Width, mouse_position);
            break;
        }
    case primitive_pie:
        {
            vec2 direction = vec2_sub(p->m_Points[1], p->m_Points[0]);
            float radius = vec2_normalize(&direction);

            result = point_in_pie(p->m_Points[0], direction, radius, p->m_Aperture, mouse_position);
            break;
        }
    case primitive_ring:
        {
            vec2 center,direction;
            float aperture, radius;
            arc_from_points(p->m_Points[0], p->m_Points[1], p->m_Points[2], &center, &direction, &aperture, &radius);

            if (radius>0.f)
                result = point_in_circle(center, radius, p->m_Thickness, mouse_position) && point_in_pie(center, direction, radius, aperture, mouse_position);
            else
                result = false;
            break;
        }

    default: 
        return false;
    }

    if (test_vertices)
    {
        for(uint32_t i=0; i<primitive_get_num_points(p->m_Type); ++i)
            result |= point_in_disc(p->m_Points[i], point_radius, mouse_position);
    }

    return result;
}


//----------------------------------------------------------------------------------------------------------------------------
float primitive_distance_to_nearest_point(struct primitive_base const* p, vec2 reference)
{
    float min_distance = FLT_MAX;
    for(uint32_t i=0; i<primitive_get_num_points(p->m_Type); ++i)
    {
        float distance = vec2_sq_distance(p->m_Points[i], reference);
        if (distance < min_distance)
            min_distance = distance;
    }
    return min_distance;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_update_aabb(struct primitive_base* p)
{
    switch(p->m_Type)
    {
    case primitive_disc : p->m_AABB = aabb_from_circle(p->m_Points[0], p->m_Roundness);break;
    case primitive_ellipse : p->m_AABB = aabb_from_obb(p->m_Points[0], p->m_Points[1], p->m_Width); break;
    case primitive_triangle :
    {
        p->m_AABB = aabb_from_triangle(p->m_Points[0], p->m_Points[1], p->m_Points[2]);
        aabb_grow(&p->m_AABB, vec2_splat(p->m_Roundness));
        break;
    }
    case primitive_oriented_box : p->m_AABB = aabb_from_rounded_obb(p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Roundness); break;
    case primitive_pie :
    case primitive_ring :
    {
        float radius = vec2_length(vec2_sub(p->m_Points[1], p->m_Points[0]));
        p->m_AABB = aabb_from_circle(p->m_Points[0], radius);
        break;
    }
    default: p->m_AABB = aabb_invalid();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
int primitive_property_grid(struct primitive_base* p, struct mu_Context* gui_context)
{
    int res = 0;

    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context,"type");
    switch(p->m_Type)
    {
    case primitive_disc : 
        {
            mu_text(gui_context, "disc");
            mu_label(gui_context, "radius");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 1000.f, 1.f, "%3.0f", 0);
            break;
        }
    case primitive_ellipse :
        {
            mu_text(gui_context, "ellipse");
            mu_label(gui_context, "width");
            res |= mu_slider_ex(gui_context, &p->m_Width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            break;
        }
    case primitive_triangle : 
        {
            mu_text(gui_context, "triangle");
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    case primitive_oriented_box : 
        {
            mu_text(gui_context, "box");
            mu_label(gui_context, "width");
            res |= mu_slider_ex(gui_context, &p->m_Width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    case primitive_pie :
        {
            mu_text(gui_context, "pie");
            mu_label(gui_context, "aperture");
            res |= mu_slider_ex(gui_context, &p->m_Aperture, 0.f, VEC2_PI, 0.01f, "%3.2f", 0);
            break;
        }
    case primitive_ring :
        {
            mu_text(gui_context, "arc");
            break;
        }

    default : break;
    }

    if (p->m_Type != primitive_ring)
    {
        mu_label(gui_context, "filled");
        res |= mu_checkbox(gui_context, "filled", &p->m_Filled);
    }

    mu_label(gui_context, "thickness");
    res |= mu_slider_ex(gui_context, &p->m_Thickness, 0.f, 100.f, 0.1f, "%3.2f", 0);

    _Static_assert(sizeof(p->m_Operator) == sizeof(int), "operator");
    mu_label(gui_context, "operation");
    const char* op_names[op_last] = {"add", "blend", "sub", "overlap"};
    res |= mu_combo_box(gui_context, &g_SDFOperationComboBox, (int*)&p->m_Operator, op_last, op_names);

    struct color_box color = 
    {
        .rgba_output = &p->m_Color,
        .num_palette_entries = 16,
        .palette_entries = primitive_palette,
        .palette_entries_per_row = 8
    };

    res |= color_property_grid(gui_context, &color);
    return res;
}

//----------------------------------------------------------------------------------------------------------------------------
vec2 primitive_compute_center(struct primitive_base const* p)
{
    vec2 center = {.x = 0.f, .y = 0.f};
    uint32_t num_points = primitive_get_num_points(p->m_Type);
    for(uint32_t i=0; i<num_points; ++i)
        center = vec2_add(center, p->m_Points[i]);

    return vec2_scale(center, 1.f / (float)num_points);
}

//----------------------------------------------------------------------------------------------------------------------------
int primitive_contextual_property_grid(struct primitive_base* p, struct mu_Context* gui_context)
{
    int res = 0;

    if (p->m_Type != primitive_ring)
        res |= mu_checkbox(gui_context, "filled", &p->m_Filled);

    return res;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_deserialize(struct primitive_base* p, serializer_context* context, uint16_t major, uint16_t minor)
{
    if (major == 2)
    {
        if (minor == 0)
        {
            serializer_read_blob(context, p->m_Points, sizeof(vec2) * PRIMITIVE_MAXPOINTS);
            p->m_Width = serializer_read_float(context);
            p->m_Aperture = serializer_read_float(context);
            p->m_Roundness = serializer_read_float(context);
            p->m_Thickness = serializer_read_float(context);
            serializer_read_struct(context, p->m_Type);
            p->m_Filled = serializer_read_uint32_t(context);
            serializer_read_struct(context, p->m_Operator);
            serializer_read_struct(context, p->m_Color);
        }
        else if (minor == 1)
        {
            serializer_read_blob(context, p->m_Points, sizeof(vec2) * PRIMITIVE_MAXPOINTS);
            serializer_read_struct(context, p->m_Type);
            if (p->m_Type == primitive_ellipse || p->m_Type == primitive_oriented_box )
                p->m_Width = serializer_read_float(context);
            if (p->m_Type == primitive_pie || p->m_Type == primitive_ring)
                p->m_Aperture = serializer_read_float(context);
            if (p->m_Type != primitive_pie && p->m_Type != primitive_pie && p->m_Type != primitive_ring)
                p->m_Roundness = serializer_read_float(context);
            p->m_Thickness = serializer_read_float(context);
            p->m_Filled = serializer_read_uint8_t(context);
            serializer_read_struct(context, p->m_Operator);
            serializer_read_struct(context, p->m_Color);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_serialize(struct primitive_base const* p, serializer_context* context)
{
    serializer_write_blob(context, p->m_Points, sizeof(vec2) * PRIMITIVE_MAXPOINTS);
    serializer_write_struct(context, p->m_Type);
    if (p->m_Type == primitive_ellipse || p->m_Type == primitive_oriented_box )
        serializer_write_float(context, p->m_Width);
    if (p->m_Type == primitive_pie || p->m_Type == primitive_ring)
        serializer_write_float(context, p->m_Aperture);
    if (p->m_Type != primitive_pie && p->m_Type != primitive_pie && p->m_Type != primitive_ring)
        serializer_write_float(context, p->m_Roundness);

    serializer_write_float(context, p->m_Thickness);
    serializer_write_uint8_t(context, (uint8_t)p->m_Filled);
    serializer_write_struct(context, p->m_Operator);
    serializer_write_struct(context, p->m_Color);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_translate(struct primitive_base* p, vec2 translation)
{
    for(uint32_t i=0; i<primitive_get_num_points(p->m_Type); ++i)
        p->m_Points[i] = vec2_add(p->m_Points[i], translation);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_normalize(struct primitive_base* p, const aabb* box)
{
    for(uint32_t i=0; i<primitive_get_num_points(p->m_Type); ++i)
        p->m_Points[i] = aabb_get_uv(box, p->m_Points[i]);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_expand(struct primitive_base* p, const aabb* box)
{
    for(uint32_t i=0; i<primitive_get_num_points(p->m_Type); ++i)
        p->m_Points[i] = aabb_bilinear(box, p->m_Points[i]);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw(struct primitive_base* p, void* renderer, float roundness, draw_color color, enum sdf_operator op)
{
    switch(p->m_Type)
    {
    case primitive_triangle:
    {
        if (p->m_Filled) 
            renderer_drawtriangle_filled(renderer, p->m_Points[0], p->m_Points[1], p->m_Points[2], roundness, color, op);
        else
            renderer_drawtriangle(renderer, p->m_Points[0], p->m_Points[1], p->m_Points[2], p->m_Thickness, color, op);
        break;
    }

    case primitive_disc:
    {
        if (p->m_Filled)
            renderer_drawcircle_filled(renderer, p->m_Points[0], p->m_Roundness, color, op);
        else
            renderer_drawcircle(renderer, p->m_Points[0], p->m_Roundness,p-> m_Thickness, color, op);
        break;
    }

    case primitive_ellipse:
    {
        if (p->m_Filled)
            renderer_drawellipse_filled(renderer, p->m_Points[0], p->m_Points[1], p->m_Width, color, op);
        else
            renderer_drawellipse(renderer, p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Thickness, color, op);
        break;
    }

    case primitive_oriented_box:
    {
        if (p->m_Filled)
            renderer_draworientedbox_filled(renderer, p->m_Points[0], p->m_Points[1], p->m_Width, roundness, color, op);
        else
            renderer_draworientedbox(renderer, p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Thickness, color, op);
        break;
    }

    case primitive_pie:
    {
        if (p->m_Filled)
            renderer_drawpie_filled(renderer, p->m_Points[0], p->m_Points[1], p->m_Aperture, color, op);
        else
            renderer_drawpie(renderer, p->m_Points[0], p->m_Points[1], p->m_Aperture, p->m_Thickness, color, op);
        break;
    }

    case primitive_ring:
    {
        renderer_drawring_filled(renderer, p->m_Points[0], p->m_Points[1], p->m_Points[2], p->m_Thickness, color, op);
        break;
    }

    default: 
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw_gizmo(struct primitive_base* p, void* renderer, draw_color color)
{
    primitive_draw(p, renderer, 0.f, color, op_add);

    for(uint32_t i=0; i<primitive_get_num_points(p->m_Type); ++i)
        renderer_drawcircle_filled(renderer, p->m_Points[i], point_radius, (draw_color){.packed_data = 0x7f10e010}, op_union);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw_alpha(struct primitive_base* p, void* renderer, float alpha)
{
    draw_color color = draw_color_from_float(p->m_Color.red, p->m_Color.green, p->m_Color.blue, alpha);
    primitive_draw(p, renderer, p->m_Roundness, color, p->m_Operator);
}
