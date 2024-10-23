#pragma once

#define CC_USE_ASSERT
#define CC_NO_SHORT_NAMES
#include "system/cc.h"
#include "system/aabb.h"
#include "shaders/common.h"


struct mu_Context;
class Renderer;

//----------------------------------------------------------------------------------------------------------------------------
class ShapesStack
{
public:
    void Init(aabb zone);
    void OnMouseButton(float x, float y, int button, int action);
    void Draw(Renderer& renderer);
    void UserInterface(struct mu_Context* gui_context);
    void Terminate();

private:

    //----------------------------------------------------------------------------------------------------------------------------
    // internal structures
    struct box_data
    {
        vec2 p0, p1;
        float width;
    };

    struct triangle_data
    {
        vec2 p0, p1, p3;
    };

    struct disc_data
    {
        vec2 center;
        float radius;
    };

    union shape_data
    {
        struct box_data box;
        struct disc_data disc;
        struct triangle_data triangle;
    };

    struct shape
    {
        shape_data shape_desc;
        command_type shape_type;
        float roundness;
        sdf_operator op;
    };

    enum mode
    {
        MODE_IDLE,
        MODE_NEW_SHAPE
    };

    //----------------------------------------------------------------------------------------------------------------------------
    // private properties
    cc_vec(shape) m_Shapes;
};

