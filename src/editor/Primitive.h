#pragma once

#include "../system/vec2.h"
#include "../shaders/common.h"
#include <assert.h>

enum {PRIMITIVE_MAXPOINTS = 3};

class Renderer;
struct mu_Context;

struct primitive_color
{
    float red, green, blue;
};

//----------------------------------------------------------------------------------------------------------------------------
class Primitive
{
public:
    Primitive() {SetInvalid();}
    Primitive(command_type type, sdf_operator op, primitive_color color, float roundness, float radius);
    
    void DrawGizmo(Renderer& renderer);
    void Draw(Renderer& renderer, float alpha);
    void Draw(Renderer& renderer, float roundness, draw_color color, sdf_operator op);
    bool TestMouseCursor(vec2 mouse_position, bool test_vertices);
    void Translate(vec2 translation);
    void Normalize(const aabb* box);
    void Expand(const aabb* box);
    int PropertyGrid(struct mu_Context* gui_context);

    void SetInvalid() {m_Type = combination_begin;}
    bool IsValid() {return m_Type != combination_begin;}
    vec2& GetPoints(uint32_t index) {assert(index < GetNumPoints()); return m_Points[index];}
    const vec2& GetPoints(uint32_t index) const {assert(index < GetNumPoints()); return m_Points[index];}
    void SetPoints(uint32_t index, vec2 point) {assert(index < GetNumPoints()); m_Points[index] = point;}
    inline command_type GetType() const {return m_Type;}
    static inline uint32_t GetNumPoints(command_type type);
    uint32_t GetNumPoints() const {return GetNumPoints(m_Type);}
    vec2 ComputerCenter() const;

public:
    static constexpr const float point_radius {6.f};

private:
    vec2 m_Points[PRIMITIVE_MAXPOINTS];
    float m_Width;
    float m_Roundness;
    float m_Thickness;
    command_type m_Type;
    int m_Filled;
    sdf_operator m_Operator;
    primitive_color m_Color;

    static int m_SDFOperationComboBox;
};

//----------------------------------------------------------------------------------------------------------------------------
inline uint32_t Primitive::GetNumPoints(command_type type)
{
    switch(type)
    {
    case primitive_disc: return 1;
    case primitive_ellipse:
    case primitive_oriented_box: return 2;
    case primitive_pie:
    case primitive_triangle: return 3;
    default: return 0;
    }
}

