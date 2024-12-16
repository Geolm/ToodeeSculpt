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
    primitive_init(this, type, op, color, roundness, width);
}