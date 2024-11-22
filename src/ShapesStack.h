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
    void Init(aabb zone, struct undo_context* undo);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(int button, int action);
    void Draw(Renderer& renderer);
    void UserInterface(struct mu_Context* gui_context);
    void Undo();
    void DeleteSelected();
    void Terminate();

    void SetSnapToGrid(bool b) {m_SnapToGrid = b;}
    void SetGridSubdivision(float f) {m_GridSubdivision = f;}

private:
    //----------------------------------------------------------------------------------------------------------------------------
    // internal structures
    enum {SHAPE_MAXPOINTS = 3};
    enum {SHAPES_STACK_RESERVATION = 100};

    struct shape_data
    {
        vec2 points[SHAPE_MAXPOINTS];
        float width;
    };

    struct shape_color
    {
        float red, green, blue;
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
        SET_WIDTH,
        SET_ROUNDNESS,
        MOVING_POINT,
        MOVING_SHAPE
    };

private:
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    bool MouseCursorInShape(const shape* s, bool test_vertices);
    void DrawShapeGizmo(Renderer& renderer, const shape* s);
    void UndoSnapshot();
    inline bool SelectedShapeValid() {return m_SelectedShapeIndex < cc_size(&m_Shapes);}
    inline uint32_t ShapeNumPoints(command_type shape);

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
    uint32_t m_CurrentPoint;
    vec2 m_ShapePoints[SHAPE_MAXPOINTS];
    vec2 m_Reference;
    float m_Width;
    float m_Roundness;
    command_type m_ShapeType;
    uint32_t m_SelectedShapeIndex;
    struct undo_context* m_pUndoContext;
    vec2* m_pGrabbedPoint;
    bool m_SnapToGrid;
    float m_GridSubdivision;
};

//----------------------------------------------------------------------------------------------------------------------------
inline uint32_t ShapesStack::ShapeNumPoints(command_type shape)
{
    switch(shape)
    {
    case shape_oriented_box: return 2;
    case shape_arc:
    case shape_arc_filled:
    case shape_circle:
    case shape_circle_filled: return 1;
    case shape_triangle:
    case shape_triangle_filled: return 3;
    default: return 0;
    }
}
