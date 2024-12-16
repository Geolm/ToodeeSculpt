#include "../renderer/Renderer.h"
#include "../system/palettes.h"
#include "../system/point_in.h"
#include "../system/microui.h"
#include "../system/arc.h"
#include "color_box.h"
#include "Primitive.h"

int Primitive::m_SDFOperationComboBox = 0;

//----------------------------------------------------------------------------------------------------------------------------
Primitive::Primitive(command_type type, sdf_operator op, color4f color, float roundness, float width)
{
    m_Width = width;
    m_Roundness = roundness;
    m_Thickness = 0.f;
    m_Type = type;
    m_Filled = 1;
    m_Operator = op;
    m_Color = color;
    m_AABB = aabb_invalid();
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
    primitive_draw(this, &renderer, m_Roundness, color, m_Operator);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Draw(Renderer& renderer, float roundness, draw_color color, sdf_operator op)
{
    primitive_draw(this, &renderer, roundness, color, op);
}

//----------------------------------------------------------------------------------------------------------------------------
bool Primitive::TestMouseCursor(vec2 mouse_position, bool test_vertices)
{
    return primitive_test_mouse_cursor(this, mouse_position, test_vertices);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Translate(vec2 translation)
{
    primitive_translate(this, translation);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Normalize(const aabb* box)
{
    primitive_normalize(this, box);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Expand(const aabb* box)
{
    primitive_expand(this, box);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Serialize(serializer_context* context)
{
    primitive_serialize(this, context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::Deserialize(serializer_context* context, uint16_t major, uint16_t minor)
{
    primitive_deserialize(this, context, major, minor);
}

//----------------------------------------------------------------------------------------------------------------------------
int Primitive::PropertyGrid(struct mu_Context* gui_context)
{
    return primitive_property_grid(this, gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
int Primitive::ContextualPropertyGrid(struct mu_Context* gui_context)
{
    return primitive_contextual_property_grid(this, gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
vec2 Primitive::ComputerCenter() const
{
    return primitive_compute_center(this);
}

//----------------------------------------------------------------------------------------------------------------------------
void Primitive::UpdateAABB()
{
    primitive_update_aabb(this);
}

//----------------------------------------------------------------------------------------------------------------------------
float Primitive::DistanceToNearestPoint(vec2 reference) const
{
    return primitive_distance_to_nearest_point(this, reference);
}