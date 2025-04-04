#include "primitive.h"
#include "../system/point_in.h"
#include "../system/arc.h"
#include "../system/palettes.h"
#include "../system/microui.h"
#include "../system/serializer.h"
#include "../system/log.h"
#include "../renderer/renderer.h"
#include "../system/format.h"
#include "../system/biarc.h"
#include "color_box.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static int g_SDFOperationComboBox = 0;
static int g_SDFFillmodeComboBox = 0;
static const char* g_sdf_op_names[op_last] = {"add", "blend", "sub", "overlap"};
static const char* g_sdf_fillmode_names[fill_last] = {"solid", "outline", "hollow"};
static draw_color point_color = (draw_color){.packed_data = 0x7f10e010};
static draw_color radius_color = (draw_color){.packed_data = 0x7fe01010};

#define UNUSED_VARIABLE(a) (void)(a)

//----------------------------------------------------------------------------------------------------------------------------
void primitive_init(struct primitive* p, enum primitive_shape shape, enum sdf_operator op, color4f color, float roundness, float width)
{
    p->m_Width = width;
    p->m_Roundness = roundness;
    p->m_Thickness = 0.f;
    p->m_Shape = shape;
    p->m_Fillmode = fill_solid;
    p->m_Operator = op;
    p->m_Color = color;
    p->m_AABB = aabb_invalid();
    p->m_NumArcs = 0;
    p->m_Editmode = edit_nothing;
}

//----------------------------------------------------------------------------------------------------------------------------
bool primitive_test_mouse_cursor(struct primitive const* p, vec2 mouse_position, bool test_vertices)
{
    bool result = false;

    if (aabb_test_point(&p->m_AABB, mouse_position))
    {
        switch(p->m_Shape)
        {
        case shape_triangle: 
            {
                result = point_in_triangle(p->m_Points[0], p->m_Points[1], p->m_Points[2], mouse_position);
                break;
            }
        case shape_disc:
            {
                result = point_in_disc(p->m_Points[0], p->m_Roundness, mouse_position);
                break;
            }
        case shape_oriented_ellipse:
            {
                result = point_in_ellipse(p->m_Points[0], p->m_Points[1], p->m_Width, mouse_position);
                break;
            }
        case shape_oriented_box:
            {
                result = point_in_oriented_box(p->m_Points[0], p->m_Points[1], p->m_Width, mouse_position);
                break;
            }
        case shape_pie:
            {
                result = point_in_pie(p->m_Points[0], p->m_Direction, p->m_Radius, p->m_Aperture, mouse_position);
                break;
            }
        case shape_arc:
            {
                if (p->m_Radius>0.f)
                {
                    float half_thickness = p->m_Thickness * .5f;
                    result = !point_in_disc(p->m_Center, p->m_Radius - half_thickness , mouse_position);
                    result &= point_in_pie(p->m_Center, p->m_Direction, p->m_Radius + half_thickness, p->m_Aperture, mouse_position);
                }
                else
                    result = false;
                break;
            }
        case shape_spline : return true;
        case shape_uneven_capsule :
            {
                result = point_in_uneven_capsule(p->m_Points[0], p->m_Points[1], p->m_Roundness, p->m_Radius, mouse_position);
                break;
            }
        case shape_trapezoid:
            {
                result = point_in_trapezoid(p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Radius, mouse_position);
                break;
            }

        default: 
            return false;
        }
    }

    if (test_vertices)
    {
        for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
            result |= point_in_disc(p->m_Points[i], primitive_point_radius, mouse_position);
    }

    return result;
}


