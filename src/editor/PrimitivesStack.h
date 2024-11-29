#pragma once

#define CC_USE_ASSERT
#define CC_NO_SHORT_NAMES
#include "../system/cc.h"
#include "../system/aabb.h"
#include "../shaders/common.h"

struct mu_Context;
class Renderer;

//----------------------------------------------------------------------------------------------------------------------------
class PrimitivesStack
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
    enum {PRIMITIVE_MAXPOINTS = 3};
    enum {PRIMITIVES_STACK_RESERVATION = 100};

    struct primitive_data
    {
        vec2 points[PRIMITIVE_MAXPOINTS];
        float width;
    };

    struct primitive_color
    {
        float red, green, blue;
    };

    struct primitive
    {
        primitive_data primitive_desc;
        command_type primitive_type;
        float roundness;
        sdf_operator op;
        primitive_color color;
    };

    enum state
    {
        IDLE,
        PRIMITIVE_SELECTED,
        ADDING_POINTS,
        SET_WIDTH,
        SET_ROUNDNESS,
        CREATE_PRIMITIVE,
        MOVING_POINT,
        MOVING_PRIMITIVE
    };

private:
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    enum state GetState() const {return m_CurrentState;}
    bool MouseCursorInPrimitive(const primitive* s, bool test_vertices);
    void DrawPrimitiveGizmo(Renderer& renderer, const primitive* s);
    void DrawPrimitive(Renderer& renderer, const primitive* s, float roundness, draw_color color, sdf_operator op);
    void UndoSnapshot();
    inline bool SelectedPrimitiveValid() {return m_SelectedPrimitiveIndex < cc_size(&m_Primitives);}
    inline uint32_t PrimitiveNumPoints(command_type primitive);

private:
    // serialized data 
    cc_vec(primitive) m_Primitives;
    float m_SmoothBlend;
    float m_AlphaValue;

    // ui
    aabb m_EditionZone;
    vec2 m_ContextualMenuPosition;
    bool m_NewPrimitiveContextualMenuOpen;
    int m_SDFOperationComboBox;
    vec2 m_MousePosition;

    // primitive creation
    state m_CurrentState;
    uint32_t m_CurrentPoint;
    vec2 m_PrimitivePoints[PRIMITIVE_MAXPOINTS];
    vec2 m_Reference;
    float m_Width;
    float m_Roundness;
    command_type m_PrimitiveType;
    uint32_t m_SelectedPrimitiveIndex;
    struct undo_context* m_pUndoContext;
    vec2* m_pGrabbedPoint;
    bool m_SnapToGrid;
    float m_GridSubdivision;
};

//----------------------------------------------------------------------------------------------------------------------------
inline uint32_t PrimitivesStack::PrimitiveNumPoints(command_type primitive)
{
    switch(primitive)
    {
    case primitive_ellipse:
    case primitive_oriented_box: return 2;
    case primitive_arc:
    case primitive_arc_filled:
    case primitive_circle:
    case primitive_circle_filled: return 1;
    case primitive_triangle:
    case primitive_triangle_filled: return 3;
    default: return 0;
    }
}
