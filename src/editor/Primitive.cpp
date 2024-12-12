#include "../renderer/Renderer.h"
#include "../system/palettes.h"
#include "../system/point_in.h"
#include "../system/microui.h"
#include "color_box.h"
#include "Primitive.h"

int Primitive::m_SDFOperationComboBox = 0;

packed_color Primitive::m_Palette[16] = 
{
    na16_light_grey, na16_dark_grey, na16_dark_brown, na16_brown,
    na16_light_brown, na16_light_green, na16_green, na16_dark_green, 
    na16_orange, na16_red, na16_pink, na16_purple,
    na16_light_blue, na16_blue, na16_dark_blue, na16_black
};

//----------------------------------------------------------------------------------------------------------------------------
Primitive::Primitive(command_type type, sdf_operator op, color4f color, float roundness, float width)
    : m_Width(width), m_Roundness(roundness), m_Thickness(0.f), m_Type(type), m_Filled(1), m_Operator(op), m_Color(color)
{
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::DrawGizmo(Renderer& renderer, draw_color color)
{
    Draw(renderer, 0.f, color, op_add);

    for(uint32_t i=0; i<GetNumPoints(); ++i)
        renderer.DrawCircleFilled(m_Points[i], point_radius, draw_color(0x10e010, 128));
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Draw(Renderer& renderer, float alpha)
{
    draw_color color;
    color.from_float(m_Color.red, m_Color.green, m_Color.blue, alpha);
    Draw(renderer, m_Roundness, color, m_Operator);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Draw(Renderer& renderer, float roundness, draw_color color, sdf_operator op)
{
    switch(m_Type)
    {
    case command_type::primitive_triangle:
    {
        if (m_Filled) 
            renderer.DrawTriangleFilled(m_Points[0], m_Points[1], m_Points[2], roundness, color, op);
        else
            renderer.DrawTriangle(m_Points[0], m_Points[1], m_Points[2], m_Thickness, color, op);
        break;
    }

    case command_type::primitive_disc:
    {
        if (m_Filled)
            renderer.DrawCircleFilled(m_Points[0], m_Roundness, color, op);
        else
            renderer.DrawCircle(m_Points[0], m_Roundness, m_Thickness, color, op);
        break;
    }

    case command_type::primitive_ellipse:
    {
        if (m_Filled)
            renderer.DrawEllipseFilled(m_Points[0], m_Points[1], m_Width, color, op);
        else
            renderer.DrawEllipse(m_Points[0], m_Points[1], m_Width, m_Thickness, color, op);
        break;
    }

    case command_type::primitive_oriented_box:
    {
        if (m_Filled)
            renderer.DrawOrientedBoxFilled(m_Points[0], m_Points[1], m_Width, roundness, color, op);
        else
            renderer.DrawOrientedBox(m_Points[0], m_Points[1], m_Width, m_Thickness, color, op);
        break;
    }

    case command_type::primitive_pie:
    {
        if (m_Filled)
            renderer.DrawPieFilled(m_Points[0], m_Points[1], m_Aperture, color, op);
        else
            renderer.DrawPie(m_Points[0], m_Points[1], m_Aperture, m_Thickness, color, op);
        break;
    }

    case command_type::primitive_ring:
    {
        renderer.DrawRingFilled(m_Points[0], m_Points[1], m_Points[2], m_Thickness, color, op);
        break;
    }

    default: 
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
bool Primitive::TestMouseCursor(vec2 mouse_position, bool test_vertices)
{
    bool result = false;

    switch(m_Type)
    {
    case command_type::primitive_triangle: 
        {
            result = point_in_triangle(m_Points[0], m_Points[1], m_Points[2], mouse_position);
            break;
        }
    case command_type::primitive_disc:
        {
            result = point_in_disc(m_Points[0], m_Roundness, mouse_position);
            break;
        }
    case command_type::primitive_ellipse:
        {
            result = point_in_ellipse(m_Points[0], m_Points[1], m_Width, mouse_position);
            break;
        }
    case command_type::primitive_oriented_box:
        {
            result = point_in_oriented_box(m_Points[0], m_Points[1], m_Width, mouse_position);
            break;
        }
    case command_type::primitive_pie:
        {
            result = point_in_pie(m_Points[0], m_Points[1], m_Aperture, mouse_position);
            break;
        }

    default: 
        return false;
    }

    if (test_vertices)
    {
        for(uint32_t i=0; i<GetNumPoints(); ++i)
            result |= point_in_disc(m_Points[i], point_radius, mouse_position);
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Translate(vec2 translation)
{
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        m_Points[i] += translation;
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Normalize(const aabb* box)
{
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        m_Points[i] = aabb_get_uv(box, m_Points[i]);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Expand(const aabb* box)
{
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        m_Points[i] = aabb_bilinear(box, m_Points[i]);
}

//----------------------------------------------------------------------------------------------------------------------------
int Primitive::PropertyGrid(struct mu_Context* gui_context)
{
    int res = 0;

    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context,"type");
    switch(m_Type)
    {
    case command_type::primitive_disc : 
        {
            mu_text(gui_context, "disc");
            mu_label(gui_context, "radius");
            res |= mu_slider_ex(gui_context, &m_Roundness, 0.f, 1000.f, 1.f, "%3.0f", 0);
            break;
        }
    case command_type::primitive_ellipse :
        {
            mu_text(gui_context, "ellipse");
            mu_label(gui_context, "width");
            res |= mu_slider_ex(gui_context, &m_Width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            break;
        }
    case command_type::primitive_triangle : 
        {
            mu_text(gui_context, "triangle");
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    case command_type::primitive_oriented_box : 
        {
            mu_text(gui_context, "box");
            mu_label(gui_context, "width");
            res |= mu_slider_ex(gui_context, &m_Width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }
    case command_type::primitive_pie :
        {
            mu_text(gui_context, "pie");
            mu_label(gui_context, "aperture");
            res |= mu_slider_ex(gui_context, &m_Aperture, 0.f, VEC2_PI, 0.01f, "%3.2f", 0);
            break;
        }
    case command_type::primitive_ring :
        {
            mu_text(gui_context, "pie");
            break;
        }

    default : break;
    }

    if (m_Type != command_type::primitive_ring)
    {
        mu_label(gui_context, "filled");
        res |= mu_checkbox(gui_context, "filled", &m_Filled);
    }

    mu_label(gui_context, "thickness");
    res |= mu_slider_ex(gui_context, &m_Thickness, 0.f, 100.f, 0.1f, "%3.2f", 0);

    _Static_assert(sizeof(m_Operator) == sizeof(int));
    mu_label(gui_context, "operation");
    const char* op_names[op_last] = {"add", "blend", "sub", "overlap"};
    res |= mu_combo_box(gui_context, &m_SDFOperationComboBox, (int*)&m_Operator, op_last, op_names);

    struct color_box color = 
    {
        .rgba_output = &m_Color,
        .num_palette_entries = 16,
        .palette_entries = m_Palette,
        .palette_entries_per_row = 8
    };

    res |= color_property_grid(gui_context, &color);

    return res;
}

//----------------------------------------------------------------------------------------------------------------------------
int Primitive::ContextualPropertyGrid(struct mu_Context* gui_context)
{
    int res = 0;

    if (m_Type != command_type::primitive_ring)
        res |= mu_checkbox(gui_context, "filled", &m_Filled);

    return res;
}

//----------------------------------------------------------------------------------------------------------------------------
vec2 Primitive::ComputerCenter() const
{
    vec2 center = {.x = 0.f, .y = 0.f};
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        center += GetPoints(i);

    return vec2_scale(center, 1.f / float(GetNumPoints()));
}
