#include "../renderer/Renderer.h"
#include "../system/palettes.h"
#include "../system/point_in.h"
#include "../system/microui.h"
#include "Primitive.h"

int Primitive::m_SDFOperationComboBox = 0;

//----------------------------------------------------------------------------------------------------------------------------
Primitive::Primitive(command_type type, sdf_operator op, primitive_color color, float roundness, float width)
    : m_Type(type), m_Roundness(roundness), m_Operator(op), m_Color(color)
{
    m_Desc.width = width;
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::DrawGizmo(Renderer& renderer)
{
    Draw(renderer, 0.f, draw_color(na16_orange, 128), op_add);

    for(uint32_t i=0; i<GetNumPoints(); ++i)
        renderer.DrawCircleFilled(m_Desc.points[i], point_radius, draw_color(na16_black, 128));
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
    const vec2* points = m_Desc.points;
    switch(m_Type)
    {
    case command_type::primitive_triangle_filled: 
        renderer.DrawTriangleFilled(points[0], points[1], points[2], roundness, color, op);
        break;

    case command_type::primitive_circle_filled:
        renderer.DrawCircleFilled(points[0], m_Roundness, color, op);
        break;

    case command_type::primitive_ellipse:
        renderer.DrawEllipse(points[0], points[1], m_Desc.width, color, op);
        break;

    case command_type::primitive_oriented_box:
        renderer.DrawOrientedBox(points[0], points[1], m_Desc.width, roundness, color, op);
        break;

    default: 
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
bool Primitive::TestMouseCursor(vec2 mouse_position, bool test_vertices)
{
    const vec2* points = m_Desc.points;
    bool result = false;

    switch(m_Type)
    {
    case command_type::primitive_triangle_filled: 
        {
            result = point_in_triangle(points[0], points[1], points[2], mouse_position);
            break;
        }
    case command_type::primitive_circle_filled:
        {
            result = point_in_disc(points[0], m_Roundness, mouse_position);
            break;
        }
    case command_type::primitive_ellipse:
        {
            result = point_in_ellipse(points[0], points[1], m_Desc.width, mouse_position);
            break;
        }
    case command_type::primitive_oriented_box:
        {
            result = point_in_oriented_box(points[0], points[1], m_Desc.width, mouse_position);
            break;
        }

    default: 
        return false;
    }

    if (test_vertices)
    {
        for(uint32_t i=0; i<GetNumPoints(); ++i)
            result |= point_in_disc(points[i], point_radius, mouse_position);
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Translate(vec2 translation)
{
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        m_Desc.points[i] += translation;
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Normalize(const aabb* box)
{
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        m_Desc.points[i] = aabb_get_uv(box, m_Desc.points[i]);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Expand(const aabb* box)
{
    for(uint32_t i=0; i<GetNumPoints(); ++i)
        m_Desc.points[i] = aabb_bilinear(box, m_Desc.points[i]);
}

//----------------------------------------------------------------------------------------------------------------------------
int Primitive::PropertyGrid(struct mu_Context* gui_context)
{
    int res = 0;

    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context,"type");
    switch(m_Type)
    {
    case command_type::primitive_circle_filled : 
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
            res |= mu_slider_ex(gui_context, &m_Desc.width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            break;
        }
    case command_type::primitive_triangle_filled : 
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
            res |= mu_slider_ex(gui_context, &m_Desc.width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
            mu_label(gui_context, "roundness");
            res |= mu_slider_ex(gui_context, &m_Roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
            break;
        }

    default : break;
    }

    _Static_assert(sizeof(m_Operator) == sizeof(int));
    mu_label(gui_context, "operation");
    const char* op_names[op_last] = {"add", "blend", "sub", "overlap"};
    res |= mu_combo_box(gui_context, &m_SDFOperationComboBox, (int*)&m_Operator, op_last, op_names);
    res |= mu_rgb_color(gui_context, &m_Color.red, &m_Color.green, &m_Color.blue);

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

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::DumpInfo() const
{
    switch(m_Type)
    {
    case command_type::primitive_circle_filled : log_info("disc");break;
    case command_type::primitive_ellipse : log_info("ellipse");break;
    case command_type::primitive_triangle_filled : log_info("triangle"); break;
    case command_type::primitive_oriented_box : log_info("box"); break;
    default: log_info("unknown"); break;
    }
    const char* op_names[op_last] = {"add", "blend", "sub", "overlap"};
    log_info("operation : %s", op_names[m_Operator]);
}