//----------------------------------------------------------------------------------------------------------------------------
float primitive_distance_to_nearest_point(struct primitive const* p, vec2 reference)
{
    float min_distance = FLT_MAX;
    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
    {
        float distance = vec2_sq_distance(p->m_Points[i], reference);
        if (distance < min_distance)
            min_distance = distance;
    }
    return min_distance;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_update_aabb(struct primitive* p)
{
    switch(p->m_Shape)
    {
    case shape_disc : p->m_AABB = aabb_from_circle(p->m_Points[0], p->m_Roundness);break;
    case shape_oriented_ellipse : p->m_AABB = aabb_from_obb(p->m_Points[0], p->m_Points[1], p->m_Width); break;
    case shape_triangle :
    {
        p->m_AABB = aabb_from_triangle(p->m_Points[0], p->m_Points[1], p->m_Points[2]);
        aabb_grow(&p->m_AABB, p->m_Fillmode == fill_hollow ? vec2_splat(p->m_Thickness * .5f) : vec2_splat(p->m_Roundness));
        break;
    }
    case shape_oriented_box : p->m_AABB = aabb_from_rounded_obb(p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Roundness); break;
    case shape_pie :
    {
        p->m_Direction = vec2_sub(p->m_Points[1], p->m_Points[0]);
        p->m_Radius = vec2_normalize(&p->m_Direction);
        p->m_AABB = aabb_from_pie(p->m_Points[0], p->m_Direction, p->m_Radius, p->m_Aperture);
        break;
    }
    case shape_arc :
    {
        arc_from_points(p->m_Points[0], p->m_Points[1], p->m_Points[2], &p->m_Center, &p->m_Direction, &p->m_Aperture, &p->m_Radius);
        p->m_AABB = aabb_from_arc(p->m_Center, p->m_Direction, p->m_Radius, p->m_Aperture);
        aabb_grow(&p->m_AABB, vec2_splat(p->m_Thickness * .5f));
        break;
    }
    case shape_spline :
    {
        p->m_NumArcs = biarc_spline(p->m_Points, primitive_get_num_points(p->m_Shape), p->m_Arcs);
        p->m_AABB = aabb_invalid();

        for(uint32_t i=0; i<p->m_NumArcs; ++i)
            p->m_AABB = aabb_merge(p->m_AABB, aabb_from_arc(p->m_Arcs[i].center, p->m_Arcs[i].direction, p->m_Arcs[i].radius, p->m_Arcs[i].aperture));

        aabb_grow(&p->m_AABB, vec2_splat(p->m_Thickness * .5f));
        break;
    }
    case shape_uneven_capsule :
    {
        p->m_AABB = aabb_from_capsule(p->m_Points[0], p->m_Points[1], float_max(p->m_Roundness, p->m_Radius));
        break;
    }
    case shape_trapezoid :
    {
        p->m_AABB = aabb_from_trapezoid(p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Radius);
        aabb_grow(&p->m_AABB, p->m_Fillmode == fill_hollow ? vec2_splat(p->m_Thickness * .5f) : vec2_splat(p->m_Roundness));
        break;
    }
    default: p->m_AABB = aabb_invalid();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
int primitive_property_grid(struct primitive* p, struct mu_Context* gui_context)
{
    int res = 0;

    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context,"shape");
    switch(p->m_Shape)
    {
    case shape_disc : 
        {
            mu_text(gui_context, "disc");
            mu_label(gui_context, "radius");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 1000.f, 1.f, "%3.0f", 0);
            break;
        }
    case shape_oriented_ellipse :
        {
            mu_text(gui_context, "ellipse");
            mu_label(gui_context, "width");
            res |= mu_slider_ex(gui_context, &p->m_Width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            break;
        }
    case shape_triangle : 
        {
            mu_text(gui_context, "triangle");
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    case shape_oriented_box : 
        {
            mu_text(gui_context, "box");
            mu_label(gui_context, "width");
            res |= mu_slider_ex(gui_context, &p->m_Width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    case shape_pie :
        {
            mu_text(gui_context, "pie");
            mu_label(gui_context, "aperture");
            res |= mu_slider_ex(gui_context, &p->m_Aperture, 0.f, VEC2_PI, 0.01f, "%3.2f", 0);
            break;
        }
    case shape_arc :
        {
            mu_text(gui_context, "arc");
            break;
        }
    case shape_spline:
        {
            mu_text(gui_context, "spline");
            mu_label(gui_context, "num arcs");
            mu_text(gui_context, format("%d", p->m_NumArcs));
            break;
        }
    case shape_uneven_capsule:
        {
            mu_text(gui_context, "uneven capsule");
            mu_label(gui_context, "radius 1");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 400.f, 1.f, "%3.0f", 0);
            mu_label(gui_context, "radius 2");
            res |= mu_slider_ex(gui_context, &p->m_Radius, 0.f, 400.f, 1.f, "%3.0f", 0);
            break;
        }
    case shape_trapezoid:
        {
            mu_text(gui_context, "trapezoid");
            mu_label(gui_context, "width 1");
            res |= mu_slider_ex(gui_context, &p->m_Width, 0.f, 400.f, 1.f, "%3.0f", 0);
            mu_label(gui_context, "width 2");
            res |= mu_slider_ex(gui_context, &p->m_Radius, 0.f, 400.f, 1.f, "%3.0f", 0);
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &p->m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    default : mu_text(gui_context, "unknown");break;
    }

    _Static_assert(sizeof(p->m_Fillmode) == sizeof(int), "fillmode enum must have a int size");
    mu_label(gui_context, "fillmode");
    res |= mu_combo_box(gui_context, &g_SDFFillmodeComboBox, (int*)&p->m_Fillmode, fill_last, g_sdf_fillmode_names);

    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context, "thickness");
    res |= mu_slider_ex(gui_context, &p->m_Thickness, 0.f, primitive_max_thickness, 0.1f, "%3.2f", 0);

    if (p->m_Shape != shape_spline)
    {
        _Static_assert(sizeof(p->m_Operator) == sizeof(int), "operator enum must have a int size");
        mu_label(gui_context, "operation");
        res |= mu_combo_box(gui_context, &g_SDFOperationComboBox, (int*)&p->m_Operator, op_last, g_sdf_op_names);
    }

    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);

    struct color_box color = 
    {
        .rgba_output = &p->m_Color,
        .palette = &primitive_palette,
        .palette_entries_per_row = 8
    };

    res |= color_property_grid(gui_context, &color);
    return res;
}

//----------------------------------------------------------------------------------------------------------------------------
vec2 primitive_compute_center(struct primitive const* p)
{
    vec2 center = {.x = 0.f, .y = 0.f};
    uint32_t num_points = primitive_get_num_points(p->m_Shape);
    for(uint32_t i=0; i<num_points; ++i)
        center = vec2_add(center, p->m_Points[i]);

    return vec2_scale(center, 1.f / (float)num_points);
}

//----------------------------------------------------------------------------------------------------------------------------
int primitive_contextual_property_grid(struct primitive* p, struct mu_Context* gui_context)
{
    int fillmode = (int)p->m_Fillmode;
    int res = mu_combo_button(gui_context, "fill", fill_last, g_sdf_fillmode_names, &fillmode);
    p->m_Fillmode = (enum primitive_fillmode) fillmode;

    if (p->m_Shape != shape_spline)
    {
        int operator = (int)p->m_Operator;
        res |= mu_combo_button(gui_context, "op", op_last, g_sdf_op_names, &operator);
        p->m_Operator = (enum sdf_operator) operator;
    }
    return res;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_deserialize(struct primitive* p, serializer_context* context, uint16_t major, uint16_t minor)
{
    if (major == 2)
    {
        if (minor == 0)
        {
            serializer_read_blob(context, p->m_Points, sizeof(vec2) * 3);
            p->m_Width = serializer_read_float(context);
            p->m_Aperture = serializer_read_float(context);
            p->m_Roundness = serializer_read_float(context);
            p->m_Thickness = serializer_read_float(context);
            serializer_read_struct(context, p->m_Shape);
            bool filled = serializer_read_uint8_t(context);
            p->m_Fillmode = filled ? fill_solid : fill_solid;
            serializer_read_struct(context, p->m_Operator);
            serializer_read_struct(context, p->m_Color);
        }
        else if (minor >= 1)
        {
            if (minor<3)
            {
                serializer_read_blob(context, p->m_Points, sizeof(vec2) * 3);
                serializer_read_struct(context, p->m_Shape);
            }
            else
            {
                serializer_read_struct(context, p->m_Shape);
                serializer_read_blob(context, p->m_Points, sizeof(vec2) * primitive_get_num_points(p->m_Shape));
            }

            if (p->m_Shape == shape_oriented_ellipse || p->m_Shape == shape_oriented_box || p->m_Shape == shape_trapezoid)
                p->m_Width = serializer_read_float(context);
            if (p->m_Shape == shape_pie || p->m_Shape == shape_arc)
                p->m_Aperture = serializer_read_float(context);
            if (p->m_Shape != shape_pie && p->m_Shape != shape_arc)
                p->m_Roundness = serializer_read_float(context);
            if (p->m_Shape == shape_uneven_capsule || p->m_Shape == shape_trapezoid)
                p->m_Radius = serializer_read_float(context);

            p->m_Thickness = serializer_read_float(context);

            if (minor < 4)
            {
                bool filled = serializer_read_uint8_t(context);
                p->m_Fillmode = filled ? fill_solid : fill_solid;
                serializer_read_struct(context, p->m_Operator);
                serializer_read_struct(context, p->m_Color);
            }
            else
            {
                serializer_read_struct(context, p->m_Fillmode);
                serializer_read_struct(context, p->m_Operator);
                serializer_read_struct(context, p->m_Color);
            }
            p->m_NumArcs = (p->m_Shape == shape_spline) ? 6 : 0;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_serialize(struct primitive const* p, serializer_context* context)
{
    serializer_write_struct(context, p->m_Shape);
    serializer_write_blob(context, p->m_Points, sizeof(vec2) * primitive_get_num_points(p->m_Shape));

    if (p->m_Shape == shape_oriented_ellipse || p->m_Shape == shape_oriented_box || p->m_Shape == shape_trapezoid)
        serializer_write_float(context, p->m_Width);
    if (p->m_Shape == shape_pie || p->m_Shape == shape_arc)
        serializer_write_float(context, p->m_Aperture);
    if (p->m_Shape != shape_pie && p->m_Shape != shape_arc)
        serializer_write_float(context, p->m_Roundness);
    if (p->m_Shape == shape_uneven_capsule || p->m_Shape == shape_trapezoid)
        serializer_write_float(context, p->m_Radius);

    serializer_write_float(context, p->m_Thickness);
    serializer_write_struct(context, p->m_Fillmode);
    serializer_write_struct(context, p->m_Operator);
    serializer_write_struct(context, p->m_Color);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_translate(struct primitive* p, vec2 translation)
{
    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
        p->m_Points[i] = vec2_add(p->m_Points[i], translation);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_rotate(struct primitive* p, float angle)
{
    vec2 center = primitive_compute_center(p);
    vec2 rotation = vec2_angle(angle);

    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
    {
        p->m_Points[i] = vec2_sub(p->m_Points[i], center);
        p->m_Points[i] = vec2_rotate(p->m_Points[i], rotation);
        p->m_Points[i] = vec2_add(p->m_Points[i], center);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_scale(struct primitive* p, float scale)
{
    if (scale < 1.e-3f)
        return;

    vec2 center = primitive_compute_center(p);

    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
    {
        vec2 to_point = vec2_sub(p->m_Points[i], center);
        p->m_Points[i] = vec2_add(vec2_scale(to_point, scale), center);
    }

    p->m_Width *= scale;

    if (p->m_Shape == shape_disc)
        p->m_Roundness *= scale;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_normalize(struct primitive* p, const aabb* box)
{
    vec2 box_size = aabb_get_size(box);
    float normalization_factor = 1.f / float_min(box_size.x, box_size.y);

    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
        p->m_Points[i] = aabb_get_uv(box, p->m_Points[i]);

    for(uint32_t i=0; i<p->m_NumArcs; ++i)
    {
        p->m_Arcs[i].center = aabb_get_uv(box, p->m_Arcs[i].center);
        p->m_Arcs[i].radius *= normalization_factor; 
    }

    p->m_Center = aabb_get_uv(box, p->m_Center);
    p->m_Roundness *= normalization_factor;
    p->m_Thickness *= normalization_factor;
    p->m_Width *= normalization_factor;
    p->m_Radius *= normalization_factor;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_expand(struct primitive* p, const aabb* box)
{
    vec2 box_size = aabb_get_size(box);
    float expand_factor = float_min(box_size.x, box_size.y);

    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
        p->m_Points[i] = aabb_bilinear(box, p->m_Points[i]);

    for(uint32_t i=0; i<p->m_NumArcs; ++i)
    {
        p->m_Arcs[i].center = aabb_bilinear(box,  p->m_Arcs[i].center);
        p->m_Arcs[i].radius *= expand_factor; 
    }

    p->m_Center = aabb_bilinear(box, p->m_Center);
    p->m_Roundness *= expand_factor;
    p->m_Thickness *= expand_factor;
    p->m_Width *= expand_factor;
    p->m_Radius *= expand_factor;
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw(struct primitive* p, struct renderer* gfx_context, float roundness, draw_color color, enum sdf_operator op)
{
    switch(p->m_Shape)
    {
    case shape_triangle:
    {
        renderer_draw_triangle(gfx_context, p->m_Points[0], p->m_Points[1], p->m_Points[2], roundness, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_disc:
    {
        renderer_draw_disc(gfx_context, p->m_Points[0], p->m_Roundness, p-> m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_oriented_ellipse:
    {
        renderer_draw_ellipse(gfx_context, p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_oriented_box:
    {
        renderer_draw_orientedbox(gfx_context, p->m_Points[0], p->m_Points[1], p->m_Width, roundness, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_pie:
    {
        renderer_draw_pie(gfx_context, p->m_Points[0], p->m_Points[1], p->m_Aperture, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_arc:
    {
        renderer_draw_arc(gfx_context, p->m_Center, p->m_Direction, p->m_Aperture, p->m_Radius, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_spline:
    {
        for(uint32_t i=0; i<p->m_NumArcs; ++i)
        {
            if (p->m_Arcs[i].radius > 0.f)
                renderer_draw_arc(gfx_context, p->m_Arcs[i].center, p->m_Arcs[i].direction, p->m_Arcs[i].aperture, p->m_Arcs[i].radius, p->m_Thickness, p->m_Fillmode, color, op_add);
            else
                renderer_draw_line(gfx_context, p->m_Arcs[i].center, p->m_Arcs[i].direction, p->m_Thickness, color, op_add);
        }
        break;
    }

    case shape_uneven_capsule:
    {
        renderer_draw_unevencapsule(gfx_context, p->m_Points[0], p->m_Points[1], p->m_Roundness, p->m_Radius, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    case shape_trapezoid:
    {
        renderer_draw_trapezoid(gfx_context, p->m_Points[0], p->m_Points[1], p->m_Width, p->m_Radius, roundness, p->m_Thickness, p->m_Fillmode, color, op);
        break;
    }

    default: 
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw_aabb(struct primitive* p, struct renderer* gfx_context, draw_color color)
{
    renderer_draw_aabb(gfx_context, p->m_AABB, color);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw_selected(struct primitive* p, struct renderer* gfx_context, draw_color color)
{
    renderer_begin_combination(gfx_context, 1.f);
    primitive_draw(p, gfx_context, 0.f, color, op_add);
    renderer_end_combination(gfx_context, false);

    for(uint32_t i=0; i<primitive_get_num_points(p->m_Shape); ++i)
        renderer_draw_disc(gfx_context, p->m_Points[i], primitive_point_radius, -1.f, fill_solid, point_color, op_add);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw_alpha(struct primitive* p, struct renderer* gfx_context, float alpha)
{
    draw_color color = draw_color_from_float(p->m_Color.red, p->m_Color.green, p->m_Color.blue, alpha);
    primitive_draw(p, gfx_context, p->m_Roundness, color, p->m_Operator);
}

//----------------------------------------------------------------------------------------------------------------------------
void primitive_draw_spline(struct renderer* gfx_context, const vec2* points, uint32_t num_points, float thickness, draw_color color)
{
    struct arc arcs[primitive_max_arcs];
    uint32_t num_arcs;
    num_arcs = biarc_spline(points, num_points, arcs);

    renderer_begin_combination(gfx_context, 1.f);
    for(uint32_t i=0; i<num_arcs; ++i)
        if (arcs[i].radius>0.f)
            renderer_draw_arc(gfx_context, arcs[i].center, arcs[i].direction, arcs[i].aperture, arcs[i].radius, thickness, fill_solid,color, op_add);
        else
            renderer_draw_line(gfx_context, arcs[i].center, arcs[i].direction, thickness, color, op_add);

    renderer_end_combination(gfx_context, false);
}

