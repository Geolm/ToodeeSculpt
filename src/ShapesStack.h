#pragma once

#define CC_USE_ASSERT
#define CC_NO_SHORT_NAMES
#include "system/cc.h"
#include "system/aabb.h"
#include "shaders/common.h"
#include "system/undo.h"


struct mu_Context;
class Renderer;

//----------------------------------------------------------------------------------------------------------------------------
class ShapesStack
{
public:
    void Init(aabb zone);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(vec2 pos, int button, int action);
    void Draw(Renderer& renderer);
    void UserInterface(struct mu_Context* gui_context);
    void Undo();
    void Terminate();

private:
    //----------------------------------------------------------------------------------------------------------------------------
    // internal structures
    enum {SHAPE_MAXPOINTS = 4};
    enum {SHAPES_STACK_RESERVATION = 100};

    struct box_data
    {
        vec2 p0, p1;
        float width;
    };

    struct triangle_data
    {
        vec2 p0, p1, p2;
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

    union shape_color
    {
        struct {float red, green, blue, alpha;};
        struct {float hue, saturation, value;};
    };

    struct shape
    {
        shape_data shape_desc;
        command_type shape_type;
        float roundness;
        sdf_operator op;
        shape_color color;
    };

    enum state
    {
        IDLE,
        SHAPE_SELECTED,
        ADDING_POINTS,
        SET_ROUNDNESS
    };

private:
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    bool MouseCursorInShape(const shape* s);
    void DrawShapeGizmo(Renderer& renderer, const shape* s);

private:
    // serialized data 
    cc_vec(shape) m_Shapes;
    float m_SmoothBlend;
    float m_AlphaValue;

    // ui
    aabb m_EditionZone;
    vec2 m_ContextualMenuPosition;
    bool m_ContextualMenuOpen;
    int m_SDFOperationComboBox;
    vec2 m_MousePosition;

    // shape creation
    state m_CurrentState;
    uint32_t m_ShapeNumPoints;
    uint32_t m_CurrentPoint;
    vec2 m_ShapePoints[SHAPE_MAXPOINTS];
    vec2 m_RoundnessReference;
    float m_Roundness;
    command_type m_ShapeType;
    uint32_t m_SelectedShapeIndex;
    struct undo_context* m_UndoContext;
};

