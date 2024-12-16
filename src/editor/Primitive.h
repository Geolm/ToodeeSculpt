#pragma once


#include "../system/serializer.h"
#include "primitive_base.h"

#include <assert.h>

class Renderer;
struct mu_Context;

//----------------------------------------------------------------------------------------------------------------------------
class Primitive : private primitive_base
{
public:
    Primitive() {SetInvalid();}
    Primitive(command_type type, sdf_operator op, color4f color, float roundness, float width);
    
    void DrawGizmo(Renderer& renderer, draw_color color);
    void Draw(Renderer& renderer, float alpha);
    void Draw(Renderer& renderer, float roundness, draw_color color, sdf_operator op);
    bool TestMouseCursor(vec2 mouse_position, bool test_vertices);
    void Translate(vec2 translation);
    void Normalize(const aabb* box);
    void Expand(const aabb* box);
    int PropertyGrid(struct mu_Context* gui_context);
    int ContextualPropertyGrid(struct mu_Context* gui_context);
    void Serialize(serializer_context* context);
    void Deserialize(serializer_context* context, uint16_t major, uint16_t minor);

    void SetInvalid() {m_Type = combination_begin;}
    bool IsValid() {return m_Type != combination_begin;}
    vec2& GetPoints(uint32_t index) {assert(index < GetNumPoints()); return m_Points[index];}
    const vec2& GetPoints(uint32_t index) const {assert(index < GetNumPoints()); return m_Points[index];}
    void SetPoints(uint32_t index, vec2 point) {assert(index < GetNumPoints()); m_Points[index] = point;}
    inline command_type GetType() const {return m_Type;}
    uint32_t GetNumPoints() const {return primitive_get_num_points(m_Type);}
    vec2 ComputerCenter() const;
    void UpdateAABB();
    float DistanceToNearestPoint(vec2 reference) const;     // returns square distance
    void SetAperture(float value) {m_Aperture = value;}
    void SetThickness(float value) {m_Thickness = value;}

public:
    static constexpr const float point_radius {6.f};
    static int m_SDFOperationComboBox;
};


