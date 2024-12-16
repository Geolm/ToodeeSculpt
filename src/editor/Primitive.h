#pragma once


#include "../system/serializer.h"
#include "primitive_base.h"

#include <assert.h>

class Renderer;
struct mu_Context;

//----------------------------------------------------------------------------------------------------------------------------
class Primitive : public primitive_base
{
public:
    Primitive() {SetInvalid();}
    Primitive(command_type type, sdf_operator op, color4f color, float roundness, float width);

    void SetInvalid() {m_Type = combination_begin;}
    bool IsValid() {return m_Type != combination_begin;}
    vec2& GetPoints(uint32_t index) {assert(index < GetNumPoints()); return m_Points[index];}
    const vec2& GetPoints(uint32_t index) const {assert(index < GetNumPoints()); return m_Points[index];}
    void SetPoints(uint32_t index, vec2 point) {assert(index < GetNumPoints()); m_Points[index] = point;}
    inline command_type GetType() const {return m_Type;}
    uint32_t GetNumPoints() const {return primitive_get_num_points(m_Type);}
    void SetAperture(float value) {m_Aperture = value;}
    void SetThickness(float value) {m_Thickness = value;}

public:
    static constexpr const float point_radius {6.f};
    static int m_SDFOperationComboBox;
};


